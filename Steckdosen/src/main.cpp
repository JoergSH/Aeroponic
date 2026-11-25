#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include "espnow_protocol.h"
#include "espnow_slave.h"
#include "relay/relay_manager.h"

#define STATUS_INTERVAL_MS  30000UL
#define WDT_TIMEOUT_MS              30000
#define BOOT_BUTTON_PIN             9                   // BOOT-Taster der ESP32-C3 Super Mini (LOW = gedrueckt)
#define UNPAIR_HOLD_MS              3000                // so lange im Betrieb halten -> Kopplung loesen

static const uint8_t my_node_type = NODE_TYPE_SOCKET;
static bool     online        = false;
static uint32_t last_status_ms = 0;
static Preferences prefs;

// ========== Node-ID und Funkkanal ==========
// Kein serielles Setup mehr: den Kanal sucht espnow_slave_begin() selbst (Kanal des Masters),
// die Node-ID vergibt der Master beim Koppeln im Webinterface. Bisher gespeicherte ID
// (nodecfg/id) und Kanal (wificfg/channel) werden uebernommen. Der Typ ist immer
// NODE_TYPE_SOCKET -- die fruehere Variante "Licht-SSR" ist durch das Lichtx4-Modul ersetzt.

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

    espnow_slave_set_command_cb(on_command);
    espnow_slave_begin(my_node_type, NODE_ID_UNASSIGNED);

    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Haengt loop() laenger als WDT_TIMEOUT_MS, startet der Node neu -- die Relais stellt
    // relay_manager_init() danach aus dem NVS wieder her.
    esp_task_wdt_config_t wdt_cfg = {};
    wdt_cfg.timeout_ms     = WDT_TIMEOUT_MS;
    wdt_cfg.trigger_panic  = true;
    if (esp_task_wdt_reconfigure(&wdt_cfg) != ESP_OK) esp_task_wdt_init(&wdt_cfg);
    enableLoopWDT();
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
        } else if (cmd == "NODEINFO") {
            Serial.printf("[NODE] Steckdose (0x%02X)  ID=%u  Kanal %u  gekoppelt=%s  %s\n", my_node_type,
                          espnow_slave_get_node_id(), espnow_slave_get_channel(),
                          espnow_slave_is_paired() ? "ja" : "nein",
                          espnow_slave_is_online() ? "online" : (espnow_slave_is_hunting() ? "sucht Master" : "meldet an"));
        } else if (cmd == "UNPAIR") {
            espnow_slave_unpair();
        } else if (cmd.length() > 0) {
            Serial.println("[CMD] STATUS  MASK <0-15>  R<1-4> <0|1>");
            Serial.println("      NODEINFO  UNPAIR");
        }
    }

    // BOOT-Taster 3 s im Betrieb halten -> Kopplung loesen (beim Einschalten gedrueckt
    // startet der C3 dagegen in den Download-Modus, das bleibt davon unberuehrt).
    {
        static uint32_t pressed_since = 0;
        static bool     done          = false;
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            if (pressed_since == 0) pressed_since = millis();
            if (!done && millis() - pressed_since >= UNPAIR_HOLD_MS) {
                done = true;
                Serial.println("[MAIN] BOOT-Taster 3 s gehalten -> Kopplung loesen");
                espnow_slave_unpair();
            }
        } else {
            pressed_since = 0;
            done          = false;
        }
    }

    espnow_slave_loop();

    if (!online && espnow_slave_is_online()) {
        online = true;
        Serial.println("[MAIN] *** ONLINE beim Master registriert! ***");
        send_status();
        last_status_ms = millis();
    } else if (online && !espnow_slave_is_online()) {
        online = false;
        Serial.println("[MAIN] Verbindung zum Master verloren");
    }
    if (online) {
        uint32_t now = millis();
        if (now - last_status_ms >= STATUS_INTERVAL_MS) {
            last_status_ms = now;
            send_status();
        }
    }
}
