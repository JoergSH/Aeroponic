#include "flowmonitor.h"
#include "valves.h"
#include "flowsensor.h"
#include "notify.h"
#include "config.h"
#include "dbg.h"
#include <EEPROM.h>
#include <string.h>
#include <stdio.h>

FlowMonitorConfig flowMonitorConfig;
FlowMonitorState  flowMonitorState;

static VentilZustand lastZustand[3]  = { V_PAUSE, V_PAUSE, V_PAUSE };
static uint32_t      cycleStartMs[3] = { 0, 0, 0 };
static float         aktivDauerS[3]  = { 0, 0, 0 };   // Ventil-offen-Zeit (ohne Nachlauf)

void loadFlowMonitorConfig() {
    if (EEPROM.read(EEPROM_FLOWMON_MAGIC_ADDR) == EEPROM_FLOWMON_MAGIC_BYTE) {
        EEPROM.get(EEPROM_FLOWMON_BASE, flowMonitorConfig);
    } else {
        flowMonitorConfig.enabled = false;
        for (uint8_t i = 0; i < 3; i++) flowMonitorConfig.erwarteter_durchfluss_ml_s[i] = 0.0f;
        flowMonitorConfig.toleranz_prozent = 30;
        saveFlowMonitorConfig();
    }
    memset(&flowMonitorState, 0, sizeof(flowMonitorState));
}

void saveFlowMonitorConfig() {
    EEPROM.put(EEPROM_FLOWMON_BASE, flowMonitorConfig);
    EEPROM.write(EEPROM_FLOWMON_MAGIC_ADDR, EEPROM_FLOWMON_MAGIC_BYTE);
    EEPROM.commit();
}

// Verarbeitet das Ende eines Giesszyklus: Menge berechnen, Summen fortschreiben, bei
// aktivierter Ueberwachung Soll/Ist vergleichen und ggf. per sendNotification() alarmieren.
static void handleCycleEnd(uint8_t idx, uint32_t pulses, float dauer_s) {
    float ppl = flowsensor_pulses_per_liter();
    float ml  = (ppl > 0.0f) ? (pulses / ppl) * 1000.0f : 0.0f;

    flowMonitorState.last_cycle_ml[idx]      = ml;
    flowMonitorState.total_ml[idx]          += ml;
    flowMonitorState.total_ml_all           += ml;
    flowMonitorState.last_cycle_pulses[idx]  = pulses;
    flowMonitorState.last_cycle_s[idx]       = dauer_s;
    flowMonitorState.log_pending[idx]        = true;

    dbgPrintf("[FLOWMON] Behaelter %u: %.0f ml in %.1fs (%u Impulse)\n",
                  idx + 1, ml, dauer_s, pulses);

    if (ppl <= 0.0f) return;      // nicht kalibriert -- weder lernen noch vergleichen
    if (dauer_s < 0.5f) return;   // zu kurzer Zyklus fuer eine belastbare Aussage

    // Kein Sollwert fuer diesen Behaelter hinterlegt (0 = unkonfiguriert oder per "Sollwerte
    // loeschen" zurueckgesetzt, z.B. nach dem Duesenreinigen) -- diesen Zyklus automatisch als
    // neue Referenz uebernehmen, statt eine manuelle ml/s-Eingabe zu verlangen. Laeuft immer,
    // unabhaengig vom Alarm-Schalter, damit der Sollwert schon bereitsteht sobald man den Alarm
    // aktiviert.
    if (flowMonitorConfig.erwarteter_durchfluss_ml_s[idx] <= 0.0f) {
        float rate = ml / dauer_s;
        flowMonitorConfig.erwarteter_durchfluss_ml_s[idx] = rate;
        saveFlowMonitorConfig();
        dbgPrintf("[FLOWMON] Behaelter %u: Sollwert automatisch ermittelt: %.2f ml/s\n", idx + 1, rate);
        return;  // dieser Zyklus ist die neue Referenz, nicht gegen sich selbst vergleichen
    }

    if (!flowMonitorConfig.enabled) return;

    float erwartet   = flowMonitorConfig.erwarteter_durchfluss_ml_s[idx] * dauer_s;
    float tolFactor  = flowMonitorConfig.toleranz_prozent / 100.0f;
    char  msg[160];

    if (ml < erwartet * (1.0f - tolFactor)) {
        snprintf(msg, sizeof(msg),
                 "⚠️ Behälter %u: Düse evtl. verstopft (erwartet ~%.0fml, gemessen %.0fml)",
                 idx + 1, erwartet, ml);
        sendNotification(msg);
    } else if (ml > erwartet * (1.0f + tolFactor)) {
        snprintf(msg, sizeof(msg),
                 "⚠️ Behälter %u: Zu viel Durchfluss, evtl. Leck (erwartet ~%.0fml, gemessen %.0fml)",
                 idx + 1, erwartet, ml);
        sendNotification(msg);
    }
}

// Messfenster: Ventil auf bis Ende Nachlauf. Ein Druckspeicher hinter dem Durchflussmesser liefert
// beim Oeffnen einen Teil des Spruehwassers selbst und wird erst im Nachlauf (Ventil zu, Pumpe an)
// wieder aufgefuellt -- erst dann ist die gesamte gespruehte Menge durch den Zaehler gelaufen.
// Als Dauer (Sollwert ml/s) zaehlt nur die Ventil-offen-Zeit.
void loopFlowMonitor() {
    for (uint8_t i = 0; i < 3; i++) {
        VentilZustand z = behaelterZustand[i].zustand;
        VentilZustand l = lastZustand[i];

        if (z == V_AKTIV && l != V_AKTIV) {
            // Zyklus beginnt -- Zaehler leeren (verwirft Reste aus Vorlauf/Druckaufbau, die ueber
            // das Ruecklaufventil bzw. in den Druckspeicher liefen, nicht ueber diese Duese).
            flowsensor_take_pulses();
            cycleStartMs[i] = millis();
        } else if (l == V_AKTIV && z != V_AKTIV) {
            aktivDauerS[i] = (millis() - cycleStartMs[i]) / 1000.0f;
            if (z != V_NACHLAUF) handleCycleEnd(i, flowsensor_take_pulses(), aktivDauerS[i]);
        } else if (l == V_NACHLAUF && z != V_NACHLAUF) {
            handleCycleEnd(i, flowsensor_take_pulses(), aktivDauerS[i]);
        }

        lastZustand[i] = z;
    }
}
