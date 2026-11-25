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
  uint8_t  menge_imp;               // Soll-Impulse je Sprühvorgang (nur bei nach_menge) — Lichtphase
  uint8_t  menge_imp_dunkel;        // wie menge_imp, aber während der Dunkelphase
};

// Anzahl Behaelter = Duesenventile 1-4. Ventil 4 war frueher das Ruecklaufventil; die
// Rueckleitung ist seit 11/2026 ein Handventil (Kugelhahn), der Ruecklauf-Vorlauf entfaellt.
#define BEHAELTER_ANZAHL 4

// ========== Gesamte Ventilkonfiguration ==========
struct VentilConfig {
  BehaelterConfig behaelter[BEHAELTER_ANZAHL];
  bool     gleiche_zeiten;   // "Gleiche Zeiten für alle": ein gemeinsamer Batch-Durchlauf
                              // (alle aktiven Behälter direkt nacheinander, eine gemeinsame
                              // Pause) statt unabhängiger Timer — siehe loopVentile().
  uint8_t  druckaufbau_s;    // Pumpe läuft so lange VOR dem Öffnen des Düsenventils und füllt den
                              // Druckspeicher (0 = aus). Beim Öffnen geht sie aus -- gesprüht wird
                              // aus dem Speicher, damit kein Anlaufen der Pumpe in die Impulse
                              // gerät. Kein Nachlauf mehr. Bei 0 läuft die Pumpe während des Sprühens.
  bool     nach_menge;       // Dosierung nach Menge: Düsenventil schließt, sobald seit dem Öffnen
                              // menge_imp Durchfluss-Impulse gezählt sind (statt nach
                              // oeffnungszeit_s). Die dabei gemessene Laufzeit überwacht
                              // flowmonitor.cpp; Sicherheitslimit siehe ventilMengeLimitMs().
};

// ========== Zustand pro Behälter ==========
// Ablauf: Pause → [Druckaufbau, Pumpe an] → Aktiv (Pumpe aus) → Pause
// (V_NACHLAUF gibt es nicht mehr, der Wert bleibt nur fuer die Anzeige/Kompatibilitaet)
enum VentilZustand { V_PAUSE, V_AKTIV, V_DRUCKAUFBAU, V_NACHLAUF };

struct BehaelterZustand {
  VentilZustand zustand;
  unsigned long timerStart;
};

// Globale Instanzen
extern VentilConfig    ventilConfig;
extern BehaelterZustand behaelterZustand[BEHAELTER_ANZAHL];

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

// Mengendosierung: Soll-Impulse fuer die aktuelle Phase und das Sicherheitslimit der
// Ventil-offen-Zeit (2x erwartete Laufzeit aus dem gelernten Sollwert, siehe config.h).
uint8_t  ventilSollImpulse(uint8_t i, bool lichtAn);
uint32_t ventilMengeLimitMs(uint8_t i, uint8_t sollImp);
// true, wenn der letzte Sprühvorgang dieses Behälters am Sicherheitslimit abgebrochen wurde
// (Sollmenge nicht erreicht) -- gilt bis zum nächsten Öffnen des Ventils.
bool     ventilMengeAbgebrochen(uint8_t i);

// Aktueller Status für API
const char* ventilZustandText(VentilZustand z);

// Pumpe: automatisch an, sobald irgendein Behälter im Druckaufbau/Nachlauf ist oder gerade
// aktiv wässert (Ventil offen).
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
// Verkabelungspruefung. Index 0-3 = Ventil 1-4 (Behälter 1-4),
// 4 = Umwälzpumpe, 5 = Pumpe — siehe OUTPUT_TEST_COUNT/output_test_name().
#define OUTPUT_TEST_COUNT 6
// keepUmwaelz = true (manueller Einzelzyklus): Umwaelzpumpe bleibt in ihrem Automatik-Takt und
// verzoegert den Start nicht.
void        output_test_mode(bool enable, bool keepUmwaelz = false);
void        output_test_set(uint8_t index, bool on);
bool        output_test_active();
// true, solange output_test_mode(true) angefordert wurde, aber noch auf das Ende eines gerade
// laufenden automatischen Zyklus wartet (kein Ventil wird mitten im Giessen unterbrochen) --
// siehe loopVentile(). Wird automatisch false, sobald der Test wirklich aktiv wird oder
// output_test_mode(false) die Anfrage zurueckzieht.
bool        output_test_pending();
uint8_t     output_test_get_state();   // Bitmaske, Bit i = Ausgang i an/aus
const char* output_test_name(uint8_t index);

#endif
