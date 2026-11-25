#include "fanctrl.h"
#include "config.h"
#include <EEPROM.h>
#include <math.h>

FanConfig fanConfig;
uint8_t   fanMaxSpeed = FAN_MAX_SPEED_DEFAULT;
uint8_t   fanTempZehntel[3] = { 0, 0, 0 };

float fanTempMinC()     { return fanConfig.temp_min      + fanTempZehntel[0] / 10.0f; }
float fanTempMaxC()     { return fanConfig.temp_max      + fanTempZehntel[1] / 10.0f; }
float fanTempCapDiffC() { return fanConfig.temp_cap_diff + fanTempZehntel[2] / 10.0f; }

static void splitZehntel(float v, uint8_t& ganz, uint8_t& zehntel) {
    int z = (int)lroundf(v * 10.0f);
    if (z < 0) z = 0;
    ganz = z / 10; zehntel = z % 10;
}
void fanSetTemps(float tMin, float tMax, float capDiff) {
    splitZehntel(tMin,    fanConfig.temp_min,      fanTempZehntel[0]);
    splitZehntel(tMax,    fanConfig.temp_max,      fanTempZehntel[1]);
    splitZehntel(capDiff, fanConfig.temp_cap_diff, fanTempZehntel[2]);
}

void loadFanConfig() {
    if (EEPROM.read(EEPROM_FAN_MAGIC_ADDR) == EEPROM_FAN_MAGIC_BYTE) {
        EEPROM.get(EEPROM_FAN_BASE, fanConfig);
    } else {
        fanConfig.mode           = FAN_MODE_MANUAL;
        fanConfig.manual_percent = 0;
        fanConfig.min_speed      = FAN_MIN_SPEED_DEFAULT;
        fanConfig.hum_min        = FAN_HUM_MIN_DEFAULT;
        fanConfig.hum_max        = FAN_HUM_MAX_DEFAULT;
        fanConfig.temp_min       = FAN_TEMP_MIN_DEFAULT;
        fanConfig.temp_max       = FAN_TEMP_MAX_DEFAULT;
        fanConfig.temp_mode      = FAN_TEMP_MODE_ABSOLUTE;
        fanConfig.temp_cap_diff  = 0;       // Bremse standardmaessig aus
        saveFanConfig();
    }
    for (int i = 0; i < 3; i++) {
        fanTempZehntel[i] = EEPROM.read(EEPROM_FAN_ZEHNTEL_BASE + i);   // vor 1.21.13 unbeschrieben
        if (fanTempZehntel[i] > 9) fanTempZehntel[i] = 0;
    }
    // Schalter "Eigene Steuerung aktiv" entfernt (1.21.5): der Master steuert den Luefter immer
    fanConfig.enabled = true;
    if (EEPROM.read(EEPROM_FAN_MAX_MAGIC_ADDR) == EEPROM_FAN_MAX_MAGIC_BYTE) {
        fanMaxSpeed = EEPROM.read(EEPROM_FAN_MAX_BASE);
        if (fanMaxSpeed > 100) fanMaxSpeed = FAN_MAX_SPEED_DEFAULT;
    } else {
        fanMaxSpeed = FAN_MAX_SPEED_DEFAULT;
        saveFanConfig();
    }
}

void saveFanConfig() {
    EEPROM.put(EEPROM_FAN_BASE, fanConfig);
    EEPROM.write(EEPROM_FAN_MAGIC_ADDR, EEPROM_FAN_MAGIC_BYTE);
    EEPROM.write(EEPROM_FAN_MAX_BASE, fanMaxSpeed);
    EEPROM.write(EEPROM_FAN_MAX_MAGIC_ADDR, EEPROM_FAN_MAX_MAGIC_BYTE);
    for (int i = 0; i < 3; i++) EEPROM.write(EEPROM_FAN_ZEHNTEL_BASE + i, fanTempZehntel[i]);
    EEPROM.commit();
}
