#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t  temperature_cdeg;  // °C * 100 (SCD41)
    uint16_t humidity_cpct;     // % * 100  (SCD41)
    uint16_t co2_ppm;           // CO2 ppm  (SCD41, echter NDIR-Wert)
    uint16_t ppfd_dppfd;        // PPFD * 10
    uint16_t ch_415nm;
    uint16_t ch_445nm;
    uint16_t ch_480nm;
    uint16_t ch_515nm;
    uint16_t ch_555nm;
    uint16_t ch_590nm;
    uint16_t ch_630nm;
    uint16_t ch_680nm;
    uint8_t  as7341_gain;
} sensor_data_t;

// Rueckgabe false = SCD41 nicht gefunden/gestartet (treibt den gesamten Messzyklus,
// AS7341 wird nur mitgelesen wenn SCD41 laeuft — siehe sensor_manager.cpp).
bool sensor_manager_init(uint8_t sda_pin, uint8_t scl_pin);
void sensor_manager_loop();
bool sensor_manager_data_ready();
void sensor_manager_get_data(sensor_data_t* out);
bool sensor_manager_as7341_ok();   // false = AS7341 nicht gefunden/defekt/deaktiviert, Lichtwerte ungueltig

// SCD41-Teilergebnis ist unabhaengig vom (ggf. haengenden) AS7341 verfuegbar -- siehe
// sensor_manager_data_ready()/_get_data() fuer den kompletten Zyklus inkl. AS7341. Ohne diese
// Trennung blieb bei haengendem AS7341 auch eine laengst fertige, gueltige SCD41-Messung
// (CO2/Temp/Feuchte) auf der Strecke, weil sie nur gemeinsam mit AS7341 uebertragen wurde.
bool sensor_manager_scd41_ready();
void sensor_manager_get_scd41(int16_t* temp_cdeg, uint16_t* hum_cpct, uint16_t* co2_ppm);

#endif
