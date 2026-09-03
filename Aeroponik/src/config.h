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

// ========== Umwälzpumpe Defaults (Vorratsbehälter-Zirkulation) ==========
#define UMWAELZ_AKTIV_DEFAULT       false
#define UMWAELZ_LAUFZEIT_DEFAULT_S      30   // Sekunden pro Lauf-Zyklus
#define UMWAELZ_PAUSE_DEFAULT_MIN       30   // Minuten Pause zwischen den Zyklen

// ========== Lueftermodul Defaults (Zeltluefter, Arctic PWM, 600-3000 U/min) ==========
#define LUEFTMODUL_MONITOR_TOL_DEFAULT   30   // % grosszuegige Toleranz um den Erwartungswert

// ========== WhatsApp-Benachrichtigung Defaults (CallMeBot) ==========
#define NOTIFY_FEUCHTE_MIN_DEFAULT       40   // %rH
#define NOTIFY_FEUCHTE_MAX_DEFAULT       80   // %rH
#define NOTIFY_TEMPERATUR_MIN_DEFAULT    15   // °C
#define NOTIFY_TEMPERATUR_MAX_DEFAULT    32   // °C
#define NOTIFY_TANK_MIN_PROZENT_DEFAULT  15   // %

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
//  176:    DWC-Timer Magic Byte
//  177–186: DwcTimerConfig (bis zu 10 Bytes)
//  187:    Ventil-"Gleiche Zeiten" Magic Byte
//  188:    Ventil-"Gleiche Zeiten" Wert (1 Byte)
//  189–190: frei (ehemals Zeltlüfter, entfallen mit dem SMD-Board)
//  191:    Gehäuselüfter Magic Byte
//  192–195: GehaeuseFanConfig (4 Bytes)
//  196:    DS18B20-Rollenzuordnung Magic Byte
//  197–223: Ds18b20Config (27 Bytes)
//  224:    Ventil-Dunkelphase Magic Byte (Werte selbst in den Behälter-Reserve-Bytes 3/4)
//  225:    Umwälzpumpe Magic Byte
//  226–228: UmwaelzConfig (3 Bytes: aktiv + laufzeit_s + pausenzeit_min)
//  229:    frei
//  230:    WhatsApp-Benachrichtigung Magic Byte
//  231–275: NotifyConfig (45 Bytes: global_enabled + phone[20] + apikey[12] + 6 Alarm-
//           Aktivierungen [inkl. Lueftermodul] + je min/max fuer Feuchte/Temperatur/Tank)
//  276:    Lueftermodul (Zeltluefter) Magic Byte
//  277–284: LueftmodulConfig (8 Bytes)
#define EEPROM_SIZE             290
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

// DWC-Beleuchtungstimer (separates System, ueber Steckdosen-Node-Ausgang geschaltet)
#define EEPROM_DWC_MAGIC_ADDR    176
#define EEPROM_DWC_MAGIC_BYTE    0x3A
#define EEPROM_DWC_BASE          177

// Ventile: "Gleiche Zeiten fuer alle" Batch-Modus — eigener Magic-Bereich, weil das Feld
// nachtraeglich zum urspruenglichen Ventilkonfig-Layout (Magic bei EEPROM_MAGIC_ADDR)
// dazukam und sonst bei Bestandsinstallationen ungueltige Alt-Daten gelesen wuerden.
#define EEPROM_VENTIL_GLEICH_MAGIC_ADDR 187
#define EEPROM_VENTIL_GLEICH_MAGIC_BYTE 0x5C
#define EEPROM_VENTIL_GLEICH_ADDR       188

// Ventile: Dunkelphase-Zeiten je Behaelter (oeffnungszeit_s_dunkel/pausenzeit_min_dunkel)
// -- liegen in den ohnehin reservierten Bytes 3/4 jedes 5-Byte-Behaelter-Blocks (siehe
// EEPROM_BEHAELTER_BASE), aber mit eigenem Magic-Byte, damit bei Bestandsinstallationen
// nicht versehentlich Alt-Reserve-Bytes als Zeiten interpretiert werden.
#define EEPROM_VENTIL_DUNKEL_MAGIC_ADDR 224
#define EEPROM_VENTIL_DUNKEL_MAGIC_BYTE 0x6F

// Umwälzpumpe (Vorratsbehälter-Zirkulation): automatischer Lauf-/Pausenzyklus, aktiv sobald
// irgendein Behälter aktiviert ist (siehe loopVentile() in valves.cpp). Ersetzt den
// fruexeren rein manuellen Zeltlüfter-Schalter auf demselben GPIO (siehe pinout.h) — eigener
// Adressbereich in bisher ungenutzten Reserve-Bytes, damit alte Zeltlüfter-Werte nicht
// versehentlich als neue Konfiguration gelesen werden.
#define EEPROM_UMWAELZ_MAGIC_ADDR       225
#define EEPROM_UMWAELZ_MAGIC_BYTE       0x2F
#define EEPROM_UMWAELZ_BASE             226   // 3 Bytes: aktiv + laufzeit_s + pausenzeit_min

// WhatsApp-Benachrichtigungen (CallMeBot) — Zugangsdaten + generelle und je Alarmtyp
// einzeln abschaltbare Aktivierung, siehe notify.h. Magic-Byte geaendert (0x9A -> 0x9B), da
// mit dem Lueftermodul-Alarm ein weiteres Aktivierungs-Bit dazugekommen ist.
#define EEPROM_NOTIFY_MAGIC_ADDR        230
#define EEPROM_NOTIFY_MAGIC_BYTE        0x9B
#define EEPROM_NOTIFY_BASE              231   // 45 Bytes, siehe NotifyConfig in notify.h

// Lueftermodul (Zeltluefter, RS485-Adresse 0x51): PWM-Sollwerte + Drehzahlueberwachung,
// siehe lueftmodul.h.
#define EEPROM_LUEFTMODUL_MAGIC_ADDR    276
#define EEPROM_LUEFTMODUL_MAGIC_BYTE    0x5E
#define EEPROM_LUEFTMODUL_BASE          277   // 8 Bytes, siehe LueftmodulConfig in lueftmodul.h

// Gehaeuseluefter-Config: eigener Magic-Bereich. Magic-Byte geaendert (0x4D -> 0x51), da
// min_speed mit dem Wechsel von PWM auf reines An/Aus entfallen ist (dieser Lueftertyp
// kommt mit PWM-Drosselung nicht klar).
#define EEPROM_GEHFAN_MAGIC_ADDR        191
#define EEPROM_GEHFAN_MAGIC_BYTE        0x51
#define EEPROM_GEHFAN_BASE              192   // 3 Bytes (enabled, temp_min, temp_max)

// DS18B20-Rollenzuordnung (Vorrat/Pflanze/Innentemperatur) ueber ROM-Adresse statt
// Bus-Scan-Reihenfolge. Solange eine Rolle nicht zugewiesen ist,
// faellt main.cpp auf die alte Index-Reihenfolge zurueck (0/1/2).
#define EEPROM_DS18B20_MAGIC_ADDR       196
#define EEPROM_DS18B20_MAGIC_BYTE       0x2C
#define EEPROM_DS18B20_BASE             197   // 27 Bytes: 3x (1 Byte "gesetzt" + 8 Byte Adresse)

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
