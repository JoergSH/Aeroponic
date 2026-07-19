/*
 * ESP32 Aeroponik Steuerung
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <SPI.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Update.h>
#include <esp_wifi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_AHTX0.h>
#include "time.h"

#include "config.h"
#include "page.h"
#include "valves.h"
#include "tank.h"
#include "rs485.h"
#include "scheduler.h"
#include "storage.h"
#include "co2ctrl.h"
#include "fanctrl.h"
#include "wifictrl.h"
#include "outputctrl.h"
#include "espnow/espnow_master.h"

// HSPI (SPI3) für W5500 + SD: GPIO-Matrix erzwingt korrekte MOSI=13/MISO=11 Richtungen
// SPI2 (FSPI) würde GPIO11/13 per IO-MUX in native Richtungen zwingen und die Vertauschung ignorieren
static SPIClass BUS(HSPI);

WebServer server(80);

// DS18B20
OneWire           oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
float tempVorrat  = -127.0;
float tempPflanze = -127.0;

// DS18B20-Konvertierung läuft nicht-blockierend (setWaitForConversion(false) in setup())
static bool     tempConversionPending = false;
static uint32_t tempConversionStartMs = 0;
static uint16_t tempConversionDelayMs = 750;  // wird in setup() aus sensors.millisToWaitForConversion(12) gesetzt

// AHT21B (2. I2C-Bus, GPIO3/46 — siehe pinout.h)
static TwoWire        I2CBus2(1);
static Adafruit_AHTX0 aht21;
static bool           aht21Ok   = false;
static float          aht21Temp = NAN;
static float          aht21Hum  = NAN;

// Druck
float druckPSI  = 0.0;
float druckBar  = 0.0;

// Füllstand
int   distanzMM         = -1;
int   wasserHoeheMM     = 0;
int   fuellstandProzent = 0;
float volumenLiter      = 0.0;

// RTC
RTC_DS3231 rtc;
bool rtcOK = false;
unsigned long lastNtpSync = 0;
unsigned long lastRtcRetryMs = 0;
#define RTC_RETRY_INTERVAL_MS 30000UL  // Falls rtc.begin() beim Boot einmal fehlschlaegt
                                        // (z.B. I2C-Timing-Race mit SD/W5500-Init), hier
                                        // periodisch erneut versuchen statt bis zum naechsten
                                        // manuellen Reset auf rtcOK=false stehen zu bleiben.

// Forward-Deklaration
void syncRtcMitNtp();

// Ethernet
static bool ethConnected = false;

static volatile bool ethJustConnected = false;
static bool phyConfigured = false;

// W5500 PHYCFGR (0x002E): Auto-Negotiation abschalten, 100BT Full-Duplex erzwingen
// Wird aus loop() aufgerufen wenn ETH_CONNECTED feuert (nicht aus IRQ-Kontext)
static void w5500_force_phy_100fd() {
    // PHYCFGR: OPMD=1 (verwende OPMDC), OPMDC=011 (100BT FD), RST=0 → dann RST=1
    BUS.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS_W5500, LOW);
    BUS.transfer(0x00); BUS.transfer(0x2E); BUS.transfer(0x04); // Adresse + Write-Control
    BUS.transfer(0x58); // OPMD=1, OPMDC=011, RST=0
    digitalWrite(SPI_CS_W5500, HIGH);
    delayMicroseconds(200);
    digitalWrite(SPI_CS_W5500, LOW);
    BUS.transfer(0x00); BUS.transfer(0x2E); BUS.transfer(0x04);
    BUS.transfer(0xD8); // RST=1 → PHY-Reset zum Übernehmen der Config
    digitalWrite(SPI_CS_W5500, HIGH);
    BUS.endTransaction();
    Serial.println("[ETH] PHYCFGR: 100BT Full-Duplex erzwungen");
}

static void onNetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            ETH.setHostname("aeroponik");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("[ETH] Kabel verbunden");
            ethJustConnected = true;
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.printf("[ETH] IP: %s  %dMbps  %s-duplex\n",
                          ETH.localIP().toString().c_str(),
                          ETH.linkSpeed(),
                          ETH.fullDuplex() ? "Full" : "Half");
            ethConnected = true;
            syncRtcMitNtp();  // NTP bevorzugt über LAN
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("[ETH] Kabel getrennt");
            ethConnected = false;
            break;
        default: break;
    }
}

// Logging
static uint32_t lastLogMs = 0;
constexpr uint32_t LOG_INTERVAL_MS = 300000UL;  // 5 Minuten

// OTA Master
bool otaInProgress = false;

// OTA Slave-Nodes (Firmware im PSRAM buffern, dann per HTTP an Node)
static uint8_t*  ota_buf       = nullptr;
static size_t    ota_buf_len   = 0;
static uint8_t   ota_buf_node  = 0;
static char      ota_ap_pass[9] = {};
static bool      ota_ap_active  = false;
static uint32_t  ota_ap_start   = 0;
#define OTA_BUF_MAX    (2UL * 1024 * 1024)
#define OTA_AP_TIMEOUT 120000UL

static void generateOtaPass() {
    static const char chars[] = "ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789";
    for (int i = 0; i < 8; i++)
        ota_ap_pass[i] = chars[esp_random() % (sizeof(chars) - 1)];
    ota_ap_pass[8] = '\0';
}

unsigned long lastTempRead = 0;
unsigned long lastUltraRead = 0;

// ========== Ultraschall (interrupt-basiert, nicht-blockierend) ==========
// pulseIn() würde bis zu 30ms blockieren (Webserver/RS485/ESP-NOW stehen still) -
// stattdessen Echo-Flanken per Interrupt erfassen, Laufzeit in loop() auswerten.
static portMUX_TYPE ultraMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t ultraEchoStartUs    = 0;
static volatile uint32_t ultraEchoDurationUs = 0;
static volatile bool     ultraEchoActive     = false;
static volatile bool     ultraEchoReady      = false;
static bool     ultraWaitingEcho = false;
static uint32_t ultraTriggerMs   = 0;
constexpr uint32_t ULTRA_ECHO_TIMEOUT_MS = 30;

void IRAM_ATTR ultraEchoISR() {
    portENTER_CRITICAL_ISR(&ultraMux);
    if (digitalRead(ULTRASONIC_ECHO) == HIGH) {
        ultraEchoStartUs = micros();
        ultraEchoActive  = true;
    } else if (ultraEchoActive) {
        ultraEchoDurationUs = micros() - ultraEchoStartUs;
        ultraEchoActive     = false;
        ultraEchoReady      = true;
    }
    portEXIT_CRITICAL_ISR(&ultraMux);
}

// ========== ESP-NOW Sensordaten ==========

struct NodeData {
    float    temperature;
    float    humidity;
    uint16_t co2;
    float    ppfd;
    uint8_t  relay_mask;
    uint16_t pwm_ch1;
    uint16_t pwm_ch2;
    bool     has_air;
    bool     has_light;
    bool     has_relays;
    bool     has_pwm;
    uint32_t last_update_ms;
    // AS7341 Spektrum-Sensor
    uint8_t  spektrum[8];    // Kanäle F1-F8 normiert 0-255
    float    licht_ppfd;
    float    licht_rb;
    uint8_t  licht_bluepct;
    uint8_t  licht_redpct;
    bool     licht_sat;
    bool     has_spektrum;
    bool     has_sensor_channels;  // SUBTYPE_LIGHT 12 Bytes: Gain + 8 Kanäle
};
static NodeData nodeData[ESPNOW_MAX_NODES];

static int findNodeIndex(uint8_t node_id) {
    for (int i = 0; i < ESPNOW_MAX_NODES; i++)
        if (espnow_nodes[i].registered && espnow_nodes[i].node_id == node_id)
            return i;
    return -1;
}

void onEspNowSensor(uint8_t node_id, uint8_t /*node_type*/, const uint8_t* payload, uint8_t len) {
    int idx = findNodeIndex(node_id);
    if (idx < 0 || len < 1) return;
    NodeData& d = nodeData[idx];
    d.last_update_ms = millis();
    uint8_t sensor_type = payload[0];
    if ((sensor_type == 0x01 || sensor_type == 0x03) && len >= 7) {
        d.temperature = (int16_t)((payload[1] << 8) | payload[2]) / 100.0f;
        d.humidity    = (uint16_t)((payload[3] << 8) | payload[4]) / 100.0f;
        d.co2         = (uint16_t)((payload[5] << 8) | payload[6]);
        d.has_air     = true;
    }
    if (sensor_type == 0x03 && len >= 9) {
        d.ppfd      = (uint16_t)((payload[7] << 8) | payload[8]) / 10.0f;
        d.has_light = true;
    } else if (sensor_type == 0x02 && len >= 3) {
        d.ppfd      = (uint16_t)((payload[1] << 8) | payload[2]) / 10.0f;
        d.has_light = true;
        if (len >= 12) {
            for (int j = 0; j < 8; j++) d.spektrum[j] = payload[4 + j];
            d.has_sensor_channels = true;
        }
    }
    if (sensor_type == SUBTYPE_RELAY_STATUS && len >= 2) {
        d.relay_mask = payload[1];
        d.has_relays = true;
    }
    if (sensor_type == SUBTYPE_PWM_STATUS && len >= 5) {
        d.pwm_ch1 = (uint16_t)((payload[1] << 8) | payload[2]);
        d.pwm_ch2 = (uint16_t)((payload[3] << 8) | payload[4]);
        d.has_pwm = true;
    }
    if (sensor_type == SUBTYPE_LICHT_SUMMARY && len >= 16) {
        d.licht_ppfd    = (uint16_t)((payload[1] << 8) | payload[2]) / 10.0f;
        d.licht_rb      = (uint16_t)((payload[3] << 8) | payload[4]) / 100.0f;
        d.licht_bluepct = payload[5];
        d.licht_redpct  = payload[6];
        d.licht_sat     = payload[7] != 0;
        for (int j = 0; j < 8; j++) d.spektrum[j] = payload[8 + j];
        d.has_spektrum  = true;
    }
}

// ========== API ==========

void handleApiRs485() {
    const Rs485SensorData& d = rs485_get_ms2();
    JsonDocument doc;
    JsonObject ms2 = doc["ms2"].to<JsonObject>();
    ms2["addr"]    = RS485_ADDR_MS2;
    ms2["online"]  = d.online;
    if (d.online) {
        ms2["age_s"] = (millis() - d.last_update_ms) / 1000;
        ms2["co2"]   = d.co2_ppm;
        ms2["temp"]  = serialized(String(d.temp_c,  1));
        ms2["hum"]   = serialized(String(d.hum_pct, 1));
        ms2["ppfd"]  = serialized(String(d.ppfd,    1));
        ms2["status"] = d.status;
        JsonArray ch = ms2["channels"].to<JsonArray>();
        for (int i = 0; i < 8; i++) ch.add(d.channels[i]);
    }

    const Rs485LichtData& l = rs485_get_licht();
    JsonObject licht = doc["licht"].to<JsonObject>();
    licht["online"] = l.online;
    if (l.online) licht["mask"] = l.mask;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiLogsList() {
    JsonDocument doc;
    JsonArray arr = doc["files"].to<JsonArray>();
    storage_list_logs([](const char* name, size_t size, void* ctx) {
        JsonArray* a = (JsonArray*)ctx;
        JsonObject f = a->add<JsonObject>();
        f["name"] = name;
        f["size"] = size;
    }, &arr);
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiLogsDownload() {
    if (!server.hasArg("file")) { server.send(400, "text/plain", "file fehlt"); return; }
    String name = server.arg("file");
    if (name.indexOf('/') >= 0 || name.indexOf("..") >= 0) {
        server.send(400, "text/plain", "Ungueltiger Dateiname");
        return;
    }
    File f = storage_open_log(name.c_str());
    if (!f) { server.send(404, "text/plain", "Datei nicht gefunden"); return; }
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
    server.streamFile(f, "text/csv");
    f.close();
}

void handleApiLogsDelete() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String name = doc["file"] | "";
    if (name.isEmpty() || name.indexOf('/') >= 0 || name.indexOf("..") >= 0) {
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    bool ok = storage_delete_log(name.c_str());
    server.send(ok ? 200 : 404, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleApiPcfTest() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc["testmode"].is<bool>()) {
        pcf_test_mode(doc["testmode"]);
    } else if (doc["pin"].is<int>() && doc["on"].is<bool>()) {
        if (!pcf_test_active()) { server.send(400, "application/json", "{\"error\":\"Testmodus nicht aktiv\"}"); return; }
        pcf_test_set((uint8_t)(int)doc["pin"], (bool)doc["on"]);
    }
    char resp[50];
    snprintf(resp, sizeof(resp), "{\"success\":true,\"state\":%d,\"testmode\":%s}",
             pcf_get_state(), pcf_test_active() ? "true" : "false");
    server.send(200, "application/json", resp);
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleApiData() {
  JsonDocument doc;
  doc["tempVorrat"]        = (tempVorrat  > -100) ? tempVorrat  : 0.0;
  doc["tempPflanze"]       = (tempPflanze > -100) ? tempPflanze : 0.0;
  doc["aht21Ok"]           = aht21Ok;
  if (aht21Ok) {
    doc["aht21Temp"] = serialized(String(aht21Temp, 1));
    doc["aht21Hum"]  = serialized(String(aht21Hum,  1));
  }
  doc["druckBar"]          = serialized(String(druckBar, 2));
  doc["distanzMM"]         = distanzMM;
  doc["wasserHoeheMM"]     = wasserHoeheMM;
  doc["fuellstandProzent"] = fuellstandProzent;
  doc["volumenLiter"]      = serialized(String(volumenLiter, 1));

  if (rtcOK) {
    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d",
             now.day(), now.month(), now.year(),
             now.hour(), now.minute(), now.second());
    doc["datetime"] = buf;
  } else {
    doc["datetime"] = "RTC fehlt";
  }
  doc["freeHeap"]    = ESP.getFreeHeap();
  doc["uptime"]      = millis() / 1000;
  doc["chipModel"]   = ESP.getChipModel();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["flashSize"]   = ESP.getFlashChipSize();
  doc["sketchSize"]  = ESP.getSketchSize();

  if (WiFi.getMode() == WIFI_AP) {
    doc["wifiMode"]  = "AP";
    doc["ipAddress"] = WiFi.softAPIP().toString();
  } else {
    doc["wifiMode"]  = "STA";
    doc["ipAddress"] = WiFi.localIP().toString();
  }
  if (ethConnected) {
    doc["ethIP"]  = ETH.localIP().toString();
    doc["sdOk"]   = storage_is_ok();
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ========== ESP-NOW API ==========

void handleApiEspNow() {
    JsonDocument doc;
    JsonArray arr = doc["nodes"].to<JsonArray>();
    int count = 0;
    unsigned long now = millis();
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (!espnow_nodes[i].registered) continue;
        JsonObject n = arr.add<JsonObject>();
        n["id"]     = espnow_nodes[i].node_id;
        n["type"]   = espnow_nodes[i].node_type;
        n["online"] = espnow_nodes[i].online;
        n["since_s"] = (now - espnow_nodes[i].last_heartbeat_ms) / 1000;
        if (nodeData[i].last_update_ms > 0) {
            n["age_s"] = (now - nodeData[i].last_update_ms) / 1000;
            if (nodeData[i].has_air) {
                n["temperature"] = serialized(String(nodeData[i].temperature, 2));
                n["humidity"]    = serialized(String(nodeData[i].humidity,    2));
                n["co2"]         = nodeData[i].co2;
            }
            if (nodeData[i].has_light)
                n["ppfd"] = serialized(String(nodeData[i].ppfd, 1));
            if (nodeData[i].has_sensor_channels) {
                JsonArray sp = n["spektrum"].to<JsonArray>();
                for (int j = 0; j < 8; j++) sp.add(nodeData[i].spektrum[j]);
            }
            if (nodeData[i].has_relays)
                n["relay_mask"] = nodeData[i].relay_mask;
            if (nodeData[i].has_pwm) {
                n["pwm_ch1"] = nodeData[i].pwm_ch1;
                n["pwm_ch2"] = nodeData[i].pwm_ch2;
            }
            if (nodeData[i].has_spektrum) {
                n["licht_ppfd"]    = serialized(String(nodeData[i].licht_ppfd,   1));
                n["licht_rb"]      = serialized(String(nodeData[i].licht_rb,     2));
                n["licht_bluepct"] = nodeData[i].licht_bluepct;
                n["licht_redpct"]  = nodeData[i].licht_redpct;
                n["licht_sat"]     = nodeData[i].licht_sat;
                JsonArray sp = n["spektrum"].to<JsonArray>();
                for (int j = 0; j < 8; j++) sp.add(nodeData[i].spektrum[j]);
            }
        }
        count++;
    }
    doc["count"] = count;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiLicht() {
    JsonDocument doc;
    JsonArray arr = doc["nodes"].to<JsonArray>();
    unsigned long now = millis();
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (!espnow_nodes[i].registered) continue;
        if (espnow_nodes[i].node_type != NODE_TYPE_SPEKTRUM) continue;
        if (!nodeData[i].has_spektrum) continue;
        JsonObject n = arr.add<JsonObject>();
        n["id"]     = espnow_nodes[i].node_id;
        n["online"] = espnow_nodes[i].online;
        n["age_s"]  = (now - nodeData[i].last_update_ms) / 1000;
        n["p"]      = serialized(String(nodeData[i].licht_ppfd,   1));
        n["r"]      = serialized(String(nodeData[i].licht_rb,     3));
        n["b"]      = nodeData[i].licht_bluepct;
        n["a"]      = nodeData[i].licht_redpct;
        n["s"]      = nodeData[i].licht_sat;
        JsonArray f = n["f"].to<JsonArray>();
        for (int j = 0; j < 8; j++) f.add(nodeData[i].spektrum[j]);
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiPwmSet() {
    if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    uint8_t  node_id = doc["node_id"] | 0;
    uint16_t ch1     = constrain((int)(doc["ch1"] | 0), 0, 1000);
    uint16_t ch2     = constrain((int)(doc["ch2"] | 0), 0, 1000);
    bool ok = espnow_send_pwm(node_id, ch1, ch2);
    server.send(200, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleApiRelaySet() {
    if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    uint8_t node_id = doc["node_id"] | 0;
    uint8_t mask    = doc["mask"]    | 0;
    bool ok = espnow_send_relay_mask(node_id, mask);
    server.send(200, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
}

// ========== Scheduler API ==========

void handleApiSchedulerGet() {
    JsonDocument doc;
    doc["enabled"]     = schedConfig.enabled;
    doc["dawn_start"]  = schedConfig.dawn_start;
    doc["dawn_end"]    = schedConfig.dawn_end;
    doc["dusk_start"]  = schedConfig.dusk_start;
    doc["dusk_end"]    = schedConfig.dusk_end;
    doc["num_ssr"]     = schedConfig.num_ssr;
    if (rtcOK) {
        DateTime now = rtc.now();
        uint16_t now_min = now.hour() * 60 + now.minute();
        doc["now_min"]      = now_min;
        doc["cur_mask"]     = schedConfig.enabled ? schedComputeRelayMask(now_min) : 0;
        doc["cur_ssr_count"] = schedConfig.enabled ? __builtin_popcount(schedComputeRelayMask(now_min)) : 0;
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiSchedulerSave() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc["enabled"].is<bool>())    schedConfig.enabled     = doc["enabled"];
    if (doc["dawn_start"].is<int>())  schedConfig.dawn_start  = constrain((int)doc["dawn_start"], 0, 1439);
    if (doc["dawn_end"].is<int>())    schedConfig.dawn_end    = constrain((int)doc["dawn_end"],   1, 1439);
    if (doc["dusk_start"].is<int>())  schedConfig.dusk_start  = constrain((int)doc["dusk_start"], 0, 1439);
    if (doc["dusk_end"].is<int>())    schedConfig.dusk_end    = constrain((int)doc["dusk_end"],   1, 1439);
    if (doc["num_ssr"].is<int>())     schedConfig.num_ssr     = constrain((int)doc["num_ssr"],    1, 4);
    saveScheduleConfig();
    server.send(200, "application/json", "{\"success\":true}");
}

// ========== Tank API ==========

void handleApiTankConfig() {
  JsonDocument doc;
  doc["hoehe_mm"]        = tankConfig.hoehe_mm;
  doc["radius_unten_mm"] = tankConfig.radius_unten_mm;
  doc["radius_oben_mm"]  = tankConfig.radius_oben_mm;
  doc["leer_abstand_mm"] = tankConfig.leer_abstand_mm;
  doc["min_mm"]          = tankConfig.min_mm;
  doc["max_mm"]          = tankConfig.max_mm;
  doc["offset_mm"]       = tankConfig.offset_mm;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleApiTankConfigSave() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, server.arg("plain"));
  if (doc["hoehe_mm"].is<int>())        tankConfig.hoehe_mm        = constrain((int)doc["hoehe_mm"],        50, 2000);
  if (doc["radius_unten_mm"].is<int>()) tankConfig.radius_unten_mm = constrain((int)doc["radius_unten_mm"], 10, 1000);
  if (doc["radius_oben_mm"].is<int>())  tankConfig.radius_oben_mm  = constrain((int)doc["radius_oben_mm"],  10, 1000);
  if (doc["leer_abstand_mm"].is<int>()) tankConfig.leer_abstand_mm = constrain((int)doc["leer_abstand_mm"], 50, 5000);
  if (doc["min_mm"].is<int>())          tankConfig.min_mm          = constrain((int)doc["min_mm"],          20,  500);
  if (doc["max_mm"].is<int>())          tankConfig.max_mm          = constrain((int)doc["max_mm"],         100, 10000);
  if (doc["offset_mm"].is<int>())       tankConfig.offset_mm       = constrain((int)doc["offset_mm"],     -200,  200);
  saveTankConfig();
  server.send(200, "application/json", "{\"success\":true}");
}

// ========== Ventil API ==========

void handleApiVentilStatus() {
  JsonDocument doc;
  for (int i = 0; i < 3; i++) {
    JsonObject b = doc["behaelter"][i].to<JsonObject>();
    b["zustand"]        = ventilZustandText(behaelterZustand[i].zustand);
    b["aktiv"]          = ventilConfig.behaelter[i].aktiv;
    b["oeffnungszeit_s"] = ventilConfig.behaelter[i].oeffnungszeit_s;
    b["pausenzeit_min"]  = ventilConfig.behaelter[i].pausenzeit_min;
    unsigned long vergangen = (millis() - behaelterZustand[i].timerStart) / 1000;
    b["timer_s"]        = vergangen;
  }
  doc["vorlaufzeit_s"]  = ventilConfig.vorlaufzeit_s;
  doc["vorlauf_aktiv"]  = ventilConfig.vorlauf_aktiv;
  doc["rueck_offen"]    = ventilRueckOffen();
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleApiVentilConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, server.arg("plain"));

  for (int i = 0; i < 3; i++) {
    if (doc["behaelter"][i]["oeffnungszeit_s"].is<int>())
      ventilConfig.behaelter[i].oeffnungszeit_s = constrain((int)doc["behaelter"][i]["oeffnungszeit_s"], 1, 60);
    if (doc["behaelter"][i]["pausenzeit_min"].is<int>())
      ventilConfig.behaelter[i].pausenzeit_min  = constrain((int)doc["behaelter"][i]["pausenzeit_min"], 1, 60);
    if (doc["behaelter"][i]["aktiv"].is<bool>())
      ventilConfig.behaelter[i].aktiv = doc["behaelter"][i]["aktiv"];
  }
  if (doc["vorlaufzeit_s"].is<int>())
    ventilConfig.vorlaufzeit_s = constrain((int)doc["vorlaufzeit_s"], 0, 30);
  if (doc["vorlauf_aktiv"].is<bool>())
    ventilConfig.vorlauf_aktiv = doc["vorlauf_aktiv"];

  saveVentilConfig();
  server.send(200, "application/json", "{\"success\":true}");
}

// ========== NTP → RTC Sync ==========

void syncRtcMitNtp() {
  if (!rtcOK || WiFi.status() != WL_CONNECTED) return;

  configTzTime(NTP_TZ, NTP_SERVER);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 3000)) {
    Serial.println("NTP: kein Signal");
    return;
  }

  // Beide als Lokalzeit vergleichen: RTClib kennt keine Timezone,
  // unixtime() behandelt die gespeicherte Zeit als wäre sie UTC.
  // mktime() würde Lokalzeit → echtes UTC umrechnen → 7200s Offset.
  DateTime ntpLokal(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                    timeinfo.tm_mday, timeinfo.tm_hour,
                    timeinfo.tm_min, timeinfo.tm_sec);
  long abweichung = abs((long)((int32_t)ntpLokal.unixtime() - (int32_t)rtc.now().unixtime()));

  if (abweichung > NTP_MAX_ABWEICHUNG) {
    rtc.adjust(ntpLokal);
    Serial.printf("RTC gestellt (Abweichung: %ld s)\n", abweichung);
  } else {
    Serial.printf("RTC OK (Abweichung: %ld s)\n", abweichung);
  }
  lastNtpSync = millis();
}

// ========== OTA ==========

void handleOTAUpdate() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA Start: %s\n", upload.filename.c_str());
    otaInProgress = true;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA OK: %u bytes\n", upload.totalSize);
      delay(2000);
      ESP.restart();
    } else {
      Update.printError(Serial);
    }
    otaInProgress = false;
  }
}

void handleOTAResult() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
}

// ========== Node-OTA (Slave-Firmware) ==========

void handleNodeOtaUpload() {
    HTTPUpload& u = server.upload();
    if (u.status == UPLOAD_FILE_START) {
        ota_buf_node = (uint8_t)server.arg("node_id").toInt();
        if (!ota_buf)
            ota_buf = (uint8_t*)heap_caps_malloc(OTA_BUF_MAX, MALLOC_CAP_SPIRAM);
        ota_buf_len = 0;
        Serial.printf("[NODE-OTA] Upload Start fuer Node %d\n", ota_buf_node);
    } else if (u.status == UPLOAD_FILE_WRITE) {
        if (ota_buf && ota_buf_len + u.currentSize <= OTA_BUF_MAX) {
            memcpy(ota_buf + ota_buf_len, u.buf, u.currentSize);
            ota_buf_len += u.currentSize;
        }
    } else if (u.status == UPLOAD_FILE_END) {
        Serial.printf("[NODE-OTA] Upload fertig: %u Bytes\n", (unsigned)ota_buf_len);
    }
}

void handleNodeOtaUploadDone() {
    if (!ota_buf || ota_buf_len == 0) {
        server.send(500, "application/json", "{\"error\":\"Upload fehlgeschlagen\"}");
        return;
    }
    char resp[80];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"size\":%u,\"node_id\":%u}",
             (unsigned)ota_buf_len, (unsigned)ota_buf_node);
    server.send(200, "application/json", resp);
}

void handleNodeOtaFlash() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    if (!ota_buf || ota_buf_len == 0) {
        server.send(400, "application/json", "{\"error\":\"Kein Firmware im Buffer\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    uint8_t node_id = doc["node_id"] | 0;
    if (node_id != ota_buf_node) {
        server.send(400, "application/json", "{\"error\":\"node_id passt nicht zum Upload\"}");
        return;
    }
    generateOtaPass();
    WiFi.softAP(OTA_AP_SSID, ota_ap_pass);
    ota_ap_active = true;
    ota_ap_start  = millis();
    Serial.printf("[NODE-OTA] AP '%s' gestartet  Pass=%s  IP=%s\n",
                  OTA_AP_SSID, ota_ap_pass, WiFi.softAPIP().toString().c_str());
    bool ok = espnow_trigger_ota(node_id, ota_ap_pass);
    char resp[80];
    snprintf(resp, sizeof(resp), "{\"success\":%s,\"size\":%u}",
             ok ? "true" : "false", (unsigned)ota_buf_len);
    server.send(200, "application/json", resp);
}

void handleNodeOtaBin() {
    if (!ota_buf || ota_buf_len == 0) {
        server.send(404, "text/plain", "kein Firmware");
        return;
    }
    Serial.printf("[NODE-OTA] Sende %u Bytes\n", (unsigned)ota_buf_len);
    server.setContentLength(ota_buf_len);
    server.send(200, "application/octet-stream", "");
    WiFiClient client = server.client();
    const size_t CHUNK = 1024;
    size_t sent = 0;
    while (sent < ota_buf_len) {
        size_t n = min(CHUNK, ota_buf_len - sent);
        size_t w = client.write(ota_buf + sent, n);
        if (w == 0) break;
        sent += w;
    }
    Serial.printf("[NODE-OTA] Gesendet: %u Bytes\n", (unsigned)sent);
}

// ========== Scheduler Loop ==========

void loopScheduler() {
    if (!schedConfig.enabled || !rtcOK) return;
    static uint8_t  last_mask    = 0xFF;  // 0xFF = unbekannt, erzwingt ersten Send
    static uint8_t  last_percent = 0xFF;
    static uint32_t last_check   = 0;
    if (millis() - last_check < 30000) return;
    last_check = millis();
    DateTime now = rtc.now();
    uint16_t now_min = now.hour() * 60 + now.minute();

    if (outputConfig.light_output == LIGHT_OUTPUT_ANALOG) {
        uint8_t pct = schedComputeBrightnessPercent(now_min);
        if (pct != last_percent) {
            rs485_set_analog_ch2((uint16_t)roundf(pct / 100.0f * 4095));
            last_percent = pct;
            Serial.printf("[Sched] Licht-Helligkeit (Analog)=%u%%\n", pct);
        }
    } else {
        uint8_t mask = schedComputeRelayMask(now_min);
        if (mask != last_mask) {
            rs485_set_licht_mask(mask);
            last_mask = mask;
            Serial.printf("[Sched] Licht-Maske=0x%02X (%d SSRs an)\n",
                          mask, __builtin_popcount(mask));
        }
    }
}

// ========== CO2-Steuerung ==========
// Hysterese: CO2 <= co2_min -> Ausgang EIN, CO2 >= co2_max -> Ausgang AUS,
// dazwischen bleibt der Zustand erhalten. CO2-Quelle ist der RS485-Multisensor (MS2).

static bool co2OutputOn = false;

void loopCo2Ctrl() {
    if (!co2Config.enabled) return;
    static uint32_t last_check = 0;
    if (millis() - last_check < 5000) return;
    last_check = millis();

    const Rs485SensorData& ms2 = rs485_get_ms2();
    if (!ms2.online) return;  // keine verlaessliche CO2-Messung

    if (ms2.co2_ppm <= co2Config.co2_min)      co2OutputOn = true;
    else if (ms2.co2_ppm >= co2Config.co2_max) co2OutputOn = false;
    // sonst: Zustand halten (Hysterese-Band)

    int idx = findNodeIndex(co2Config.target_node_id);
    if (idx < 0 || !espnow_nodes[idx].online) return;
    uint8_t bit  = co2Config.target_relay_bit & 0x03;
    uint8_t mask = nodeData[idx].relay_mask;
    uint8_t want = co2OutputOn ? (mask | (1 << bit)) : (mask & ~(1 << bit));
    if (want != mask) {
        if (espnow_send_relay_mask(co2Config.target_node_id, want))
            Serial.printf("[CO2] Node %d Bit %d -> %s (CO2=%u ppm)\n",
                          co2Config.target_node_id, bit, co2OutputOn ? "EIN" : "AUS", ms2.co2_ppm);
    }
}

// ========== CO2 API ==========

void handleApiCo2Get() {
    JsonDocument doc;
    doc["enabled"]          = co2Config.enabled;
    doc["target_node_id"]   = co2Config.target_node_id;
    doc["target_relay_bit"] = co2Config.target_relay_bit;
    doc["co2_min"]          = co2Config.co2_min;
    doc["co2_max"]          = co2Config.co2_max;
    doc["output_on"]        = co2OutputOn;
    const Rs485SensorData& ms2 = rs485_get_ms2();
    doc["co2_online"]       = ms2.online;
    if (ms2.online) doc["co2_now"] = ms2.co2_ppm;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiCo2Save() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc["enabled"].is<bool>())          co2Config.enabled          = doc["enabled"];
    if (doc["target_node_id"].is<int>())    co2Config.target_node_id   = constrain((int)doc["target_node_id"],   0, 254);
    if (doc["target_relay_bit"].is<int>())  co2Config.target_relay_bit = constrain((int)doc["target_relay_bit"], 0, 3);
    if (doc["co2_min"].is<int>())           co2Config.co2_min          = constrain((int)doc["co2_min"], 0, 5000);
    if (doc["co2_max"].is<int>())           co2Config.co2_max          = constrain((int)doc["co2_max"], 0, 5000);
    saveCo2Config();
    server.send(200, "application/json", "{\"success\":true}");
}

// ========== Abluftluefter-Steuerung (MARS Hydro, RS485-Adresse 6) ==========
// Manuell: fester %-Wert. Automodi: linearer Ramp zwischen Mindestdrehzahl (bei/unter Min)
// und 100% (bei/ueber Max), Quelle Luftfeuchte bzw. Temperatur vom RS485-Multisensor (MS2).
// enabled=false: keine eigenen Schreibbefehle, iControl behaelt die Kontrolle.

static uint8_t fanRampPercent(float value, uint8_t minVal, uint8_t maxVal, uint8_t minSpeed) {
    if (maxVal <= minVal) return minSpeed;  // Schutz gegen ungueltige Konfiguration
    float t = (value - minVal) / (float)(maxVal - minVal);
    t = constrain(t, 0.0f, 1.0f);
    return minSpeed + (uint8_t)roundf(t * (100 - minSpeed));
}

void loopFanCtrl() {
    if (!fanConfig.enabled) return;
    static uint32_t last_check = 0;
    if (millis() - last_check < 5000) return;
    last_check = millis();

    static uint8_t last_sent = 0xFF;  // 0xFF = unbekannt, erzwingt ersten Send
    uint8_t target;

    if (fanConfig.mode == FAN_MODE_MANUAL) {
        target = fanConfig.manual_percent;
    } else {
        const Rs485SensorData& ms2 = rs485_get_ms2();
        if (!ms2.online) return;  // keine verlaessliche Feuchte-/Temperaturmessung
        if (fanConfig.mode == FAN_MODE_AUTO_HUMIDITY) {
            target = fanRampPercent(ms2.hum_pct, fanConfig.hum_min, fanConfig.hum_max, fanConfig.min_speed);
        } else if (fanConfig.temp_mode == FAN_TEMP_MODE_DIFF) {
            // Reine Differenz-Rampe: bringt der Raum kaum Kuehlung (Zelt~Raum-Temp),
            // ist Volllast sinnlos — Min/Max beziehen sich hier auf die Differenz selbst.
            if (!aht21Ok) return;  // Raumtemperatur noetig, Zustand halten bis wieder verfuegbar
            float diff = ms2.temp_c - aht21Temp;
            target = fanRampPercent(diff, fanConfig.temp_min, fanConfig.temp_max, fanConfig.min_speed);
        } else {
            // Absolute Zelttemperatur-Rampe, optional gedeckelt durch die Differenz-Bremse
            target = fanRampPercent(ms2.temp_c, fanConfig.temp_min, fanConfig.temp_max, fanConfig.min_speed);
            if (fanConfig.temp_cap_diff > 0 && aht21Ok) {
                float diff = ms2.temp_c - aht21Temp;
                if (diff < fanConfig.temp_cap_diff) target = fanConfig.min_speed;
            }
        }
    }

    if (target != last_sent) {
        bool sent = (outputConfig.fan_output == FAN_OUTPUT_ANALOG)
            ? rs485_set_analog_ch1((uint16_t)roundf(target / 100.0f * 4095))
            : rs485_set_fan_percent(target);
        if (sent) {
            last_sent = target;
            Serial.printf("[Fan] Ziel-Leistung=%u%% (Modus=%d, Ausgang=%s)\n", target, fanConfig.mode,
                          outputConfig.fan_output == FAN_OUTPUT_ANALOG ? "Analog" : "MARS");
        }
    }
}

// ========== Fan API ==========

void handleApiFanGet() {
    JsonDocument doc;
    doc["mode"]            = fanConfig.mode;
    doc["manual_percent"]  = fanConfig.manual_percent;
    doc["min_speed"]       = fanConfig.min_speed;
    doc["hum_min"]         = fanConfig.hum_min;
    doc["hum_max"]         = fanConfig.hum_max;
    doc["temp_min"]        = fanConfig.temp_min;
    doc["temp_max"]        = fanConfig.temp_max;
    doc["enabled"]         = fanConfig.enabled;
    doc["temp_mode"]       = fanConfig.temp_mode;
    doc["temp_cap_diff"]   = fanConfig.temp_cap_diff;
    doc["room_ok"]         = aht21Ok;
    if (aht21Ok) doc["room_temp"] = serialized(String(aht21Temp, 1));

    // Status/Ist-Werte kommen vom tatsaechlich aktiven Ausgang — beim jeweils anderen
    // Geraet ist das Pollen ja bewusst abgeschaltet (siehe outputctrl.h) und wuerde immer
    // "offline" liefern.
    if (outputConfig.fan_output == FAN_OUTPUT_ANALOG) {
        const Rs485AnalogData& analog = rs485_get_analog();
        doc["online"] = analog.online;
        if (analog.online) doc["percent"] = (int)roundf(analog.ch1_raw / 4095.0f * 100.0f);
    } else {
        const Rs485FanData& fan = rs485_get_fan();
        doc["online"] = fan.online;
        if (fan.online) {
            doc["percent"] = fan.percent;
            doc["rpm"]     = fan.rpm;
        }
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiFanSave() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc["mode"].is<int>())            fanConfig.mode           = constrain((int)doc["mode"], 0, 2);
    if (doc["manual_percent"].is<int>())  fanConfig.manual_percent = constrain((int)doc["manual_percent"], 0, 100);
    if (doc["min_speed"].is<int>())       fanConfig.min_speed      = constrain((int)doc["min_speed"], 0, 100);
    if (doc["hum_min"].is<int>())         fanConfig.hum_min        = constrain((int)doc["hum_min"], 0, 100);
    if (doc["hum_max"].is<int>())         fanConfig.hum_max        = constrain((int)doc["hum_max"], 0, 100);
    if (doc["temp_min"].is<int>())        fanConfig.temp_min       = constrain((int)doc["temp_min"], 0, 60);
    if (doc["temp_max"].is<int>())        fanConfig.temp_max       = constrain((int)doc["temp_max"], 0, 60);
    if (doc["enabled"].is<bool>())        fanConfig.enabled        = doc["enabled"];
    if (doc["temp_mode"].is<int>())       fanConfig.temp_mode      = constrain((int)doc["temp_mode"], 0, 1);
    if (doc["temp_cap_diff"].is<int>())   fanConfig.temp_cap_diff  = constrain((int)doc["temp_cap_diff"], 0, 30);
    saveFanConfig();
    server.send(200, "application/json", "{\"success\":true}");
}

// ========== Ausgangs-Konfiguration (Licht/Luefter: Lichtx4/MARS vs. Analogmodul) ==========
// Setup-Entscheidung statt Laufzeit-Umschaltung: legt fest, welches RS485-Geraet
// ueberhaupt gepollt wird (siehe rs485_set_*_polling) und welches Geraet die
// Steuerbefehle aus loopScheduler()/loopFanCtrl() bekommt.

static void applyOutputPollingFlags() {
    rs485_set_licht_polling(outputConfig.light_output == LIGHT_OUTPUT_LICHTX4);
    rs485_set_fan_polling(outputConfig.fan_output == FAN_OUTPUT_MARS);
    rs485_set_analog_polling(outputConfig.fan_output   == FAN_OUTPUT_ANALOG ||
                              outputConfig.light_output == LIGHT_OUTPUT_ANALOG);
}

void handleApiOutputGet() {
    JsonDocument doc;
    doc["fan_output"]   = outputConfig.fan_output;
    doc["light_output"] = outputConfig.light_output;
    const Rs485AnalogData& a = rs485_get_analog();
    doc["analog_online"] = a.online;
    if (a.online) {
        doc["analog_ch1_raw"] = a.ch1_raw;
        doc["analog_ch2_raw"] = a.ch2_raw;
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiOutputSave() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc["fan_output"].is<int>())   outputConfig.fan_output   = constrain((int)doc["fan_output"],   0, 1);
    if (doc["light_output"].is<int>()) outputConfig.light_output = constrain((int)doc["light_output"], 0, 1);
    saveOutputConfig();
    applyOutputPollingFlags();
    server.send(200, "application/json", "{\"success\":true}");
}

// ========== WLAN-Konfiguration ==========

static bool wifiRestartPending = false;
static uint32_t wifiRestartAtMs = 0;

void handleApiWifiGet() {
    JsonDocument doc;
    doc["ssid"]         = wifiConfig.ssid;
    doc["has_password"] = strlen(wifiConfig.password) > 0;
    doc["ap_active"]    = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    doc["connected"]    = (WiFi.status() == WL_CONNECTED);
    if (WiFi.status() == WL_CONNECTED) doc["ip"] = WiFi.localIP().toString();
    doc["ap_ssid"] = AP_SSID;
    doc["ap_ip"]   = WiFi.softAPIP().toString();
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiWifiSave() {
    if (server.method() != HTTP_POST) { server.send(405); return; }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    if (!doc["ssid"].is<const char*>()) { server.send(400, "application/json", "{\"success\":false}"); return; }
    strncpy(wifiConfig.ssid, doc["ssid"] | "", sizeof(wifiConfig.ssid) - 1);
    wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';
    // Passwort nur ueberschreiben, wenn eines mitgeschickt wurde (leer = vorhandenes behalten)
    const char* pw = doc["password"] | "";
    if (strlen(pw) > 0) {
        strncpy(wifiConfig.password, pw, sizeof(wifiConfig.password) - 1);
        wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';
    }
    saveWifiConfig();
    server.send(200, "application/json", "{\"success\":true,\"restarting\":true}");
    wifiRestartPending = true;
    wifiRestartAtMs = millis() + 1500;
}

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP32 Aeroponik");

  EEPROM.begin(EEPROM_SIZE);
  loadTankConfig();
  loadScheduleConfig();
  loadCo2Config();
  loadFanConfig();
  loadWifiConfig();
  loadOutputConfig();
  applyOutputPollingFlags();

  // SPI + W5500 Ethernet (vor WiFi, damit lwIP beide Interfaces kennt)
  WiFi.onEvent(onNetworkEvent);
  BUS.begin(SPI_SCK, SPI_MISO, SPI_MOSI, -1);
  delay(200);  // W5500 auf ESP32-S3: IPC-Task braucht Zeit zur Initialisierung
  ETH.begin(ETH_PHY_W5500, 1, SPI_CS_W5500, WSINT, -1, BUS);

  // MicroSD (gleicher physischer Bus, anderer CS)
  storage_init(BUS);

  // I2C + RTC
  Wire.begin(I2C_SDA, I2C_SCL);
  if (rtc.begin()) {
    rtcOK = true;
    if (rtc.lostPower()) {
      Serial.println("RTC: Strom weg gewesen — Uhrzeit prüfen!");
    } else {
      DateTime now = rtc.now();
      Serial.printf("RTC: %02d.%02d.%04d %02d:%02d:%02d\n",
                    now.day(), now.month(), now.year(),
                    now.hour(), now.minute(), now.second());
    }
  } else {
    Serial.println("DS1307 nicht gefunden!");
  }

  // I2C Bus 2 + AHT21B
  I2CBus2.begin(I2C2_SDA, I2C2_SCL);
  aht21Ok = aht21.begin(&I2CBus2);
  if (aht21Ok) {
    // Adafruit_AHTX0 nutzt CMD_CALIBRATE=0xE1 (AHT10-Befehl), AHT20/AHT21(B)
    // erwarten stattdessen 0xBE — bekannter Library-Bug (siehe
    // github.com/adafruit/Adafruit_CircuitPython_AHTx0 Issue #17). Mit dem
    // korrekten Befehl nachkalibrieren, sonst kann z.B. der Feuchtekanal
    // falsche/gesaettigte Werte liefern, obwohl die Temperatur stimmt.
    I2CBus2.beginTransmission(AHTX0_I2CADDR_DEFAULT);
    I2CBus2.write(0xBE);
    I2CBus2.write(0x08);
    I2CBus2.write(0x00);
    I2CBus2.endTransmission();
    while (aht21.getStatus() & AHTX0_STATUS_BUSY) delay(10);
  }
  Serial.println(aht21Ok ? "AHT21B gefunden" : "AHT21B nicht gefunden!");

  // WiFi
  bool startFallbackAP = USE_ACCESS_POINT;
  if (!USE_ACCESS_POINT) {
    if (strlen(wifiConfig.ssid) == 0) {
      // Kein WLAN konfiguriert (Erststart) — direkt in den Fallback-AP,
      // ohne erst 30s auf eine leere SSID zu warten.
      Serial.println("Kein WLAN konfiguriert, starte Fallback-AP");
      startFallbackAP = true;
    } else {
      WiFi.persistent(false);
      WiFi.mode(WIFI_OFF);
      delay(500);
      WiFi.mode(WIFI_STA);
      delay(500);

      WiFi.begin(wifiConfig.ssid, wifiConfig.password);
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        esp_wifi_set_ps(WIFI_PS_NONE);  // nach Verbindung: Power-Saving aus (sonst ESP-NOW Broadcasts verpasst)
        wifi_mode_t mode; esp_wifi_get_mode(&mode);
        uint8_t ch = 0; wifi_second_chan_t sec; esp_wifi_get_channel(&ch, &sec);
        Serial.printf("\nWiFi verbunden: %s  Mode=%d  Kanal=%d\n",
                      WiFi.localIP().toString().c_str(), (int)mode, ch);
      } else {
        Serial.println("\nWiFi Fehler, Fallback AP");
        startFallbackAP = true;
      }
    }
  }
  if (startFallbackAP) {
    // Sauberer Reset vor dem AP-Start: nach einem gescheiterten STA-Verbindungsversuch
    // kann der WiFi-Treiber sonst in einem Zwischenzustand haengen bleiben, in dem der
    // AP zwar sichtbar ist (Beacons werden gesendet), der WPA2-Handshake aber fehlschlaegt.
    WiFi.mode(WIFI_OFF);
    delay(300);
    WiFi.mode(WIFI_AP);
    delay(300);
    bool apCfgOk = WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    bool apOk    = WiFi.softAP(AP_SSID, AP_PASSWORD, ESPNOW_DEFAULT_CHANNEL);
    Serial.printf("AP: %s  Kanal %d  (Config=%s, Start=%s)\n",
                  WiFi.softAPIP().toString().c_str(), ESPNOW_DEFAULT_CHANNEL,
                  apCfgOk ? "OK" : "FEHLER", apOk ? "OK" : "FEHLER");
  }

  // Ultraschall Trigger/Echo (Echo per Interrupt, nicht-blockierend)
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  attachInterrupt(digitalPinToInterrupt(ULTRASONIC_ECHO), ultraEchoISR, CHANGE);
  Serial.println("Ultraschall Sensor bereit");

  // Ventile
  setupVentile();

  // RS485
  setupRS485();

  // Drucksensor
  analogSetAttenuation(ADC_11db);  // Eingangsbereich 0–3.3V
  pinMode(PRESSURE_PIN, INPUT);

  // DS18B20
  sensors.begin();
  Serial.printf("DS18B20 gefunden: %d Sensor(en)\n", sensors.getDeviceCount());
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);  // requestTemperatures() nicht mehr blockierend (sonst bis 750ms)
  tempConversionDelayMs = sensors.millisToWaitForConversion(12);

  // Webserver
  server.on("/",         handleRoot);
  server.on("/api/data",         handleApiData);
  server.on("/api/espnow",            handleApiEspNow);
  server.on("/api/espnow/relay",     HTTP_POST, handleApiRelaySet);
  server.on("/api/espnow/pwm",      HTTP_POST, handleApiPwmSet);
  server.on("/api/licht",            handleApiLicht);
  server.on("/api/ventile",         handleApiVentilStatus);
  server.on("/api/pcf/test",        HTTP_POST, handleApiPcfTest);
  server.on("/api/rs485",           handleApiRs485);
  server.on("/api/logs",            HTTP_GET,  handleApiLogsList);
  server.on("/api/logs/download",   HTTP_GET,  handleApiLogsDownload);
  server.on("/api/logs/delete",     HTTP_POST, handleApiLogsDelete);
  server.on("/api/ventile/config",  HTTP_POST, handleApiVentilConfig);
  server.on("/api/scheduler",       HTTP_GET,  handleApiSchedulerGet);
  server.on("/api/scheduler/save",  HTTP_POST, handleApiSchedulerSave);
  server.on("/api/co2",             HTTP_GET,  handleApiCo2Get);
  server.on("/api/co2/save",        HTTP_POST, handleApiCo2Save);
  server.on("/api/fan",             HTTP_GET,  handleApiFanGet);
  server.on("/api/fan/save",        HTTP_POST, handleApiFanSave);
  server.on("/api/output",          HTTP_GET,  handleApiOutputGet);
  server.on("/api/output/save",     HTTP_POST, handleApiOutputSave);
  server.on("/api/wifi",            HTTP_GET,  handleApiWifiGet);
  server.on("/api/wifi/save",       HTTP_POST, handleApiWifiSave);
  server.on("/api/tank",            handleApiTankConfig);
  server.on("/api/tank/config",     HTTP_POST, handleApiTankConfigSave);
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html",
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='update' accept='.bin'>"
      "<input type='submit' value='Update'>"
      "</form>");
  });
  server.on("/update", HTTP_POST, handleOTAResult, handleOTAUpdate);
  server.on("/api/ota/upload", HTTP_POST, handleNodeOtaUploadDone, handleNodeOtaUpload);
  server.on("/api/ota/flash",  HTTP_POST, handleNodeOtaFlash);
  server.on("/ota.bin",        HTTP_GET,  handleNodeOtaBin);
  server.begin();
  Serial.println("Webserver gestartet");

  // NTP-Sync erst nach server.begin() — verhindert LWIP-Mutex-Konflikt
  if (WiFi.status() == WL_CONNECTED) {
    syncRtcMitNtp();
  }

  // ESP-NOW (nach WiFi-Init)
  memset(nodeData, 0, sizeof(nodeData));
  setupEspNow();
  espnow_set_sensor_callback(onEspNowSensor);
}

// ========== LOOP ==========

// Nicht-blockierend: ein Sample pro loopDruck()-Aufruf statt delay(2) in einer Schleife.
// starteDruckMessung() startet einen neuen 8-Sample-Mittelwert, loopDruck() treibt ihn voran.
static bool     druckSampling     = false;
static int      druckIdx          = 0;
static long     druckSumme        = 0;
static uint32_t druckLastSampleMs = 0;

void starteDruckMessung() {
  druckSampling     = true;
  druckIdx          = 0;
  druckSumme        = 0;
  druckLastSampleMs = millis() - 2;  // erste Probe sofort im naechsten loopDruck()
}

void loopDruck() {
  if (!druckSampling) return;
  uint32_t now = millis();
  if (now - druckLastSampleMs < 2) return;
  druckLastSampleMs = now;

  druckSumme += analogReadMilliVolts(PRESSURE_PIN);
  druckIdx++;
  if (druckIdx < DRUCK_ADC_SAMPLES) return;

  druckSampling = false;
  int mV = druckSumme / DRUCK_ADC_SAMPLES;

  // Spannung → PSI (linear)
  float psi = (float)(mV - DRUCK_MV_MIN) / (DRUCK_MV_MAX - DRUCK_MV_MIN) * DRUCK_PSI_MAX;
  druckPSI = constrain(psi, 0.0f, (float)DRUCK_PSI_MAX);
  druckBar = druckPSI / 14.5038f;

#ifdef DEBUG_DRUCK
  Serial.printf("Druck: %.1f PSI / %.2f bar (%d mV)\n", druckPSI, druckBar, mV);
#endif
}

// Volumen eines Kegelstumpfs von Boden bis Wasserhöhe h
float berechneVolumen(int h_mm) {
  if (h_mm <= 0) return 0.0;
  float h  = (float)h_mm;
  float H  = (float)tankConfig.hoehe_mm;
  float r1 = (float)tankConfig.radius_unten_mm;
  float r2 = (float)tankConfig.radius_oben_mm;
  float rh = r1 + (r2 - r1) * h / H;
  float vol_mm3 = (float)M_PI * h / 3.0f * (r1*r1 + r1*rh + rh*rh);
  return vol_mm3 / 1000000.0f;
}

// VPD (Vapor Pressure Deficit) nach Tetens-Formel — Server-seitiges Pendant zur JS-Berechnung im UI
float computeVpd(float tempC, float humPct) {
  float svp = 0.6108f * expf((17.27f * tempC) / (tempC + 237.3f));
  return svp * (1.0f - humPct / 100.0f);
}

void triggerUltraschall() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
}

void loop() {
  if (wifiRestartPending && millis() >= wifiRestartAtMs) {
    ESP.restart();
  }

  if (ethJustConnected) {
    ethJustConnected = false;
    if (!phyConfigured) {
      phyConfigured = true;
      w5500_force_phy_100fd();
    }
  }

  server.handleClient();
  loopVentile();
  loopRS485();
  loopEspNow();
  loopScheduler();
  loopCo2Ctrl();
  loopFanCtrl();
  loopDruck();

  if (ota_ap_active && millis() - ota_ap_start > OTA_AP_TIMEOUT) {
    WiFi.softAPdisconnect(true);
    ota_ap_active = false;
    Serial.println("[NODE-OTA] AP Timeout - gestoppt");
  }

  unsigned long now = millis();

  if (!rtcOK && now - lastRtcRetryMs >= RTC_RETRY_INTERVAL_MS) {
    lastRtcRetryMs = now;
    if (rtc.begin()) {
      rtcOK = true;
      Serial.println("RTC: jetzt erkannt (fehlgeschlagener Init-Versuch beim Boot hat sich erholt)");
    }
  }

  if (now - lastNtpSync >= NTP_SYNC_INTERVAL) {
    syncRtcMitNtp();
  }

  if (now - lastTempRead >= MEASUREMENT_INTERVAL) {
    lastTempRead = now;
    sensors.requestTemperatures();  // nicht-blockierend, siehe setWaitForConversion(false)
    tempConversionStartMs = now;
    tempConversionPending = true;
    starteDruckMessung();

    if (aht21Ok) {
      sensors_event_t humEvt, tempEvt;
      aht21.getEvent(&humEvt, &tempEvt);
      aht21Temp = tempEvt.temperature;
      aht21Hum  = humEvt.relative_humidity;
      Serial.printf("[AHT21] %.1f°C  %.1f%% rH\n", aht21Temp, aht21Hum);
    }
  }

  if (tempConversionPending && (now - tempConversionStartMs >= tempConversionDelayMs)) {
    tempConversionPending = false;
    tempVorrat  = sensors.getTempCByIndex(0);
    tempPflanze = sensors.getTempCByIndex(1);

    if (rtcOK && storage_is_ok() && now - lastLogMs >= LOG_INTERVAL_MS) {
      lastLogMs = now;
      LogEntry e{};
      e.tempVorrat        = tempVorrat;
      e.tempPflanze       = tempPflanze;
      e.druckBar          = druckBar;
      e.fuellstandProzent = fuellstandProzent;
      e.volumenLiter      = volumenLiter;
      e.raumOk            = aht21Ok;
      e.raumTemp          = aht21Temp;
      e.raumFeuchte       = aht21Hum;
      const Rs485SensorData& ms2 = rs485_get_ms2();
      e.zeltOk      = ms2.online;
      e.zeltTemp    = ms2.temp_c;
      e.zeltFeuchte = ms2.hum_pct;
      e.zeltCo2     = ms2.co2_ppm;
      e.zeltVpd     = ms2.online ? computeVpd(ms2.temp_c, ms2.hum_pct) : 0.0f;
      e.lichtMaske  = rs485_get_licht().mask;
      const Rs485FanData& fan = rs485_get_fan();
      e.fanOk       = fan.online;
      e.fanPercent  = fan.percent;
      e.fanRpm      = fan.rpm;
      e.co2Ausgang  = co2OutputOn;
      storage_log(rtc.now().unixtime(), e);
    }
    #ifdef DEBUG_DISTANZ
    Serial.printf("Vorrat: %.1f°C  Pflanze: %.1f°C\n", tempVorrat, tempPflanze);
    #endif
  }

  if (now - lastUltraRead >= 500) {
    lastUltraRead    = now;
    ultraEchoReady   = false;
    ultraWaitingEcho = true;
    ultraTriggerMs   = now;
    triggerUltraschall();
  }

  if (ultraWaitingEcho && ultraEchoReady) {
    portENTER_CRITICAL(&ultraMux);
    uint32_t durUs = ultraEchoDurationUs;
    ultraEchoReady = false;
    portEXIT_CRITICAL(&ultraMux);
    ultraWaitingEcho = false;

    int distCm = (int)(durUs * 0.03557f / 2);
    if (distCm > 0) {
      int rohmm = distCm * 10 + tankConfig.offset_mm;

      if (rohmm < tankConfig.min_mm) {
        distanzMM         = tankConfig.min_mm;
        wasserHoeheMM     = tankConfig.hoehe_mm;
        fuellstandProzent = 100;
        volumenLiter      = berechneVolumen(tankConfig.hoehe_mm);
      } else if (rohmm > tankConfig.max_mm) {
        // ungültiger Wert, ignorieren
      } else {
        distanzMM         = rohmm;
        wasserHoeheMM     = constrain((int)tankConfig.leer_abstand_mm - distanzMM, 0, (int)tankConfig.hoehe_mm);
        fuellstandProzent = wasserHoeheMM * 100 / tankConfig.hoehe_mm;
        volumenLiter      = berechneVolumen(wasserHoeheMM);
      }

#ifdef DEBUG_DISTANZ
      Serial.printf("Distanz: %d mm  Wasser: %d mm  Füllstand: %d%%  Volumen: %.1f L\n",
                    distanzMM, wasserHoeheMM, fuellstandProzent, volumenLiter);
#endif
    }
  } else if (ultraWaitingEcho && (now - ultraTriggerMs > ULTRA_ECHO_TIMEOUT_MS)) {
    ultraWaitingEcho = false;  // kein Echo erhalten - naechster Versuch in 500ms
  }
}
