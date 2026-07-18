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

#endif
