#ifndef FANCTRL_H
#define FANCTRL_H

#include <stdint.h>
#include <stdbool.h>

// Modus des Abluftluefters (MARS Hydro, RS485-Adresse 6)
#define FAN_MODE_MANUAL        0
#define FAN_MODE_AUTO_HUMIDITY 1
#define FAN_MODE_AUTO_TEMP     2

// Temperatur-Basis fuer FAN_MODE_AUTO_TEMP
#define FAN_TEMP_MODE_ABSOLUTE 0  // temp_min/temp_max = absolute Zelttemperatur (°C)
#define FAN_TEMP_MODE_DIFF     1  // temp_min/temp_max = Differenz Zelt-Raum (°C)

struct FanConfig {
    uint8_t mode;            // FAN_MODE_*
    uint8_t manual_percent;  // 0-100, genutzt wenn mode==FAN_MODE_MANUAL
    uint8_t min_speed;       // 0-100, Mindestdrehzahl fuer beide Automodi
    uint8_t hum_min;         // %rH — darunter: Mindestdrehzahl
    uint8_t hum_max;         // %rH — darueber: 100%
    uint8_t temp_min;        // je nach temp_mode: absolute °C oder Differenz °C — darunter: Mindestdrehzahl
    uint8_t temp_max;        // je nach temp_mode: absolute °C oder Differenz °C — darueber: 100%
    bool    enabled;         // false = keine eigenen Schreibbefehle (iControl behaelt Kontrolle)
    uint8_t temp_mode;       // FAN_TEMP_MODE_*
    uint8_t temp_cap_diff;   // nur bei temp_mode=ABSOLUTE: Differenz-Bremse °C, 0=deaktiviert
                              // (Zelt-Raum-Differenz kleiner als dieser Wert -> trotz Rampe nur Mindestdrehzahl)
};

extern FanConfig fanConfig;

void loadFanConfig();
void saveFanConfig();

#endif
