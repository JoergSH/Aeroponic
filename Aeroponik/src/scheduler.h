#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

struct LightScheduleConfig {
    bool     enabled;
    uint8_t  _reserved;    // war target_node, nicht mehr genutzt
    uint16_t dawn_start;   // Minuten seit Mitternacht (0–1439)
    uint16_t dawn_end;
    uint16_t dusk_start;
    uint16_t dusk_end;
    uint8_t  num_ssr;      // Anzahl SSRs (1–4)
};

extern LightScheduleConfig schedConfig;

void    loadScheduleConfig();
void    saveScheduleConfig();

// Gibt Relay-Maske (Bits 0–3 = SSR1–4) für die aktuelle Tageszeit zurück
uint8_t schedComputeRelayMask(uint16_t now_min);

// Gibt die Helligkeit (0–100 %) stufenlos für die aktuelle Tageszeit zurück — fuer den
// analogen 0-10V-Lichtausgang. Nutzt dieselben Zeitfenster wie schedComputeRelayMask(),
// aber ohne dessen Stufen-Quantisierung.
uint8_t schedComputeBrightnessPercent(uint16_t now_min);

#endif
