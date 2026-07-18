#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>

#include "driver/gpio.h"
#include "espnow_slave.h"
#include "espnow_protocol.h"
#include "sensors/sensor_manager.h"
#include "sensors/scd41_handler.h"
#include "rs485_slave.h"

#define I2C_SDA    1
#define I2C_SCL    0
#define RS485_TX   3
#define RS485_RX   5

#ifndef MY_NODE_ID
#define MY_NODE_ID  0x02
#endif
#define MY_NODE_TYPE  NODE_TYPE_SENSOR

#define ERR_SENSOR_INIT  0x01
#define ERR_SCD41_FAIL   0x02

typedef enum {
    DEV_REGISTERING = 0,
    DEV_ONLINE,
    DEV_ERROR
} device_state_t;

static device_state_t dev_state      = DEV_REGISTERING;
static bool           sensor_init_ok = false;
static bool           error_sent     = false;
static bool           use_espnow     = false;
static Preferences    prefs;

// ========== WiFi-Kanal (inline) ==========

static uint8_t channel_setup() {
    prefs.begin("wificfg", true);
    String ssid = prefs.getString("ssid", "");
    uint8_t ch  = prefs.getUChar("channel", 0);
    prefs.end();

    if (ch > 0 && !ssid.isEmpty()) {
        Serial.printf("[WIFI] Kanal %d (SSID: '%s')\n", ch, ssid.c_str());
        return ch;
    }
    if (ch > 0 && ssid.isEmpty()) {
        Serial.printf("[WIFI] Kein WLAN, fester Kanal %d\n", ch);
        return ch;
    }

    Serial.println("[WIFI] Kein Kanal gespeichert, scanne...");
    int n = WiFi.scanNetworks();
    Serial.printf("[WIFI] %d Netzwerke gefunden\n", n);

    if (n <= 0) {
        Serial.printf("[WIFI] Keine Netzwerke, Fallback Kanal %d\n", ESPNOW_DEFAULT_CHANNEL);
        WiFi.scanDelete();
        return ESPNOW_DEFAULT_CHANNEL;
    }

    for (int i = 0; i < n; i++)
        Serial.printf("  [%2d] %-32s  Kanal %2d  %d dBm\n",
                      i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
    Serial.printf("  [ 0] Kein WLAN (fester Kanal %d)\n", ESPNOW_DEFAULT_CHANNEL);
    Serial.printf("Nummer (0-%d) + Enter (60s Timeout):\n", n);
    while (Serial.available()) Serial.read();

    uint32_t t0 = millis();
    while (!Serial.available()) {
        if (millis() - t0 > 60000) {
            Serial.printf("[WIFI] Timeout, Fallback Kanal %d\n", ESPNOW_DEFAULT_CHANNEL);
            WiFi.scanDelete();
            return ESPNOW_DEFAULT_CHANNEL;
        }
        delay(50);
    }

    int choice = Serial.readStringUntil('\n').toInt();

    if (choice == 0) {
        WiFi.scanDelete();
        prefs.begin("wificfg", false);
        prefs.putString("ssid", "");
        prefs.putUChar("channel", ESPNOW_DEFAULT_CHANNEL);
        prefs.end();
        Serial.printf("[WIFI] Kein WLAN, Kanal %d gespeichert\n", ESPNOW_DEFAULT_CHANNEL);
        return ESPNOW_DEFAULT_CHANNEL;
    }

    if (choice < 1 || choice > n) {
        Serial.printf("[WIFI] Ungueltige Auswahl, Fallback Kanal %d\n", ESPNOW_DEFAULT_CHANNEL);
        WiFi.scanDelete();
        return ESPNOW_DEFAULT_CHANNEL;
    }

    String  chosen_ssid = WiFi.SSID(choice - 1);
    uint8_t chosen_ch   = (uint8_t)WiFi.channel(choice - 1);
    WiFi.scanDelete();

    prefs.begin("wificfg", false);
    prefs.putString("ssid", chosen_ssid);
    prefs.putUChar("channel", chosen_ch);
    prefs.end();

    Serial.printf("[WIFI] Gespeichert: '%s' Kanal %d\n", chosen_ssid.c_str(), chosen_ch);
    return chosen_ch;
}

// ========== Sensordaten senden ==========

static void send_sensor_data(const sensor_data_t& d) {
    uint8_t p1[7];
    p1[0] = SUBTYPE_AIR;
    p1[1] = (uint8_t)(d.temperature_cdeg >> 8);
    p1[2] = (uint8_t)(d.temperature_cdeg & 0xFF);
    p1[3] = (uint8_t)(d.humidity_cpct >> 8);
    p1[4] = (uint8_t)(d.humidity_cpct & 0xFF);
    p1[5] = (uint8_t)(d.co2_ppm >> 8);
    p1[6] = (uint8_t)(d.co2_ppm & 0xFF);
    Serial.printf("[TX] T=%.2f°C  H=%.2f%%  CO2=%u ppm  PPFD=%.1f\n",
                  d.temperature_cdeg / 100.0f, d.humidity_cpct / 100.0f,
                  d.co2_ppm, d.ppfd_dppfd / 10.0f);
    espnow_slave_send_data(p1, sizeof(p1));

    // AS7341 Spektrum
    uint16_t chRaw[8] = {d.ch_415nm, d.ch_445nm, d.ch_480nm, d.ch_515nm,
                         d.ch_555nm, d.ch_590nm, d.ch_630nm, d.ch_680nm};
    uint16_t maxCh = 0;
    for (int i = 0; i < 8; i++) if (chRaw[i] > maxCh) maxCh = chRaw[i];

    uint8_t p2[12];
    p2[0] = SUBTYPE_LIGHT;
    p2[1] = (uint8_t)(d.ppfd_dppfd >> 8);
    p2[2] = (uint8_t)(d.ppfd_dppfd & 0xFF);
    p2[3] = d.as7341_gain;
    for (int i = 0; i < 8; i++)
        p2[4+i] = (maxCh > 0) ? (uint8_t)((uint32_t)chRaw[i] * 255 / maxCh) : 0;
    espnow_slave_send_data(p2, 12);
}

// ========== Callbacks ==========

static void on_command(const uint8_t* payload, uint8_t len) {
    sensor_manager_on_command(payload, len);
}

// ========== I2C ==========

static void i2c_recover() {
    pinMode(I2C_SDA, INPUT_PULLUP);
    pinMode(I2C_SCL, INPUT_PULLUP);
    delay(5);
    if (digitalRead(I2C_SDA) == HIGH) {
        Serial.println("[I2C] Bus frei");
        return;
    }
    Serial.println("[I2C] Bus haengt — Recovery...");
    pinMode(I2C_SCL, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(I2C_SCL, LOW);  delayMicroseconds(10);
        digitalWrite(I2C_SCL, HIGH); delayMicroseconds(10);
        if (digitalRead(I2C_SDA) == HIGH) break;
    }
    pinMode(I2C_SDA, OUTPUT);
    digitalWrite(I2C_SDA, LOW);  delayMicroseconds(10);
    digitalWrite(I2C_SCL, HIGH); delayMicroseconds(10);
    digitalWrite(I2C_SDA, HIGH); delayMicroseconds(10);
    pinMode(I2C_SDA, INPUT_PULLUP);
    pinMode(I2C_SCL, INPUT_PULLUP);
    delay(10);
}

static void i2c_scan() {
    Serial.printf("[I2C] Scan (SDA=%d SCL=%d)...\n", I2C_SDA, I2C_SCL);
    const struct { uint8_t addr; const char* name; } known[] = {
        { 0x39, "AS7341" },
        { 0x62, "SCD41" },
    };
    uint8_t found = 0;
    for (auto& dev : known) {
        Wire.beginTransmission(dev.addr);
        uint8_t err = Wire.endTransmission();
        Serial.printf("  0x%02X %-24s -> %s\n", dev.addr, dev.name,
                      err == 0 ? "OK" : (err == 5 ? "TIMEOUT" : "nicht gefunden"));
        if (err == 0) found++;
    }
    Serial.printf("[I2C] %d Geraet(e) gefunden\n", found);
}

// ========== Setup / Loop ==========

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[MAIN] Multisensor2 Node 0x02 startet...");

    gpio_reset_pin(GPIO_NUM_5);
    rs485_slave_init(RS485_TX, RS485_RX);
    i2c_recover();
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(50000);
    Wire.setTimeOut(500);
    delay(200);
    i2c_scan();

    sm_init_result_t res = sensor_manager_init(I2C_SDA, I2C_SCL);
    sensor_init_ok = (res == SM_OK);

    // WiFi vorab starten — stabilisiert APB-Takt für UART
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    // RS485-Master abwarten — kommt ein Poll, kein ESP-NOW nötig
    Serial.println("[MAIN] Warte auf RS485 Master (max 20s)...");
    uint32_t t0 = millis();
    while (millis() - t0 < 20000) {
        rs485_slave_loop();
        sensor_manager_loop();
        if (rs485_slave_got_request()) break;
        delay(5);
    }

    if (rs485_slave_got_request()) {
        Serial.println("[MAIN] RS485 Master erkannt — ESP-NOW deaktiviert");
        WiFi.mode(WIFI_OFF);
        use_espnow = false;
        dev_state  = DEV_ONLINE;
    } else {
        Serial.println("[MAIN] Kein RS485 Master — starte ESP-NOW...");
        use_espnow = true;
        WiFi.mode(WIFI_STA);
        uint8_t channel = channel_setup();
        espnow_slave_set_command_cb(on_command);
        espnow_slave_init(MY_NODE_ID, MY_NODE_TYPE, channel);
        dev_state = DEV_REGISTERING;
    }
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "WIFICLEAR") {
            prefs.begin("wificfg", false);
            prefs.clear();
            prefs.end();
            Serial.println("[WIFI] Geloescht, starte neu...");
            delay(300);
            ESP.restart();
        } else if (cmd == "WIFIINFO") {
            prefs.begin("wificfg", true);
            Serial.printf("[WIFI] SSID: '%s'  Kanal: %d\n",
                          prefs.getString("ssid", "").c_str(),
                          prefs.getUChar("channel", 0));
            prefs.end();
        }
    }

    if (use_espnow) espnow_slave_loop();
    rs485_slave_loop();

    switch (dev_state) {
        case DEV_REGISTERING:
            if (use_espnow) {
                if (espnow_slave_get_state() == SLAVE_STATE_ERROR) {
                    dev_state = DEV_ERROR;
                } else if (espnow_slave_is_online()) {
                    if (!sensor_init_ok) {
                        espnow_slave_send_error(ERR_SENSOR_INIT);
                        dev_state = DEV_ERROR;
                    } else {
                        Serial.println("[MAIN] Online → Messzyklus startet");
                        dev_state = DEV_ONLINE;
                    }
                }
            }
            break;

        case DEV_ONLINE:
            sensor_manager_loop();
            if (sensor_manager_data_ready()) {
                sensor_data_t d;
                sensor_manager_get_data(&d);
                if (use_espnow) {
                    if (scd41_has_error() && !error_sent) {
                        espnow_slave_send_error(ERR_SCD41_FAIL);
                        error_sent = true;
                    } else {
                        error_sent = false;
                        send_sensor_data(d);
                    }
                }
                uint8_t ch[8] = {
                    (uint8_t)d.ch_415nm, (uint8_t)d.ch_445nm,
                    (uint8_t)d.ch_480nm, (uint8_t)d.ch_515nm,
                    (uint8_t)d.ch_555nm, (uint8_t)d.ch_590nm,
                    (uint8_t)d.ch_630nm, (uint8_t)d.ch_680nm
                };
                rs485_slave_update(d.co2_ppm, d.temperature_cdeg, d.humidity_cpct,
                                   d.ppfd_dppfd, ch, d.as7341_gain,
                                   !scd41_has_error(), true);
            }
            break;

        case DEV_ERROR:
            break;
    }
}
