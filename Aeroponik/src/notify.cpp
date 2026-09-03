#include "notify.h"
#include "config.h"
#include "dbg.h"
#include <EEPROM.h>
#include <HTTPClient.h>
#include <string.h>

NotifyConfig notifyConfig;

static unsigned long lastSendMs = 0;
#define NOTIFY_RATE_LIMIT_MS 30000UL   // Mindestabstand zwischen zwei Nachrichten

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
    if (strlen(notifyConfig.phone) == 0 || strlen(notifyConfig.apikey) == 0) {
        dbgPrintln("[Notify] Telefonnummer/API-Key nicht konfiguriert");
        return;
    }
    unsigned long now = millis();
    if (now - lastSendMs < NOTIFY_RATE_LIMIT_MS) {
        dbgPrintln("[Notify] Rate-Limit, Nachricht verworfen");
        return;
    }
    lastSendMs = now;

    // URL-Encoding: "%" zuerst, sonst werden die eigenen %XX-Sequenzen unten doppelt kodiert.
    String encoded = message;
    encoded.replace("%", "%25");
    encoded.replace("\n", "%0A");
    encoded.replace(" ", "+");
    encoded.replace("ä", "%C3%A4"); encoded.replace("ö", "%C3%B6"); encoded.replace("ü", "%C3%BC");
    encoded.replace("ß", "%C3%9F");
    encoded.replace("Ä", "%C3%84"); encoded.replace("Ö", "%C3%96"); encoded.replace("Ü", "%C3%9C");

    String url = "https://api.callmebot.com/whatsapp.php?phone=" + String(notifyConfig.phone) +
                 "&text=" + encoded + "&apikey=" + String(notifyConfig.apikey);

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
