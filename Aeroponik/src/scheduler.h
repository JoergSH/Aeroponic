#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

struct LightScheduleConfig {
    bool     enabled;
    uint8_t  manual_percent;  // 0-100, genutzt wenn enabled==false (manuelle Vorgabe statt Rampe)
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

// Grobe Licht/Dunkel-Einordnung für vom Lichtstatus abhängige Logik (z. B. Bewässerung,
// siehe valves.cpp): bei aktivem Zeitplan alles zwischen dawn_start und dusk_end (also
// inklusive der Dämmerungsrampen — schon während des Sonnenaufgangs zählt es als "an").
// Bei deaktiviertem Zeitplan (manuelle Vorgabe) zählt jede Helligkeit > 0 % als "an".
bool schedLichtIstAn(uint16_t now_min);

#endif
