#include "tank.h"
#include "config.h"
#include <EEPROM.h>

TankConfig tankConfig;

void saveTankConfig() {
    EEPROM.put(EEPROM_TANK_BASE, tankConfig);
    EEPROM.put(EEPROM_TANK_MAGIC_ADDR, (uint8_t)EEPROM_TANK_MAGIC_BYTE);
    EEPROM.commit();
    Serial.println("Tankkonfig gespeichert");
}

void loadTankConfig() {
    uint8_t magic;
    EEPROM.get(EEPROM_TANK_MAGIC_ADDR, magic);
    if (magic == EEPROM_TANK_MAGIC_BYTE) {
        EEPROM.get(EEPROM_TANK_BASE, tankConfig);
        Serial.println("Tankkonfig geladen");
    } else {
        tankConfig.hoehe_mm        = TANK_HOEHE_MM;
        tankConfig.radius_unten_mm = TANK_RADIUS_UNTEN_MM;
        tankConfig.radius_oben_mm  = TANK_RADIUS_OBEN_MM;
        tankConfig.leer_abstand_mm = SENSOR_LEER_ABSTAND_MM;
        tankConfig.min_mm          = SENSOR_MIN_MM;
        tankConfig.max_mm          = SENSOR_MAX_MM;
        tankConfig.offset_mm       = SENSOR_OFFSET_MM;
        saveTankConfig();
        Serial.println("Tankkonfig: Standardwerte");
    }
}
