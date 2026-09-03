#include "ds18b20cfg.h"
#include "config.h"
#include <EEPROM.h>
#include <string.h>

Ds18b20Config ds18b20Config;

void loadDs18b20Config() {
    if (EEPROM.read(EEPROM_DS18B20_MAGIC_ADDR) == EEPROM_DS18B20_MAGIC_BYTE) {
        EEPROM.get(EEPROM_DS18B20_BASE, ds18b20Config);
    } else {
        memset(&ds18b20Config, 0, sizeof(ds18b20Config));  // alle Rollen unzugewiesen
        saveDs18b20Config();
    }
}

void saveDs18b20Config() {
    EEPROM.put(EEPROM_DS18B20_BASE, ds18b20Config);
    EEPROM.write(EEPROM_DS18B20_MAGIC_ADDR, EEPROM_DS18B20_MAGIC_BYTE);
    EEPROM.commit();
}
