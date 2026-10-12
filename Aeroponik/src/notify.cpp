#include "notify.h"
#include "config.h"
#include "dbg.h"
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
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

void sendNotification(const String& message) {
    if (!notifyConfig.global_enabled) return;
    if (notifyExtra.whatsapp_enabled) sendWhatsApp(message);
    if (notifyExtra.telegram_enabled) sendTelegram(message);
    if (notifyExtra.email_enabled)    sendEmailSmtp(message);
}
