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

// Kombi-Licht (Hauptlampe 0-10V + Lichtx4-Zusatzkanaele, siehe LIGHT_OUTPUT_KOMBI_* in outputctrl.h)
struct LichtKombiConfig {
    uint8_t max_pct;   // 1-100: Hoechstwert der Hauptlampe (Tag), die Rampen laufen 0 -> max_pct
    uint8_t mask;      // Bits 0-3 = Lichtx4-Kanaele 1-4, die tagsueber zugeschaltet werden
};
extern LichtKombiConfig kombiConfig;
void saveKombiConfig();

// Kombi-Betrieb: analogPct = Hauptlampe (0..max_pct), mask = Lichtx4-Zusatzkanaele -- nur im
// Tag-Abschnitt (Ende Aufgang bis Beginn Untergang) an, waehrend der Rampen aus.
void schedComputeKombi(uint16_t now_min, uint8_t& analogPct, uint8_t& mask);
// Wie oben fuer die manuelle Vorgabe (Zeitplan aus): 0-100 % -> Hauptlampe 0..max_pct,
// Zusatzkanaele erst bei 100 %.
void schedComputeKombiManual(uint8_t manualPct, uint8_t& analogPct, uint8_t& mask);

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
