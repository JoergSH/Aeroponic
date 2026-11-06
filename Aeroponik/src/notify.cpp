#include "notify.h"
#include "config.h"
#include "dbg.h"
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <base64.h>
#include <string.h>

NotifyConfig notifyConfig;
NotifyExtraConfig notifyExtra;

#define NOTIFY_RATE_LIMIT_MS 30000UL   // Mindestabstand zwischen zwei Nachrichten (pro Kanal, siehe je Sendefunktion)

// Gemeinsames URL-Encoding fuer WhatsApp (CallMeBot) und Telegram — beides schlichte
// HTTPS-GET-APIs mit dem Nachrichtentext als Query-Parameter. "%" zuerst kodieren, sonst
// werden die eigenen %XX-Sequenzen unten doppelt kodiert.
static String urlEncodeText(const String& message) {
    String encoded = message;
    encoded.replace("%", "%25");
    encoded.replace("\n", "%0A");
    encoded.replace("&", "%26");
    encoded.replace(" ", "+");
    encoded.replace("ä", "%C3%A4"); encoded.replace("ö", "%C3%B6"); encoded.replace("ü", "%C3%BC");
    encoded.replace("ß", "%C3%9F");
    encoded.replace("Ä", "%C3%84"); encoded.replace("Ö", "%C3%96"); encoded.replace("Ü", "%C3%9C");
    return encoded;
}

void loadNotifyConfig() {
    if (EEPROM.read(EEPROM_NOTIFY_MAGIC_ADDR) == EEPROM_NOTIFY_MAGIC_BYTE) {
        EEPROM.get(EEPROM_NOTIFY_BASE, notifyConfig);
    } else {
        memset(&notifyConfig, 0, sizeof(notifyConfig));
        notifyConfig.feuchte_min      = NOTIFY_FEUCHTE_MIN_DEFAULT;
        notifyConfig.feuchte_max      = NOTIFY_FEUCHTE_MAX_DEFAULT;
        notifyConfig.temperatur_min   = NOTIFY_TEMPERATUR_MIN_DEFAULT;
        notifyConfig.temperatur_max   = NOTIFY_TEMPERATUR_MAX_DEFAULT;
        notifyConfig.tank_min_prozent = NOTIFY_TANK_MIN_PROZENT_DEFAULT;
        saveNotifyConfig();
    }
}

void saveNotifyConfig() {
    EEPROM.put(EEPROM_NOTIFY_BASE, notifyConfig);
    EEPROM.write(EEPROM_NOTIFY_MAGIC_ADDR, EEPROM_NOTIFY_MAGIC_BYTE);
    EEPROM.commit();
}

void sendWhatsApp(const String& message) {
    if (!notifyConfig.global_enabled) {
        dbgPrintln("[Notify] Benachrichtigungen deaktiviert");
        return;
    }
    if (!notifyExtra.whatsapp_enabled) {
        dbgPrintln("[Notify] WhatsApp-Kanal deaktiviert");
        return;
    }
    if (strlen(notifyConfig.phone) == 0 || strlen(notifyConfig.apikey) == 0) {
        dbgPrintln("[Notify] Telefonnummer/API-Key nicht konfiguriert");
        return;
    }
    static unsigned long lastSendMs = 0;
    unsigned long now = millis();
    if (now - lastSendMs < NOTIFY_RATE_LIMIT_MS) {
        dbgPrintln("[Notify] WhatsApp Rate-Limit, Nachricht verworfen");
        return;
    }
    lastSendMs = now;

    String url = "https://api.callmebot.com/whatsapp.php?phone=" + String(notifyConfig.phone) +
                 "&text=" + urlEncodeText(message) + "&apikey=" + String(notifyConfig.apikey);

    dbgPrintf("[Notify] Sende: %s\n", message.c_str());

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        dbgPrintln("[Notify] WhatsApp gesendet");
    } else if (code > 0) {
        dbgPrintf("[Notify] Fehler %d: %s\n", code, http.getString().c_str());
    } else {
        dbgPrintf("[Notify] Anfrage fehlgeschlagen: %s\n", http.errorToString(code).c_str());
    }
    http.end();
}

void loadNotifyExtraConfig() {
    if (EEPROM.read(EEPROM_NOTIFY2_MAGIC_ADDR) == EEPROM_NOTIFY2_MAGIC_BYTE) {
        EEPROM.get(EEPROM_NOTIFY2_BASE, notifyExtra);
    } else {
        memset(&notifyExtra, 0, sizeof(notifyExtra));
        notifyExtra.whatsapp_enabled = true;  // bisheriges Verhalten (WhatsApp war der einzige Kanal) erhalten
        notifyExtra.smtp_port        = 465;   // implizites TLS, gaengigster Standardport
        saveNotifyExtraConfig();
    }
}

void saveNotifyExtraConfig() {
    EEPROM.put(EEPROM_NOTIFY2_BASE, notifyExtra);
    EEPROM.write(EEPROM_NOTIFY2_MAGIC_ADDR, EEPROM_NOTIFY2_MAGIC_BYTE);
    EEPROM.commit();
}

void sendTelegram(const String& message) {
    if (!notifyConfig.global_enabled) {
        dbgPrintln("[Notify] Benachrichtigungen deaktiviert");
        return;
    }
    if (!notifyExtra.telegram_enabled) {
        dbgPrintln("[Notify] Telegram-Kanal deaktiviert");
        return;
    }
    if (strlen(notifyExtra.telegram_token) == 0 || strlen(notifyExtra.telegram_chat_id) == 0) {
        dbgPrintln("[Notify] Telegram: Token/Chat-ID nicht konfiguriert");
        return;
    }
    static unsigned long lastSendMs = 0;
    unsigned long now = millis();
    if (now - lastSendMs < NOTIFY_RATE_LIMIT_MS) {
        dbgPrintln("[Notify] Telegram Rate-Limit, Nachricht verworfen");
        return;
    }
    lastSendMs = now;

    String url = "https://api.telegram.org/bot" + String(notifyExtra.telegram_token) +
                 "/sendMessage?chat_id=" + String(notifyExtra.telegram_chat_id) +
                 "&text=" + urlEncodeText(message);

    dbgPrintf("[Notify] Sende (Telegram): %s\n", message.c_str());

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        dbgPrintln("[Notify] Telegram gesendet");
    } else if (code > 0) {
        dbgPrintf("[Notify] Telegram-Fehler %d: %s\n", code, http.getString().c_str());
    } else {
        dbgPrintf("[Notify] Telegram-Anfrage fehlgeschlagen: %s\n", http.errorToString(code).c_str());
    }
    http.end();
}

// Liest eine (moeglicherweise mehrzeilige) SMTP-Antwort komplett ein und prueft den 3-stelligen
// Code der letzten Zeile ("250-..." = folgt noch was, "250 ..." = letzte Zeile des Blocks).
static bool smtpReadResponse(WiFiClientSecure& client, const char* expectedCode) {
    uint32_t start = millis();
    String lastLine;
    bool blockDone = false;
    while (!blockDone && millis() - start < 10000) {
        while (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            dbgPrintf("[SMTP] < %s\n", line.c_str());
            lastLine = line;
            if (line.length() >= 4 && line.charAt(3) != '-') { blockDone = true; break; }
        }
    }
    return lastLine.startsWith(expectedCode);
}

static bool smtpSend(WiFiClientSecure& client, const String& cmd, const char* expectedCode) {
    dbgPrintf("[SMTP] > %s\n", cmd.c_str());
    client.print(cmd);
    client.print("\r\n");
    return smtpReadResponse(client, expectedCode);
}

void sendEmailSmtp(const String& message) {
    if (!notifyConfig.global_enabled) {
        dbgPrintln("[Notify] Benachrichtigungen deaktiviert");
        return;
    }
    if (!notifyExtra.email_enabled) {
        dbgPrintln("[Notify] E-Mail-Kanal deaktiviert");
        return;
    }
    if (strlen(notifyExtra.smtp_host) == 0 || strlen(notifyExtra.smtp_user) == 0 ||
        strlen(notifyExtra.smtp_pass) == 0 || strlen(notifyExtra.email_to) == 0) {
        dbgPrintln("[Notify] E-Mail: SMTP nicht vollstaendig konfiguriert");
        return;
    }
    static unsigned long lastSendMs = 0;
    unsigned long now = millis();
    if (now - lastSendMs < NOTIFY_RATE_LIMIT_MS) {
        dbgPrintln("[Notify] E-Mail Rate-Limit, Nachricht verworfen");
        return;
    }
    lastSendMs = now;

    dbgPrintf("[Notify] Sende (E-Mail): %s\n", message.c_str());

    WiFiClientSecure client;
    client.setInsecure();  // kein Cert-Pinning -- gleiche Risikoeinschaetzung wie beim unverschluesselten HTTP zu CallMeBot/Telegram anderswo in dieser Firmware
    uint16_t port = notifyExtra.smtp_port ? notifyExtra.smtp_port : 465;
    if (!client.connect(notifyExtra.smtp_host, port, 8000)) {
        dbgPrintln("[Notify] E-Mail: SMTP-Verbindung fehlgeschlagen");
        return;
    }

    bool ok = smtpReadResponse(client, "220");
    ok = ok && smtpSend(client, "EHLO aeroponik", "250");
    ok = ok && smtpSend(client, "AUTH LOGIN", "334");
    ok = ok && smtpSend(client, base64::encode(String(notifyExtra.smtp_user)), "334");
    ok = ok && smtpSend(client, base64::encode(String(notifyExtra.smtp_pass)), "235");
    ok = ok && smtpSend(client, "MAIL FROM:<" + String(notifyExtra.smtp_user) + ">", "250");
    ok = ok && smtpSend(client, "RCPT TO:<" + String(notifyExtra.email_to) + ">", "250");
    ok = ok && smtpSend(client, "DATA", "354");

    if (ok) {
        // Dot-Stuffing: eine Zeile, die nur "." enthaelt, beendet die DATA-Uebertragung -- eine
        // Nutzlast-Zeile, die zufaellig mit "." beginnt, muss deshalb verdoppelt werden.
        String body = message;
        body.replace("\n.", "\n..");
        if (body.startsWith(".")) body = "." + body;

        String mail = "From: " + String(notifyExtra.smtp_user) + "\r\n" +
                      "To: " + String(notifyExtra.email_to) + "\r\n" +
                      "Subject: Grow Center Alarm\r\n" +
                      "Content-Type: text/plain; charset=UTF-8\r\n" +
                      "\r\n" +
                      body + "\r\n" +
                      ".";
        ok = smtpSend(client, mail, "250");
    }

    client.print("QUIT\r\n");
    client.stop();

    if (ok) dbgPrintln("[Notify] E-Mail gesendet");
    else    dbgPrintln("[Notify] E-Mail: SMTP-Fehler");
}

// Warteschlange + eigener Task statt direktem Senden aus loop(): ein HTTPS-Aufruf (TLS-Aufbau)
// oder eine SMTP-Sitzung (8 s Verbindung + bis zu 10 s je Antwort) kann bei schlechtem WLAN weit
// ueber 30 s dauern -- in loop() loeste das den Task-Watchdog aus (Neustart mitten in der
// Lampen-Kennlinie, 06.11.2026) und hielt bis dahin auch Ventile/Sprueh-Takte an. Ausserdem
// gehen Nachrichten ohne WLAN nicht verloren (z. B. die Meldung eines WLAN-Ausfalls), und zwei
// Alarme kurz hintereinander scheitern nicht mehr am Rate-Limit je Kanal.
#define NOTIFY_QUEUE_LEN    8
#define NOTIFY_QUEUE_GAP_MS (NOTIFY_RATE_LIMIT_MS + 1000UL)
#define NOTIFY_LATE_MS      (2UL * 60000UL)   // ab so viel Verzoegerung den Hinweis anhaengen
#define CH_WHATSAPP 0x01
#define CH_TELEGRAM 0x02
#define CH_EMAIL    0x04
#define CH_ALLE     (CH_WHATSAPP | CH_TELEGRAM | CH_EMAIL)

struct QueuedNotify { String text; unsigned long queuedMs; uint8_t kanaele; };
static QueuedNotify      notifyQueue[NOTIFY_QUEUE_LEN];
static uint8_t           notifyQueueHead = 0, notifyQueueCount = 0;
static SemaphoreHandle_t notifyQueueMutex = nullptr;

static void enqueue(const String& message, uint8_t kanaele) {
    if (!notifyQueueMutex) return;
    xSemaphoreTake(notifyQueueMutex, portMAX_DELAY);
    if (notifyQueueCount == NOTIFY_QUEUE_LEN) {   // voll: aelteste Nachricht verwerfen
        dbgPrintf("[Notify] Warteschlange voll, verwerfe: %s\n", notifyQueue[notifyQueueHead].text.c_str());
        notifyQueue[notifyQueueHead].text = String();
        notifyQueueHead = (notifyQueueHead + 1) % NOTIFY_QUEUE_LEN;
        notifyQueueCount--;
    }
    QueuedNotify& q = notifyQueue[(notifyQueueHead + notifyQueueCount) % NOTIFY_QUEUE_LEN];
    q.text     = message;
    q.queuedMs = millis();
    q.kanaele  = kanaele;
    notifyQueueCount++;
    xSemaphoreGive(notifyQueueMutex);
}

void sendNotification(const String& message) {
    if (!notifyConfig.global_enabled) return;
    enqueue(message, CH_ALLE);
}

void notifyTestWhatsApp(const String& message) { enqueue(message, CH_WHATSAPP); }
void notifyTestTelegram(const String& message) { enqueue(message, CH_TELEGRAM); }
void notifyTestEmail(const String& message)    { enqueue(message, CH_EMAIL); }

static void notifyTask(void*) {
    unsigned long lastSentMs = 0;
    bool          sentOnce   = false;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (WiFi.status() != WL_CONNECTED) continue;   // bleibt in der Warteschlange
        unsigned long now = millis();
        if (sentOnce && now - lastSentMs < NOTIFY_QUEUE_GAP_MS) continue;

        xSemaphoreTake(notifyQueueMutex, portMAX_DELAY);
        if (notifyQueueCount == 0) { xSemaphoreGive(notifyQueueMutex); continue; }
        QueuedNotify& q = notifyQueue[notifyQueueHead];
        String  message = q.text;
        uint8_t kanaele = q.kanaele;
        if (now - q.queuedMs >= NOTIFY_LATE_MS)
            message += "\n(verzögert um " + String((now - q.queuedMs) / 60000UL) + " min, kein WLAN)";
        q.text = String();
        notifyQueueHead = (notifyQueueHead + 1) % NOTIFY_QUEUE_LEN;
        notifyQueueCount--;
        xSemaphoreGive(notifyQueueMutex);

        lastSentMs = now;
        sentOnce   = true;
        // Die Sendefunktionen pruefen selbst global_enabled/Kanal-Schalter/Zugangsdaten
        if ((kanaele & CH_WHATSAPP) && (kanaele != CH_ALLE || notifyExtra.whatsapp_enabled)) sendWhatsApp(message);
        if ((kanaele & CH_TELEGRAM) && (kanaele != CH_ALLE || notifyExtra.telegram_enabled)) sendTelegram(message);
        if ((kanaele & CH_EMAIL)    && (kanaele != CH_ALLE || notifyExtra.email_enabled))    sendEmailSmtp(message);
    }
}

void notify_begin() {
    if (notifyQueueMutex) return;
    notifyQueueMutex = xSemaphoreCreateMutex();
    // HTTPS/TLS + SMTP brauchen reichlich Stack; niedrige Prioritaet, laeuft neben loop()
    xTaskCreatePinnedToCore(notifyTask, "notify", 12288, nullptr, 1, nullptr, 0);
}
