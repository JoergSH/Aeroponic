#include "wifictrl.h"
#include "config.h"
#include <EEPROM.h>
#include <string.h>

WifiConfig wifiConfig;

void loadWifiConfig() {
    if (EEPROM.read(EEPROM_WIFI_MAGIC_ADDR) == EEPROM_WIFI_MAGIC_BYTE) {
        EEPROM.get(EEPROM_WIFI_BASE, wifiConfig);
    } else {
        // Kein gespeichertes WLAN vorhanden (Erststart) — leer lassen, Fallback-AP
        // uebernimmt und erlaubt die Konfiguration ueber das Webinterface.
        memset(&wifiConfig, 0, sizeof(wifiConfig));
        saveWifiConfig();
    }
}

void saveWifiConfig() {
    EEPROM.put(EEPROM_WIFI_BASE, wifiConfig);
    EEPROM.write(EEPROM_WIFI_MAGIC_ADDR, EEPROM_WIFI_MAGIC_BYTE);
    EEPROM.commit();
}
