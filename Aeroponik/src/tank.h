#ifndef TANK_H
#define TANK_H

#include <stdint.h>

struct TankConfig {
    uint16_t hoehe_mm;
    uint16_t radius_unten_mm;
    uint16_t radius_oben_mm;
    uint16_t leer_abstand_mm;
    uint16_t min_mm;
    uint16_t max_mm;
    int16_t  offset_mm;
};

extern TankConfig tankConfig;

void loadTankConfig();
void saveTankConfig();

// ========== Fuellstandssensor-Auswahl (Ultraschall / Drucksensor) ==========
// Eigener EEPROM-Block mit eigenem Magic-Byte, damit bestehende TankConfig-Adressen
// unten (Zeitplan/CO2/Fan/...) unveraendert bleiben, siehe config.h.
enum TankSensorType : uint8_t {
    TANK_SENSOR_ULTRASCHALL = 0,
    TANK_SENSOR_DRUCK       = 1,
};

struct TankSensorConfig {
    uint8_t sensor_type;         // TankSensorType
    int32_t druck_null_raw;      // HX710B-Rohwert bei leerem Tank (0mm Wasserhoehe)
    float   druck_counts_pro_mm; // Rohwert-Aenderung pro mm Wasserhoehe (empirisch kalibriert)
};

extern TankSensorConfig tankSensorConfig;

void loadTankSensorConfig();
void saveTankSensorConfig();

#endif
