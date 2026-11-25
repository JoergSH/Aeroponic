#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "valves.h"   // BEHAELTER_ANZAHL

#define EC_LOG_KATS 5   // = EC_KAT_ANZAHL (espnow_protocol.h), Reihenfolge der CSV-Spalten

struct LogEntry {
    float    tempVorrat, tempPflanze;
    float    druckBar;
    int      fuellstandProzent;
    float    volumenLiter;
    bool     raumOk;
    float    raumTemp, raumFeuchte;
    bool     zeltOk;
    float    zeltTemp, zeltFeuchte;
    uint16_t zeltCo2;
    bool     zeltCo2Ok;           // false: kein CO2-Sensor / ENS160 waermt auf -> leere Zelle
    float    zeltVpd;
    bool     ppfdOk;              // MS2 online UND AS7341 ok (Status-Bit1) -- sonst leere Zelle
    float    zeltPpfd;
    uint8_t  lichtMaske;
    bool     fanOk;
    uint8_t  fanPercent;
    uint16_t fanRpm;
    bool     co2Ausgang;
    bool     humidAusgang;
    // Letzter Sprühvorgang je Behaelter seit der vorigen Log-Zeile (flowValid=false -> leere Zellen,
    // es gab in diesem Intervall keinen Sprühvorgang dieses Behaelters)
    bool     flowValid[BEHAELTER_ANZAHL];
    uint32_t flowImpulse[BEHAELTER_ANZAHL];
    float    flowDauerS[BEHAELTER_ANZAHL];
    // Neustart-Diagnose: Laufzeit seit Boot (faellt sie zurueck, gab es einen Neustart) und
    // Grund des letzten Neustarts (Text ohne Komma, siehe resetReasonToText() in main.cpp)
    uint32_t laufzeitMin;
    const char* neustartGrund;
    // Neustart-Diagnose des Lichtx4 (FW >= 1.2.0, sonst leere Zellen): steigt der Startzaehler,
    // hat das Modul neu gestartet -- dabei sind alle SSRs kurz aus.
    bool     lichtDiagOk;
    uint16_t lichtStarts;
    const char* lichtGrund;
    // Kurze Netzaussetzer seit der vorigen Log-Zeile (USV uebernimmt, LED-Treiber gehen kurz aus)
    uint16_t netzAussetzer;
    uint32_t netzAussetzerMaxMs;
    // Rueckmeldung der Steckdose (zuletzt gemeldeter Relaiszustand) zu CO2-/Humid-Ausgang:
    // 0/1, -1 = Steckdose offline/unbekannt (leere Zelle). Weicht sie vom Wunsch (CO2Ausgang/
    // HumidAusgang) ab, kommt der Schaltbefehl nicht an.
    int8_t   co2Steckdose;
    int8_t   humidSteckdose;
    // Funkverbindung: WLAN-Signal (leer ohne Verbindung), WLAN-Abbrueche seit der vorigen Zeile,
    // ESP-NOW-Nodes online/gekoppelt, kleinster freier Heap seit dem Start
    bool     wlanOk;
    int8_t   wlanDbm;
    uint16_t wlanAbbrueche;
    uint8_t  espnowOnline;
    uint8_t  espnowGekoppelt;
    uint32_t heapMinKb;
    uint16_t ms2Abfragen;   // RS485-Abfragen an den Zelt-Sensor (Adresse 0x20) seit der vorigen Zeile
    uint16_t ms2Timeouts;   // davon unbeantwortet
    bool     vpdOk;         // VPD-Regelung aktiv mit gueltigen Werten (sonst leere Spalten)
    float    vpdBlatt, vpdSollMin, vpdSollMax;   // kPa
    // EC-Meter (Handgeraet): Messung je Kategorie seit der vorigen Zeile, Index = EC_KAT_* - 1:
    // 0 = Naehrloesung, 1 = Ruecklauf, 2 = Cal-Mag, 3 = Rohwasser, 4 = VE-Wasser
    // (ecValid=false -> leere Zellen)
    bool     ecValid[EC_LOG_KATS];
    uint16_t ecUs[EC_LOG_KATS];       // uS/cm bei 25 °C
    bool     ecTempOk[EC_LOG_KATS];   // false -> ohne Temperaturfuehler gemessen, Temperaturzelle leer
    float    ecTemp[EC_LOG_KATS];
};

// Eigene, tagesuebergreifende Datei fuer die EC-Messungen vom EC-Meter (eine Zeile je Messung).
// Liegt neben den Tageslogs, ist daher auch ueber /api/logs/download abrufbar.
#define EC_LOG_NAME "EC-Messungen.csv"
void storage_log_ec(time_t ts, const char* kategorie, uint16_t ec25, bool tempOk, float temp,
                    uint16_t ecRaw, bool overrange);

// Ruecklauf vom Fuellstand-Modul (Abpumpe, meldet jeden Pumpzyklus per WLAN, /api/ruecklauf).
// Eigene, tagesuebergreifende Datei, eine Zeile je Zyklus.
#define RUECKLAUF_LOG_NAME "Ruecklauf.csv"
void storage_log_ruecklauf(const char* datum, const char* zeit, uint32_t dauer_s, uint32_t bisMin_s,
                           const char* art, uint32_t volumen_ml);

bool storage_init(SPIClass &spi);
bool storage_is_ok();
void storage_log(time_t ts, const LogEntry &e);

// Listet alle *.csv-Logdateien im Root der SD-Karte auf (fuer Download-UI).
void storage_list_logs(void (*cb)(const char *name, size_t size, void *ctx), void *ctx);

// Oeffnet eine Log-Datei zum Lesen (Download). Leeres File-Objekt wenn nicht gefunden.
File storage_open_log(const char *name);

// Loescht eine Log-Datei. Rueckgabe false wenn nicht gefunden/Fehler.
bool storage_delete_log(const char *name);

// Loescht alle *.csv/*.txt-Dateien im Root der SD-Karte. Rueckgabe: Anzahl geloeschter Dateien.
uint32_t storage_delete_all_logs();

// Upload einer bearbeiteten EC-Messungen.csv: wird erst in eine Temp-Datei geschrieben und
// erst nach erfolgreicher Pruefung (Kopfzeile) gegen die bestehende Datei getauscht -- die
// bisherige bleibt als EC_BACKUP_NAME erhalten. finish() liefert false + Fehlertext.
#define EC_BACKUP_NAME "EC-Messungen_vorher.csv"
#define EC_UPLOAD_MAX_BYTES (2UL * 1024UL * 1024UL)
bool storage_ec_upload_begin();
bool storage_ec_upload_write(const uint8_t* buf, size_t len);
bool storage_ec_upload_finish(char* err, size_t errLen);
void storage_ec_upload_abort();

#endif
