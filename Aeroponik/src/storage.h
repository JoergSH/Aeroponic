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
    float    zeltVpd;
    uint8_t  lichtMaske;
    bool     fanOk;
    uint8_t  fanPercent;
    uint16_t fanRpm;
    bool     co2Ausgang;
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

#endif
