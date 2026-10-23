#include "raumlichtctrl.h"
#include "config.h"
#include <EEPROM.h>

RaumlichtConfig raumlichtConfig;

void loadRaumlichtConfig() {
    if (EEPROM.read(EEPROM_RAUMLICHT_MAGIC_ADDR) == EEPROM_RAUMLICHT_MAGIC_BYTE) {
        EEPROM.get(EEPROM_RAUMLICHT_BASE, raumlichtConfig);
    } else {
        raumlichtConfig.target_node_id   = 0;
        raumlichtConfig.target_relay_bit = 0;
        saveRaumlichtConfig();
    }
}

void saveRaumlichtConfig() {
    EEPROM.put(EEPROM_RAUMLICHT_BASE, raumlichtConfig);
    EEPROM.write(EEPROM_RAUMLICHT_MAGIC_ADDR, EEPROM_RAUMLICHT_MAGIC_BYTE);
    EEPROM.commit();
}
