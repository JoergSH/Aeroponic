#ifndef GEHAEUSEFAN_H
#define GEHAEUSEFAN_H

#include <stdint.h>
#include <stdbool.h>

// Gehaeuseluefter: einfacher An/Aus-Schalter (kein PWM — dieser Lüftertyp kommt mit
// PWM-Drosselung nicht klar) an GEHAEUSE_FAN_PIN, geregelt ueber den Innentemperatur-
// DS18B20 der Steuerung. Hysterese: an ab temp_max, aus ab/unter temp_min — dazwischen
// bleibt der zuletzt gesetzte Zustand erhalten, damit der Lüfter nicht dauernd taktet.
struct GehaeuseFanConfig {
    bool    enabled;
    uint8_t temp_min;    // °C — ab hier (und darunter) Lüfter aus
    uint8_t temp_max;    // °C — ab hier Lüfter an
};

extern GehaeuseFanConfig gehaeuseFanConfig;

void loadGehaeuseFanConfig();
void saveGehaeuseFanConfig();

#endif
