#include "scheduler.h"
#include "config.h"
#include <EEPROM.h>
#include <math.h>

LightScheduleConfig schedConfig;

void loadScheduleConfig() {
    if (EEPROM.read(EEPROM_SCHED_MAGIC_ADDR) == EEPROM_SCHED_MAGIC_BYTE) {
        EEPROM.get(EEPROM_SCHED_BASE, schedConfig);
    } else {
        schedConfig.enabled     = false;
        schedConfig._reserved   = 0;
        schedConfig.dawn_start  = SCHED_DEFAULT_DAWN_START;
        schedConfig.dawn_end    = SCHED_DEFAULT_DAWN_END;
        schedConfig.dusk_start  = SCHED_DEFAULT_DUSK_START;
        schedConfig.dusk_end    = SCHED_DEFAULT_DUSK_END;
        schedConfig.num_ssr     = SCHED_DEFAULT_NUM_SSR;
        saveScheduleConfig();
    }
}

void saveScheduleConfig() {
    EEPROM.put(EEPROM_SCHED_BASE, schedConfig);
    EEPROM.write(EEPROM_SCHED_MAGIC_ADDR, EEPROM_SCHED_MAGIC_BYTE);
    EEPROM.commit();
}

uint8_t schedComputeRelayMask(uint16_t now_min) {
    uint8_t n = schedConfig.num_ssr;
    if (n == 0 || n > 4) n = 4;

    if (now_min < schedConfig.dawn_start || now_min >= schedConfig.dusk_end)
        return 0x00;
    if (now_min >= schedConfig.dawn_end && now_min < schedConfig.dusk_start)
        return (uint8_t)((1 << n) - 1);  // alle n SSRs an

    // e = Fensterfortschritt 0.0 (Fensterbeginn) → 1.0 (Fensterende), getrennt fuer
    // Auf-/Untergang berechnet (NICHT dieselbe Formel fuer beide — das war der Bug:
    // eine gemeinsame "t"-Variable mit ceil() macht die erste Auf-Stufe zwar sofort
    // scharf, laesst aber beim Untergang die erste Ab-Stufe symmetrisch falsch erst
    // nach 1/n des Fensters greifen, weil ceil(n-x) mathematisch n-floor(x) entspricht,
    // nicht n-ceil(x)).
    uint8_t active;
    if (now_min < schedConfig.dawn_end) {
        // Aufgang: Stufe 1 zuendet sofort nach Fensterbeginn, Stufe n exakt am Ende.
        float e = (float)(now_min - schedConfig.dawn_start) /
                  (float)(schedConfig.dawn_end - schedConfig.dawn_start);
        if (e < 0.0f) e = 0.0f;
        if (e > 1.0f) e = 1.0f;
        active = (uint8_t)ceilf(e * n);
    } else {
        // Untergang: erste Stufe dimmt sofort nach Fensterbeginn ab, 0 exakt am Ende.
        float e = (float)(now_min - schedConfig.dusk_start) /
                  (float)(schedConfig.dusk_end - schedConfig.dusk_start);
        if (e < 0.0f) e = 0.0f;
        if (e > 1.0f) e = 1.0f;
        uint8_t drop = (uint8_t)ceilf(e * n);
        active = (drop >= n) ? 0 : n - drop;
    }
    if (active > n) active = n;
    return (active == 0) ? 0x00 : (uint8_t)((1 << active) - 1);
}
