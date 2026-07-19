#ifndef CONFIG_H
#define CONFIG_H

#include <IPAddress.h>
#include "pinout.h"

// ========== WiFi Konfiguration ==========
// SSID/Passwort des Heim-WLANs liegen NICHT mehr im Quellcode — siehe wifictrl.h.
// Sie werden über das Webinterface (Konfiguration → WLAN) gesetzt und im EEPROM
// gespeichert. Bei leerem/falschem WLAN startet der Master automatisch als AP
// (siehe unten), über den die Zugangsdaten dann gesetzt werden können.

// Access Point Konfiguration
extern const char* AP_SSID;
extern const char* AP_PASSWORD;
extern const IPAddress AP_IP;
extern const IPAddress AP_GATEWAY;
extern const IPAddress AP_SUBNET;
extern bool USE_ACCESS_POINT;

// ========== Tank Geometrie (Kegelstumpf) ==========
#define TANK_HOEHE_MM          370   // Physikalische Gesamthöhe in mm
#define TANK_RADIUS_UNTEN_MM   160   // Radius Boden (Ø320 / 2)
#define TANK_RADIUS_OBEN_MM    180   // Radius oben (Ø360 / 2)

// ========== Ultraschall Sensor Kalibrierung ==========
#define SENSOR_LEER_ABSTAND_MM 420   // Abstand Sensor→Tankboden bei LEEREM Tank — nach Montage anpassen!
#define SENSOR_MIN_MM          50   // Mindestreichweite — Werte darunter = Tank voll
#define SENSOR_MAX_MM         2000   // Maximalreichweite — Werte darüber = ungültig
#define SENSOR_OFFSET_MM        20   // Kalibrierungsoffset (gemessen 23cm bei echten 25cm)

// ========== Ventilsteuerung Defaults ==========
#define VENTIL_OEFFNUNG_DEFAULT_S   5    // Öffnungszeit Düsen in Sekunden
#define VENTIL_PAUSE_DEFAULT_MIN    5    // Pausenzeit in Minuten
#define VENTIL_VORLAUF_DEFAULT_S    3    // Vorlaufzeit Rückleitung in Sekunden
#define VENTIL_VORLAUF_AKTIV        true // Vorlauf standardmäßig aktiv

// ========== EEPROM Konfiguration ==========
// Adressen:
//  0–3:   Magic Number (Ventilkonfig)
//  4–18:  Behälter 1–3 (je 5 Bytes: oeffnung_s + pause_min + aktiv + reserved×2)
//  19:    Vorlaufzeit_s
//  20:    Vorlauf aktiv
//  21–23: frei
//  24–37: TankConfig (7 × 2 Bytes = 14 Bytes)
//  38:    Tank Magic Byte
//  54:    CO2-Steuerung Magic Byte
//  55–62: Co2Config (8 Bytes)
//  63:    Luefter-Steuerung Magic Byte
//  64–73: FanConfig (10 Bytes)
//  74:    WLAN-Konfiguration Magic Byte
//  75–172: WifiConfig (ssid[33] + password[65] = 98 Bytes)
//  173:    Ausgangs-Konfiguration Magic Byte
//  174–175: OutputConfig (2 Bytes)
#define EEPROM_SIZE             180
#define EEPROM_MAGIC_ADDR        0
#define EEPROM_MAGIC_NUMBER      0xAE4013AC
#define EEPROM_BEHAELTER_BASE    4    // je 5 Bytes pro Behälter
#define EEPROM_VORLAUF_ADDR     19
#define EEPROM_VORLAUF_AK_ADDR  20
#define EEPROM_TANK_BASE        24
#define EEPROM_TANK_MAGIC_ADDR  38
#define EEPROM_TANK_MAGIC_BYTE  0xB7
//  39:    Scheduler Magic Byte
//  40–53: LightScheduleConfig (14 Bytes)
#define EEPROM_SCHED_MAGIC_ADDR 39
#define EEPROM_SCHED_MAGIC_BYTE 0xD5   // geändert: PWM→SSR Struct-Änderung
#define EEPROM_SCHED_BASE       40
#define EEPROM_CO2_MAGIC_ADDR   54
#define EEPROM_CO2_MAGIC_BYTE   0x7C
#define EEPROM_CO2_BASE         55
#define EEPROM_FAN_MAGIC_ADDR   63
#define EEPROM_FAN_MAGIC_BYTE   0x92   // geändert: temp_mode/temp_cap_diff ergänzt
#define EEPROM_FAN_BASE         64
#define EEPROM_WIFI_MAGIC_ADDR  74
#define EEPROM_WIFI_MAGIC_BYTE  0x4E
#define EEPROM_WIFI_BASE        75
//  173:    Ausgangs-Konfiguration Magic Byte (Licht/Luefter: Lichtx4/MARS vs. Analogmodul)
//  174–175: OutputConfig (2 Bytes) — Rest (176–179) bleibt Reserve
#define EEPROM_OUTPUT_MAGIC_ADDR 173
#define EEPROM_OUTPUT_MAGIC_BYTE 0x61
#define EEPROM_OUTPUT_BASE       174

// ========== Lichtsteuerung Scheduler Defaults ==========
#define SCHED_DEFAULT_NODE        1
#define SCHED_DEFAULT_DAWN_START  360   //  6:00 Uhr
#define SCHED_DEFAULT_DAWN_END    480   //  8:00 Uhr
#define SCHED_DEFAULT_DUSK_START 1080   // 18:00 Uhr
#define SCHED_DEFAULT_DUSK_END   1200   // 20:00 Uhr
#define SCHED_DEFAULT_NUM_SSR     4     // Anzahl SSRs

// ========== CO2-Steuerung Defaults ==========
#define CO2_MIN_DEFAULT           800   // ppm — darunter: Ausgang EIN
#define CO2_MAX_DEFAULT          1200   // ppm — darüber: Ausgang AUS

// ========== Abluftluefter-Steuerung Defaults ==========
#define FAN_MIN_SPEED_DEFAULT      30   // % — Mindestdrehzahl fuer beide Automodi
#define FAN_HUM_MIN_DEFAULT        50   // %rH — darunter: Mindestdrehzahl
#define FAN_HUM_MAX_DEFAULT        70   // %rH — darueber: 100%
#define FAN_TEMP_MIN_DEFAULT       24   // °C — darunter: Mindestdrehzahl
#define FAN_TEMP_MAX_DEFAULT       30   // °C — darueber: 100%

// ========== Drucksensor Kalibrierung ==========
// Sensor: 0–150 PSI / 0.5–4.5V (ratiometrisch)
// Arbeitsbereich: 0–75 PSI (0.5–2.5V)
#define DRUCK_MV_MIN     500   // mV bei 0 PSI
#define DRUCK_MV_MAX    2500   // mV bei 75 PSI
#define DRUCK_PSI_MAX     75   // PSI bei MV_MAX
#define DRUCK_ADC_SAMPLES  8   // Mittelwert über N Messungen

// ========== NTP / Zeitzone ==========
#define NTP_SERVER        "pool.ntp.org"
// POSIX-TZ-String für Mitteleuropa (Deutschland): CET=UTC+1 im Winter, CEST=UTC+2 im
// Sommer, automatischer Wechsel jeweils am letzten Sonntag im März/Oktober (EU-Regel).
// Im Gegensatz zu configTime(gmtOffset, dstOffset, ...) wird die Sommerzeit damit
// korrekt nur zeitweise angewendet statt ganzjährig fest addiert.
#define NTP_TZ             "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SYNC_INTERVAL  3600000UL  // RTC alle 1h mit NTP abgleichen (ms)
#define NTP_MAX_ABWEICHUNG 60         // RTC stellen wenn Abweichung > 60 Sekunden

// ========== Debug-Ausgaben ==========
// Auskommentieren zum Aktivieren der jeweiligen Sensor-Ausgaben
// #define DEBUG_DRUCK    // Drucksensor: Ausgabe bei jeder Messung (~alle 3s)
// #define DEBUG_DISTANZ  // Distanzsensor: Ausgabe alle 500ms

// ========== Timing Intervalle ==========
constexpr unsigned long MEASUREMENT_INTERVAL   = 3000;
constexpr unsigned long DISPLAY_UPDATE_INTERVAL = 1000;
constexpr unsigned long EEPROM_SAVE_INTERVAL   = 30000;

#endif
