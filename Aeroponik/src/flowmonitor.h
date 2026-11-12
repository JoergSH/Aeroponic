#ifndef FLOWMONITOR_H
#define FLOWMONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "valves.h"   // BEHAELTER_ANZAHL

// Duesen-Ueberwachung: beobachtet behaelterZustand[] (valves.h) selbststaendig von aussen --
// sobald ein Behaelter von/zu V_AKTIV wechselt (inkl. folgendem V_NACHLAUF), wird die gezaehlte
// Impulszahl des Durchflussmessers (siehe flowsensor.h) erfasst -- roh, ohne Umrechnung in ml.
// Da laut loopVentile() immer nur ein Behaelter gleichzeitig giesst, reicht der eine
// gemeinsame Sensor in der Zulaufleitung vor dem Ventil-Verteiler.
struct FlowMonitorConfig {
    bool    enabled;                        // Soll/Ist-Vergleich + Alarm aktiv (Erfassung laeuft immer)
    float   erwartet_impulse_s[BEHAELTER_ANZAHL];   // je Behaelter, Impulse/s bei sauberer Duese (0 = wird gelernt)
    uint8_t toleranz_prozent;                // gemeinsame Toleranz, z.B. 30 = +/-30%
};

// Laufende Werte -- nur im RAM: Rohimpulse + Dauer des letzten Sprühvorgangs je Behaelter fuers
// SD-Log und die Anzeige. log_pending = seit der letzten Log-Zeile neu.
struct FlowMonitorState {
    uint32_t last_cycle_pulses[BEHAELTER_ANZAHL];
    float    last_cycle_s[BEHAELTER_ANZAHL];
    bool     log_pending[BEHAELTER_ANZAHL];
};

extern FlowMonitorConfig flowMonitorConfig;
extern FlowMonitorState  flowMonitorState;

void loadFlowMonitorConfig();
void saveFlowMonitorConfig();

// Einmal pro loop() aufrufen (nach loopVentile()) -- rein beobachtend, keine eigene Taktung noetig.
void loopFlowMonitor();

#endif
