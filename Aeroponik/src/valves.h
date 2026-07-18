#ifndef VALVES_H
#define VALVES_H

#include <Arduino.h>

// ========== Konfiguration pro Behälter ==========
struct BehaelterConfig {
  uint8_t  oeffnungszeit_s;   // Düsenventil offen (Sekunden)
  uint8_t  pausenzeit_min;    // Wartezeit bis nächster Zyklus (Minuten)
  bool     aktiv;             // Behälter aktiv
};

// ========== Gesamte Ventilkonfiguration ==========
struct VentilConfig {
  BehaelterConfig behaelter[3];
  uint8_t  vorlaufzeit_s;    // Rücklaufventil Vorlaufzeit (Sekunden)
  bool     vorlauf_aktiv;    // Vorlauf aktiviert
};

// ========== Zustand pro Behälter ==========
enum VentilZustand { V_PAUSE, V_VORLAUF, V_AKTIV };

struct BehaelterZustand {
  VentilZustand zustand;
  unsigned long timerStart;
};

// Globale Instanzen
extern VentilConfig    ventilConfig;
extern BehaelterZustand behaelterZustand[3];

// Funktionen
void setupVentile();
void loopVentile();
void saveVentilConfig();
void loadVentilConfig();

// Aktueller Status für API
const char* ventilZustandText(VentilZustand z);
bool        ventilRueckOffen();

// PCF8574 Direkttest (pausiert automatische Ventilsteuerung)
void    pcf_test_mode(bool enable);
void    pcf_test_set(uint8_t pin, bool on);
bool    pcf_test_active();
uint8_t pcf_get_state();

#endif
