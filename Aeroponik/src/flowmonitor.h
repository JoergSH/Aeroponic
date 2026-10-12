#ifndef FLOWMONITOR_H
#define FLOWMONITOR_H

#include <stdint.h>
#include <stdbool.h>

// Duesen-Ueberwachung: beobachtet behaelterZustand[] (valves.h) selbststaendig von aussen --
// sobald ein Behaelter von/zu V_AKTIV wechselt, wird die in diesem Fenster gezaehlte
// Durchflussmenge (ueber den kalibrierten Durchflussmesser, siehe flowsensor.h) erfasst.
// Da laut loopVentile() immer nur ein Behaelter gleichzeitig giesst, reicht der eine
// gemeinsame Sensor in der Zulaufleitung vor dem Ventil-Verteiler.
struct FlowMonitorConfig {
    bool    enabled;                        // Soll/Ist-Vergleich + Alarm aktiv (Erfassung laeuft immer)
    float   erwarteter_durchfluss_ml_s[3];   // je Behaelter, ml/s bei sauberer Duese (0 = kein Vergleich)
    uint8_t toleranz_prozent;                // gemeinsame Toleranz, z.B. 30 = +/-30%
};

// Laufende Werte -- nur im RAM (seit letztem Neustart), um die Haeufung aus vielen Giesszyklen
// pro Tag nicht als EEPROM-Schreibzyklen zu verschleissen. Der dauerhafte Verlauf steht im
// SD-Karten-Log (storage.cpp), aus dem sich eine echte Lebenszeit-Summe bei Bedarf rekonstruieren
// liesse.
struct FlowMonitorState {
    float last_cycle_ml[3];   // letzter abgeschlossener Giesszyklus je Behaelter
    float total_ml[3];        // Summe je Behaelter seit letztem Neustart
    float total_ml_all;       // Gesamtsumme aller Behaelter seit letztem Neustart
};

extern FlowMonitorConfig flowMonitorConfig;
extern FlowMonitorState  flowMonitorState;

void loadFlowMonitorConfig();
void saveFlowMonitorConfig();

// Einmal pro loop() aufrufen (nach loopVentile()) -- rein beobachtend, keine eigene Taktung noetig.
void loopFlowMonitor();

#endif
