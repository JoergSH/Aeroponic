#ifndef NOTIFY_H
#define NOTIFY_H

#include <Arduino.h>

// WhatsApp-Benachrichtigungen ueber CallMeBot (https://www.callmebot.com/blog/free-api-whatsapp-messages/).
// Zugangsdaten (Telefonnummer + API-Key) erhaelt man einmalig, indem man dem CallMeBot-
// Kontakt auf WhatsApp eine Aktivierungsnachricht schickt.
struct NotifyConfig {
    bool    global_enabled;    // Generelle Benachrichtigung ein/aus (uebersteuert alle unten)
    char    phone[20];         // inkl. Laendervorwahl, z.B. "+491701234567"
    char    apikey[12];        // von CallMeBot per WhatsApp zugewiesen

    bool    netzausfall_enabled;      // Netzspannung ausgefallen (USV-Betrieb, siehe NETZ_OK_PIN)
    bool    sensorausfall_enabled;    // AHT21/RTC/DS18B20 nicht erreichbar

    bool    feuchte_enabled;          // Zelt-Luftfeuchte (RS485-Multisensor) außerhalb Min/Max
    uint8_t feuchte_min;              // %rH
    uint8_t feuchte_max;              // %rH

    bool    temperatur_enabled;       // Zelt-Temperatur (RS485-Multisensor) außerhalb Min/Max
    uint8_t temperatur_min;           // °C
    uint8_t temperatur_max;           // °C

    bool    tank_enabled;             // Vorratsbehälter-Füllstand unter Grenzwert
    uint8_t tank_min_prozent;         // %

    bool    lueftmodul_enabled;       // Zeltlüfter-Drehzahl weicht vom Erwartungswert ab
};

extern NotifyConfig notifyConfig;

void loadNotifyConfig();
void saveNotifyConfig();

// Schickt "message" per CallMeBot an notifyConfig.phone. Greift nur, wenn global_enabled +
// whatsapp_enabled gesetzt und Telefonnummer/API-Key hinterlegt sind; zusaetzlich ein knappes
// Rate-Limit, damit ein Bug nicht wiederholt Nachrichten (und damit das CallMeBot-Konto)
// verbrennt. Wird sowohl von den einzelnen Alarmen (siehe loopNotify() in main.cpp, ueber
// sendNotification()) als auch von der Testnachricht im Webinterface genutzt.
void sendWhatsApp(const String& message);

// ========== Telegram + E-Mail (SMTP) — zusaetzliche Benachrichtigungskanaele ==========
// CallMeBot (WhatsApp) ist erfahrungsgemaess oft instabil (WhatsApp-seitige Probleme) --
// Telegram und/oder E-Mail koennen unabhaengig voneinander zusaetzlich aktiviert werden, damit
// Alarme nicht komplett ausbleiben wenn ein Kanal gerade down ist. Eigener EEPROM-Block (siehe
// config.h), damit das bestehende NotifyConfig-Layout oben unveraendert bleibt.
struct NotifyExtraConfig {
    bool     whatsapp_enabled;     // WhatsApp einzeln an/aus (zusaetzlich zu global_enabled)
    bool     telegram_enabled;
    char     telegram_token[48];   // von @BotFather, Format "123456789:AAExxx..."
    char     telegram_chat_id[20]; // eigene Chat-ID (z.B. von @userinfobot), kann negativ sein (Gruppe)
    bool     email_enabled;
    char     smtp_host[40];        // z.B. "smtp.gmail.com"
    uint16_t smtp_port;            // z.B. 465 (implizites TLS, kein STARTTLS-Aufwand noetig)
    char     smtp_user[40];        // Absender-Login (meist die volle E-Mail-Adresse)
    char     smtp_pass[24];        // App-Passwort — NICHT das normale Kontopasswort!
    char     email_to[40];         // Empfaenger-Adresse
};

extern NotifyExtraConfig notifyExtra;

void loadNotifyExtraConfig();
void saveNotifyExtraConfig();

// Sendet ueber die Telegram Bot API (HTTPS GET, gleiches Muster wie sendWhatsApp). Greift nur,
// wenn global_enabled + telegram_enabled gesetzt und Token/Chat-ID hinterlegt sind.
void sendTelegram(const String& message);

// Sendet per SMTP (implizites TLS) ueber das konfigurierte Postfach. Greift nur, wenn
// global_enabled + email_enabled gesetzt und alle SMTP-Felder hinterlegt sind.
void sendEmailSmtp(const String& message);

// Zentraler Einstiegspunkt fuer alle Alarme (siehe loopNotify() in main.cpp): schickt die
// Nachricht ueber alle aktuell aktivierten Kanaele (WhatsApp/Telegram/E-Mail). Jeder Kanal
// wird unabhaengig versucht -- faellt einer aus (z.B. CallMeBot down), gehen die anderen
// trotzdem raus.
void sendNotification(const String& message);

#endif
