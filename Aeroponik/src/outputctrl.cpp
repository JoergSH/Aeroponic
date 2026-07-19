#include "outputctrl.h"
#include "config.h"
#include <EEPROM.h>

OutputConfig outputConfig;

void loadOutputConfig() {
    if (EEPROM.read(EEPROM_OUTPUT_MAGIC_ADDR) == EEPROM_OUTPUT_MAGIC_BYTE) {
        EEPROM.get(EEPROM_OUTPUT_BASE, outputConfig);
    } else {
        outputConfig.fan_output   = FAN_OUTPUT_MARS;
        outputConfig.light_output = LIGHT_OUTPUT_LICHTX4;
        saveOutputConfig();
    }
}

void saveOutputConfig() {
    EEPROM.put(EEPROM_OUTPUT_BASE, outputConfig);
    EEPROM.write(EEPROM_OUTPUT_MAGIC_ADDR, EEPROM_OUTPUT_MAGIC_BYTE);
    EEPROM.commit();
}
