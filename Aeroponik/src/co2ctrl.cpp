#include "co2ctrl.h"
#include "config.h"
#include <EEPROM.h>

Co2Config co2Config;

void loadCo2Config() {
    if (EEPROM.read(EEPROM_CO2_MAGIC_ADDR) == EEPROM_CO2_MAGIC_BYTE) {
        EEPROM.get(EEPROM_CO2_BASE, co2Config);
    } else {
        co2Config.co2_min          = CO2_MIN_DEFAULT;
        co2Config.co2_max          = CO2_MAX_DEFAULT;
        co2Config.enabled          = false;
        co2Config.target_node_id   = 0;
        co2Config.target_relay_bit = 0;
        saveCo2Config();
    }
}

void saveCo2Config() {
    EEPROM.put(EEPROM_CO2_BASE, co2Config);
    EEPROM.write(EEPROM_CO2_MAGIC_ADDR, EEPROM_CO2_MAGIC_BYTE);
    EEPROM.commit();
}
