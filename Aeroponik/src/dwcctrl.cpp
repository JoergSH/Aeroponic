#include "dwcctrl.h"
#include "config.h"
#include <EEPROM.h>

DwcTimerConfig dwcConfig;

void loadDwcConfig() {
    if (EEPROM.read(EEPROM_DWC_MAGIC_ADDR) == EEPROM_DWC_MAGIC_BYTE) {
        EEPROM.get(EEPROM_DWC_BASE, dwcConfig);
    } else {
        dwcConfig.enabled          = false;
        dwcConfig.target_node_id   = 0;
        dwcConfig.target_relay_bit = 0;
        dwcConfig.on_min           = 360;   // 06:00
        dwcConfig.off_min          = 1320;  // 22:00 (16h Photoperiode als Default)
        saveDwcConfig();
    }
}

void saveDwcConfig() {
    EEPROM.put(EEPROM_DWC_BASE, dwcConfig);
    EEPROM.write(EEPROM_DWC_MAGIC_ADDR, EEPROM_DWC_MAGIC_BYTE);
    EEPROM.commit();
}
