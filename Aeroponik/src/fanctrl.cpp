#include "fanctrl.h"
#include "config.h"
#include <EEPROM.h>

FanConfig fanConfig;
uint8_t   fanMaxSpeed = FAN_MAX_SPEED_DEFAULT;

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
        fanConfig.enabled        = false;  // sicherer Default: iControl behaelt Kontrolle bis aktiviert
        fanConfig.temp_mode      = FAN_TEMP_MODE_ABSOLUTE;
        fanConfig.temp_cap_diff  = 0;       // Bremse standardmaessig aus
        saveFanConfig();
    }
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
    EEPROM.commit();
}
