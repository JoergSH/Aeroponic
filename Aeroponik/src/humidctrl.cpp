#include "humidctrl.h"
#include "config.h"
#include <EEPROM.h>

HumidConfig humidConfig;

void loadHumidConfig() {
    if (EEPROM.read(EEPROM_HUMID_MAGIC_ADDR) == EEPROM_HUMID_MAGIC_BYTE) {
        EEPROM.get(EEPROM_HUMID_BASE, humidConfig);
    } else {
        humidConfig.humid_min        = HUMID_MIN_DEFAULT;
        humidConfig.humid_max        = HUMID_MAX_DEFAULT;
        humidConfig.enabled          = false;
        humidConfig.target_node_id   = 0;
        humidConfig.target_relay_bit = 0;
        saveHumidConfig();
    }
}

void saveHumidConfig() {
    EEPROM.put(EEPROM_HUMID_BASE, humidConfig);
    EEPROM.write(EEPROM_HUMID_MAGIC_ADDR, EEPROM_HUMID_MAGIC_BYTE);
    EEPROM.commit();
}
