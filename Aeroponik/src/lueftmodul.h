#ifndef LUEFTMODUL_H
#define LUEFTMODUL_H

#include <stdint.h>
#include <stdbool.h>

// Zelt-Lüfter (Arctic PWM-Lüfter am RS485-Lüftermodul, Adresse 0x51):
// 600–3000 U/min, PWM-gesteuert; laut Hersteller steht der Lüfter unter 5 % PWM (0 U/min).
struct LueftmodulConfig {
  bool    enabled;          // Automatik/Anlage aktiv (aus = alle Kanäle 0 %)
  bool    gleiche_werte;    // true = ein gemeinsamer Wert für alle 4 Kanäle
  uint8_t percent[4];       // Sollwert je Kanal, 10 %-Schritte (0,10,…100)
  bool    monitor_enabled;  // Drehzahlüberwachung an/aus
  uint8_t monitor_tol_pct;  // großzügige Toleranz um den Erwartungswert (Prozent)
};

extern LueftmodulConfig lueftmodulConfig;

void loadLueftmodulConfig();
void saveLueftmodulConfig();

// Erwartete Drehzahl für einen PWM-Sollwert: 0 U/min unter 5 %, sonst linear von
// 600 U/min (bei 5 %) bis 3000 U/min (bei 100 %) — Arctic-Herstellerangabe.
uint16_t lueftmodulErwarteteRpm(uint8_t percent);

#endif
