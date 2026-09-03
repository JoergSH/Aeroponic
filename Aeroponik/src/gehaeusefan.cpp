#include "gehaeusefan.h"
#include "config.h"
#include <EEPROM.h>

GehaeuseFanConfig gehaeuseFanConfig;

void loadGehaeuseFanConfig() {
    if (EEPROM.read(EEPROM_GEHFAN_MAGIC_ADDR) == EEPROM_GEHFAN_MAGIC_BYTE) {
        EEPROM.get(EEPROM_GEHFAN_BASE, gehaeuseFanConfig);
    } else {
        gehaeuseFanConfig.enabled   = true;
        gehaeuseFanConfig.temp_min  = 35;   // °C
        gehaeuseFanConfig.temp_max  = 50;   // °C
        saveGehaeuseFanConfig();
    }
}

void saveGehaeuseFanConfig() {
    EEPROM.put(EEPROM_GEHFAN_BASE, gehaeuseFanConfig);
    EEPROM.write(EEPROM_GEHFAN_MAGIC_ADDR, EEPROM_GEHFAN_MAGIC_BYTE);
    EEPROM.commit();
}
