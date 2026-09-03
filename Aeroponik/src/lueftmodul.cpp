#include "lueftmodul.h"
#include "config.h"
#include <EEPROM.h>

LueftmodulConfig lueftmodulConfig;

void loadLueftmodulConfig() {
    if (EEPROM.read(EEPROM_LUEFTMODUL_MAGIC_ADDR) == EEPROM_LUEFTMODUL_MAGIC_BYTE) {
        EEPROM.get(EEPROM_LUEFTMODUL_BASE, lueftmodulConfig);
    } else {
        lueftmodulConfig.enabled         = false;
        lueftmodulConfig.gleiche_werte   = true;
        for (int i = 0; i < 4; i++) lueftmodulConfig.percent[i] = 0;
        lueftmodulConfig.monitor_enabled = false;
        lueftmodulConfig.monitor_tol_pct = LUEFTMODUL_MONITOR_TOL_DEFAULT;
        saveLueftmodulConfig();
    }
}

void saveLueftmodulConfig() {
    EEPROM.put(EEPROM_LUEFTMODUL_BASE, lueftmodulConfig);
    EEPROM.write(EEPROM_LUEFTMODUL_MAGIC_ADDR, EEPROM_LUEFTMODUL_MAGIC_BYTE);
    EEPROM.commit();
}

uint16_t lueftmodulErwarteteRpm(uint8_t percent) {
    if (percent < 5) return 0;
    return (uint16_t)(600 + (uint32_t)(percent - 5) * (3000 - 600) / (100 - 5));
}
