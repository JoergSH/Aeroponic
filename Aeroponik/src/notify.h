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

// Schickt "message" per CallMeBot an notifyConfig.phone. Greift nur, wenn global_enabled
// gesetzt und Telefonnummer/API-Key hinterlegt sind; zusaetzlich ein knappes Rate-Limit,
// damit ein Bug nicht wiederholt Nachrichten (und damit das CallMeBot-Konto) verbrennt.
// Wird sowohl von den einzelnen Alarmen (siehe loopNotify() in main.cpp) als auch von der
// Testnachricht im Webinterface genutzt.
void sendWhatsApp(const String& message);

#endif
