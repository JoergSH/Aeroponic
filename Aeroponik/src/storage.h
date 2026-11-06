#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

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
    bool     flowValid[3];
    uint32_t flowImpulse[3];
    float    flowDauerS[3];
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
};

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

#endif
