#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include "espnow_protocol.h"
#include "espnow_slave.h"
#include "relay/relay_manager.h"

#define STATUS_INTERVAL_MS  30000UL
#define CHANNEL_RECHECK_INTERVAL_MS (12UL * 3600000UL)  // 12h -- siehe recheck_channel_if_needed()
#define OFFLINE_RECHECK_MS          (2UL * 60000UL)     // so lange offline -> Kanal sofort pruefen
#define OFFLINE_RECHECK_INTERVAL_MS (5UL * 60000UL)     // ...und danach hoechstens alle 5 min erneut
#define WDT_TIMEOUT_MS              30000               // Kanal-Scan blockiert loop() ein paar Sekunden
#define BOOT_BUTTON_PIN             9                   // BOOT-Taster der ESP32-C3 Super Mini (LOW = gedrueckt)
#define UNPAIR_HOLD_MS              3000                // so lange im Betrieb halten -> Kopplung loesen

static uint8_t  my_node_id    = 0x03;
static const uint8_t my_node_type = NODE_TYPE_SOCKET;
static bool     online        = false;
static uint32_t last_status_ms = 0;
static Preferences prefs;

// ========== Node-ID (Erststart) ==========
// Der Typ ist immer NODE_TYPE_SOCKET -- die fruehere Variante "Licht-SSR" ist durch das
// Lichtx4-Modul (RS485) ersetzt. Ein noch gespeicherter alter Typ wird ignoriert.

static void node_setup() {
    prefs.begin("nodecfg", true);
    uint8_t stored_id = prefs.getUChar("id", 0xFF);
    prefs.end();

    if (stored_id != 0xFF) {
        my_node_id = stored_id;
        Serial.printf("[NODE] Steckdose  ID=0x%02X\n", stored_id);
        return;
    }

    // Erststart: interaktive Abfrage
    Serial.println("\n=== ERSTSTART: Node-Konfiguration ===");
    Serial.println("Node-ID eingeben (1-254) + Enter (60s Timeout, sonst 3):");
    while (Serial.available()) Serial.read();
    uint32_t t0 = millis();
    while (!Serial.available() && millis() - t0 < 60000) delay(50);
    uint8_t id = 0x03;
    if (Serial.available()) {
        int v = Serial.readStringUntil('\n').toInt();
        id = (uint8_t)constrain(v, 1, 254);
    }

    prefs.begin("nodecfg", false);
    prefs.remove("type");
    prefs.putUChar("id", id);
    prefs.end();

    my_node_id = id;
    Serial.printf("[NODE] Gespeichert: Steckdose  ID=0x%02X\n", id);
}

// ========== WiFi-Kanal ==========

static uint8_t channel_setup() {
    prefs.begin("wificfg", true);
    String ssid = prefs.getString("ssid", "");
    uint8_t ch  = prefs.getUChar("channel", 0);
    prefs.end();

    if (ch > 0 && !ssid.isEmpty()) {
        // Router kann seinen Kanal jederzeit wechseln (Auto-Channel, Neustart nach
        // Stromausfall, ...) -- der gespeicherte Wert waere dann dauerhaft falsch, ohne dass
        // das je auffaellt (der Node haette einfach keine Verbindung zum Master mehr, siehe
        // "Node meldet sich nicht an"). Deshalb bei jedem Boot still im Hintergrund pruefen
        // statt dem gespeicherten Kanal blind zu vertrauen -- kein Serial-Prompt noetig, die
        // SSID ist ja schon bekannt.
        Serial.printf("[WIFI] Gespeichert: Kanal %d (SSID: '%s') -- verifiziere...\n", ch, ssid.c_str());
        int n = WiFi.scanNetworks();
        uint8_t found_ch = 0;
        for (int i = 0; i < n; i++) {
            if (ssid.equals(WiFi.SSID(i))) { found_ch = (uint8_t)WiFi.channel(i); break; }
        }
        WiFi.scanDelete();
        if (found_ch > 0 && found_ch != ch) {
            Serial.printf("[WIFI] Router-Kanal geaendert: %d -> %d, aktualisiere\n", ch, found_ch);
            prefs.begin("wificfg", false);
            prefs.putUChar("channel", found_ch);
            prefs.end();
            return found_ch;
        }
        if (found_ch == 0) {
            Serial.println("[WIFI] SSID im Scan nicht gefunden, nutze gespeicherten Kanal weiter");
        } else {
            Serial.println("[WIFI] Kanal stimmt noch");
        }
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

// Ergaenzt die Boot-Pruefung in channel_setup(): falls der Router den Kanal waehrend des
// laufenden Betriebs wechselt (Node selbst laeuft ja oft monatelang durch, ohne neu zu
// starten), wuerde das sonst erst beim naechsten Reboot auffallen. Scan stoert ESP-NOW kurz
// (Kanal-Hopping), daher nur selten pruefen.
// force: sofort pruefen (Node ist schon laenger offline -- haeufigste Ursache ist ein
// Kanalwechsel des Routers, den die 12-h-Pruefung erst viel spaeter bemerken wuerde).
static void recheck_channel_if_needed(bool force = false) {
    static uint32_t last_check_ms = 0;
    uint32_t now = millis();
    if (!force && now - last_check_ms < CHANNEL_RECHECK_INTERVAL_MS) return;
    last_check_ms = now;

    prefs.begin("wificfg", true);
    String  ssid      = prefs.getString("ssid", "");
    uint8_t stored_ch = prefs.getUChar("channel", 0);
    prefs.end();
    if (ssid.isEmpty()) return;  // kein WLAN konfiguriert (fester Kanal) -- nichts zu pruefen

    Serial.println("[WIFI] Periodische Kanal-Pruefung...");
    int n = WiFi.scanNetworks();
    uint8_t found_ch = 0;
    for (int i = 0; i < n; i++) {
        if (ssid.equals(WiFi.SSID(i))) { found_ch = (uint8_t)WiFi.channel(i); break; }
    }
    WiFi.scanDelete();

    if (found_ch > 0 && found_ch != stored_ch) {
        Serial.printf("[WIFI] Router-Kanal hat sich waehrend des Betriebs geaendert (%d -> %d) -- starte neu\n",
                      stored_ch, found_ch);
        prefs.begin("wificfg", false);
        prefs.putUChar("channel", found_ch);
        prefs.end();
        delay(300);
        ESP.restart();
    }

    // Der Scan selbst hat den Funkkanal verstellt, auch wenn sich nichts geaendert hat --
    // zurueck auf den ESP-NOW-Betriebskanal, sonst bleibt der Node bis zum naechsten Reboot
    // auf dem zuletzt gescannten Kanal haengen und faellt scheinbar grundlos offline.
    esp_wifi_set_channel(stored_ch, WIFI_SECOND_CHAN_NONE);
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

    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Watchdog erst nach dem Setup (Erststart-Abfrage wartet bis zu 60 s auf Eingabe).
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
            Serial.printf("[NODE] Steckdose (0x%02X)  ID=0x%02X  gekoppelt=%s\n", my_node_type,
                          my_node_id, espnow_slave_is_paired() ? "ja" : "nein");
        } else if (cmd == "UNPAIR") {
            espnow_slave_unpair();
        } else if (cmd.length() > 0) {
            Serial.println("[CMD] STATUS  MASK <0-15>  R<1-4> <0|1>");
            Serial.println("      WIFICLEAR  WIFIINFO  NODECLEAR  NODEINFO  UNPAIR");
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

    static uint32_t offline_since_ms   = 0;
    static uint32_t last_offline_check = 0;
    if (!online && espnow_slave_is_online()) {
        online = true;
        Serial.println("[MAIN] *** ONLINE beim Master registriert! ***");
        send_status();
        last_status_ms = millis();
    } else if (online && !espnow_slave_is_online()) {
        online = false;
        offline_since_ms = millis();
        Serial.println("[MAIN] Verbindung zum Master verloren");
    }
    if (!online) {
        uint32_t now = millis();
        if (offline_since_ms == 0) offline_since_ms = now;   // nie online gewesen (Boot)
        if (now - offline_since_ms >= OFFLINE_RECHECK_MS &&
            (last_offline_check == 0 || now - last_offline_check >= OFFLINE_RECHECK_INTERVAL_MS)) {
            last_offline_check = now;
            recheck_channel_if_needed(true);   // startet bei geaendertem Kanal neu
        }
    } else {
        last_offline_check = 0;
    }

    if (online) {
        uint32_t now = millis();
        if (now - last_status_ms >= STATUS_INTERVAL_MS) {
            last_status_ms = now;
            send_status();
        }
        recheck_channel_if_needed();
    }
}
