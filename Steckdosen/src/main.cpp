#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include "espnow_protocol.h"
#include "espnow_slave.h"
#include "relay/relay_manager.h"

#define STATUS_INTERVAL_MS  30000UL

static uint8_t  my_node_id    = 0x03;
static uint8_t  my_node_type  = NODE_TYPE_SOCKET;
static bool     online        = false;
static uint32_t last_status_ms = 0;
static Preferences prefs;

// ========== Node-Typ und ID (Erststart) ==========

static void node_setup() {
    prefs.begin("nodecfg", true);
    uint8_t stored_type = prefs.getUChar("type", 0xFF);
    uint8_t stored_id   = prefs.getUChar("id",   0xFF);
    prefs.end();

    if (stored_type != 0xFF && stored_id != 0xFF) {
        my_node_type = stored_type;
        my_node_id   = stored_id;
        const char* tname = (stored_type == NODE_TYPE_LIGHT_SSR) ? "Licht-SSR" : "Steckdose";
        Serial.printf("[NODE] Typ=%s (0x%02X)  ID=0x%02X\n", tname, stored_type, stored_id);
        return;
    }

    // Erststart: interaktive Abfrage
    Serial.println("\n=== ERSTSTART: Node-Konfiguration ===");
    Serial.println("Typ waehlen (60s Timeout):");
    Serial.println("  1 = Steckdose  (allgemeine Steuerung)");
    Serial.println("  2 = Licht-SSR  (Sonnenauf/-untergang via Scheduler)");
    while (Serial.available()) Serial.read();

    uint32_t t0 = millis();
    while (!Serial.available() && millis() - t0 < 60000) delay(50);
    int typ_choice = Serial.available() ? Serial.readStringUntil('\n').toInt() : 1;
    uint8_t type = (typ_choice == 2) ? NODE_TYPE_LIGHT_SSR : NODE_TYPE_SOCKET;

    Serial.println("Node-ID eingeben (1-254) + Enter:");
    while (Serial.available()) Serial.read();
    t0 = millis();
    while (!Serial.available() && millis() - t0 < 60000) delay(50);
    uint8_t id = 0x03;
    if (Serial.available()) {
        int v = Serial.readStringUntil('\n').toInt();
        id = (uint8_t)constrain(v, 1, 254);
    }

    prefs.begin("nodecfg", false);
    prefs.putUChar("type", type);
    prefs.putUChar("id",   id);
    prefs.end();

    my_node_type = type;
    my_node_id   = id;
    const char* tname = (type == NODE_TYPE_LIGHT_SSR) ? "Licht-SSR" : "Steckdose";
    Serial.printf("[NODE] Gespeichert: Typ=%s  ID=0x%02X\n", tname, id);
}

// ========== WiFi-Kanal ==========

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

// ========== Status senden ==========

static void send_status() {
    uint8_t mask = relay_manager_get_mask();
    uint8_t payload[2];
    payload[0] = SUBTYPE_RELAY_STATUS;
    payload[1] = mask;
    espnow_slave_send_data(payload, 2);
    Serial.printf("[TX] Relais-Status: 0x%02X  (R1=%d R2=%d R3=%d R4=%d)\n",
                  mask,
                  (mask >> 0) & 1, (mask >> 1) & 1,
                  (mask >> 2) & 1, (mask >> 3) & 1);
}

// ========== Befehle vom Master ==========

static void on_command(const uint8_t* payload, uint8_t len) {
    if (len < 2) return;
    if (payload[0] == CMD_SET_ONOFF) {
        relay_manager_set_mask(payload[1]);
        send_status();
    }
}

// ========== Setup / Loop ==========

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[MAIN] Steckdosen2-Node startet...");

    relay_manager_init();

    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.printf("[MAIN] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    node_setup();

    uint8_t channel = channel_setup();
    Serial.printf("[MAIN] Kanal %d  ID: 0x%02X  Typ: 0x%02X\n",
                  channel, my_node_id, my_node_type);

    espnow_slave_set_command_cb(on_command);
    espnow_slave_init(my_node_id, my_node_type, channel);
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "STATUS") {
            uint8_t mask = relay_manager_get_mask();
            Serial.printf("[RELAY] Maske: 0x%02X  R1=%d R2=%d R3=%d R4=%d\n",
                          mask,
                          (mask >> 0) & 1, (mask >> 1) & 1,
                          (mask >> 2) & 1, (mask >> 3) & 1);
        } else if (cmd.startsWith("MASK ")) {
            int val = cmd.substring(5).toInt();
            relay_manager_set_mask((uint8_t)(val & 0x0F));
            if (online) send_status();
        } else if (cmd.startsWith("R") && cmd.length() >= 4) {
            int ch  = cmd.charAt(1) - '0';
            int val = cmd.charAt(3) - '0';
            if (ch >= 1 && ch <= 4 && (val == 0 || val == 1)) {
                uint8_t mask = relay_manager_get_mask();
                if (val) mask |=  (1 << (ch - 1));
                else     mask &= ~(1 << (ch - 1));
                relay_manager_set_mask(mask);
                if (online) send_status();
            } else {
                Serial.println("[CMD] Syntax: R<1-4> <0|1>  z.B. R1 1");
            }
        } else if (cmd == "WIFICLEAR") {
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
        } else if (cmd == "NODECLEAR") {
            prefs.begin("nodecfg", false);
            prefs.clear();
            prefs.end();
            Serial.println("[NODE] Geloescht, starte neu...");
            delay(300);
            ESP.restart();
        } else if (cmd == "NODEINFO") {
            const char* tname = (my_node_type == NODE_TYPE_LIGHT_SSR) ? "Licht-SSR" : "Steckdose";
            Serial.printf("[NODE] Typ=%s (0x%02X)  ID=0x%02X\n", tname, my_node_type, my_node_id);
        } else if (cmd.length() > 0) {
            Serial.println("[CMD] STATUS  MASK <0-15>  R<1-4> <0|1>");
            Serial.println("      WIFICLEAR  WIFIINFO  NODECLEAR  NODEINFO");
        }
    }

    espnow_slave_loop();

    if (!online && espnow_slave_is_online()) {
        online = true;
        Serial.println("[MAIN] *** ONLINE beim Master registriert! ***");
        send_status();
        last_status_ms = millis();
    }

    if (online) {
        uint32_t now = millis();
        if (now - last_status_ms >= STATUS_INTERVAL_MS) {
            last_status_ms = now;
            send_status();
        }
    }
}
