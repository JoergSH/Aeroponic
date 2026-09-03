#ifndef VALVES_H
#define VALVES_H

#include <Arduino.h>

// ========== Konfiguration pro Behälter ==========
// Öffnungszeit/Pausenzeit gelten für die Lichtphase; für die Dunkelphase (siehe
// schedLichtIstAn() in scheduler.h) gibt es ein eigenes, kürzer/länger einstellbares
// Zeitpaar — siehe LPA_Betriebsprotokoll.md Punkt 2/3 (Sprühintervalle hängen vom
// Lichtstatus ab, z. B. seltener/kürzer sprühen wenn die Lampen aus sind).
struct BehaelterConfig {
  uint8_t  oeffnungszeit_s;         // Düsenventil offen (Sekunden) — Lichtphase
  uint8_t  pausenzeit_min;          // Wartezeit bis nächster Zyklus (Minuten) — Lichtphase
  bool     aktiv;                   // Behälter aktiv
  uint8_t  oeffnungszeit_s_dunkel;  // wie oeffnungszeit_s, aber während der Dunkelphase
  uint8_t  pausenzeit_min_dunkel;   // wie pausenzeit_min, aber während der Dunkelphase
};

// ========== Gesamte Ventilkonfiguration ==========
struct VentilConfig {
  BehaelterConfig behaelter[3];
  uint8_t  vorlaufzeit_s;    // Rücklaufventil Vorlaufzeit (Sekunden)
  bool     vorlauf_aktiv;    // Vorlauf aktiviert
  bool     gleiche_zeiten;   // "Gleiche Zeiten für alle": ein gemeinsamer Batch-Durchlauf
                              // (Vorlauf nur einmal vor dem ersten aktiven Behälter, dann
                              // alle aktiven Behälter direkt nacheinander, eine gemeinsame
                              // Pause) statt 3 unabhängiger Timer — siehe loopVentile().
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
// lichtAn: aktueller Lichtstatus (siehe schedLichtIstAn() in scheduler.h) — bestimmt, ob
// die Licht- oder die Dunkelphase-Zeiten je Behälter verwendet werden.
void loopVentile(bool lichtAn);
void saveVentilConfig();
void loadVentilConfig();

// Setzt alle Behaelter/Ventile in einen sauberen Ruhezustand (Pause, alle Ausgaenge aus)
// und startet auch den Batch-Zustand neu — aufzurufen wenn gleiche_zeiten umgeschaltet
// wird, damit kein Ventil aus dem vorherigen Modus offen haengen bleibt.
void resetVentilRuntimeState();

// Aktueller Status für API
const char* ventilZustandText(VentilZustand z);
bool        ventilRueckOffen();

// Pumpe: automatisch an, sobald irgendein Behälter gerade aktiv wässert (Ventil offen).
bool pumpeAktiv();

// ========== Umwälzpumpe (Vorratsbehälter-Zirkulation) ==========
// Läuft automatisch im konfigurierbaren Lauf-/Pausenzyklus, sobald irgendein Behälter
// aktiviert ist (Konfig-Haken) — unabhängig davon, ob dieser gerade tatsächlich wässert
// oder nur zwischen zwei Zyklen pausiert. Ist kein Behälter aktiviert, bleibt sie aus —
// siehe loopVentile().
struct UmwaelzConfig {
  bool     aktiv;           // Automatik ein/aus
  uint8_t  laufzeit_s;      // Laufzeit pro Zyklus (Sekunden)
  uint8_t  pausenzeit_min;  // Pause zwischen den Zyklen (Minuten)
};
extern UmwaelzConfig umwaelzConfig;
bool umwaelzpumpeAktiv();   // aktueller Ausgangszustand (an/aus)
void saveUmwaelzConfig();
void loadUmwaelzConfig();

// Ausgangs-Direkttest (pausiert automatische Ventil-/Pumpensteuerung) zur
// Verkabelungspruefung. Index 0-3 = Ventil 1-4 (Behälter 1-3 + Rücklauf),
// 4 = Umwälzpumpe, 5 = Pumpe — siehe OUTPUT_TEST_COUNT/output_test_name().
#define OUTPUT_TEST_COUNT 6
void        output_test_mode(bool enable);
void        output_test_set(uint8_t index, bool on);
bool        output_test_active();
uint8_t     output_test_get_state();   // Bitmaske, Bit i = Ausgang i an/aus
const char* output_test_name(uint8_t index);

#endif
