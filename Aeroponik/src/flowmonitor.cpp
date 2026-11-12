#include "flowmonitor.h"
#include "valves.h"
#include "flowsensor.h"
#include "notify.h"
#include "config.h"
#include "dbg.h"
#include <EEPROM.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

FlowMonitorConfig flowMonitorConfig;
FlowMonitorState  flowMonitorState;

static VentilZustand lastZustand[BEHAELTER_ANZAHL]  = {};   // = V_PAUSE
static uint32_t      cycleStartMs[BEHAELTER_ANZAHL] = {};
static float         aktivDauerS[BEHAELTER_ANZAHL]  = {};   // Ventil-offen-Zeit (ohne Nachlauf)

// EEPROM-Adresse des Sollwerts je Behaelter: 1-3 im alten Block (gleiche Offsets wie das
// fruehere Struct-Layout), 4 in einem eigenen Feld (siehe config.h). Feldweise statt als
// ganzes Struct: EEPROM.put(struct) schrieb 3 Fuellbytes ueber das Blockende hinaus bis in
// die Raumlicht-Konfiguration (550/551).
static int sollAddr(uint8_t i) {
    return (i < 3) ? EEPROM_FLOWMON_BASE + 4 + i * 4 : EEPROM_FLOWMON_SOLL4_ADDR;
}

void loadFlowMonitorConfig() {
    if (EEPROM.read(EEPROM_FLOWMON_MAGIC_ADDR) == EEPROM_FLOWMON_MAGIC_BYTE) {
        flowMonitorConfig.enabled          = EEPROM.read(EEPROM_FLOWMON_BASE) != 0;
        flowMonitorConfig.toleranz_prozent = EEPROM.read(EEPROM_FLOWMON_BASE + 16);
        for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) {
            float v;
            EEPROM.get(sollAddr(i), v);
            // Behaelter 4 war vorher unbelegt (0xFF = NaN) -> 0 = Sollwert wird gelernt
            flowMonitorConfig.erwartet_impulse_s[i] = (isfinite(v) && v >= 0.0f && v <= 1000.0f) ? v : 0.0f;
        }
    } else {
        flowMonitorConfig.enabled = false;
        for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) flowMonitorConfig.erwartet_impulse_s[i] = 0.0f;
        flowMonitorConfig.toleranz_prozent = 30;
        saveFlowMonitorConfig();
    }
    memset(&flowMonitorState, 0, sizeof(flowMonitorState));
}

void saveFlowMonitorConfig() {
    EEPROM.write(EEPROM_FLOWMON_BASE, flowMonitorConfig.enabled ? 1 : 0);
    EEPROM.write(EEPROM_FLOWMON_BASE + 16, flowMonitorConfig.toleranz_prozent);
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++)
        EEPROM.put(sollAddr(i), flowMonitorConfig.erwartet_impulse_s[i]);
    EEPROM.write(EEPROM_FLOWMON_MAGIC_ADDR, EEPROM_FLOWMON_MAGIC_BYTE);
    EEPROM.commit();
}

// Verarbeitet das Ende eines Giesszyklus: Impulse/Dauer fuers Log merken, bei aktivierter
// Ueberwachung die Impulsrate (Impulse pro Sekunde Ventil-offen-Zeit) gegen den Sollwert
// vergleichen und ggf. per sendNotification() alarmieren.
static void handleCycleEnd(uint8_t idx, uint32_t pulses, float dauer_s) {
    flowMonitorState.last_cycle_pulses[idx] = pulses;
    flowMonitorState.last_cycle_s[idx]      = dauer_s;
    flowMonitorState.log_pending[idx]       = true;

    dbgPrintf("[FLOWMON] Behaelter %u: %u Impulse in %.1fs\n", idx + 1, pulses, dauer_s);

    if (dauer_s < 0.5f) return;   // zu kurzer Zyklus fuer eine belastbare Aussage
    float rate = pulses / dauer_s;

    // Kein Sollwert fuer diesen Behaelter hinterlegt (0 = unkonfiguriert oder per "Sollwerte
    // loeschen" zurueckgesetzt, z.B. nach dem Duesenreinigen) -- diesen Zyklus automatisch als
    // neue Referenz uebernehmen. Laeuft immer, unabhaengig vom Alarm-Schalter, damit der
    // Sollwert schon bereitsteht sobald man den Alarm aktiviert.
    if (flowMonitorConfig.erwartet_impulse_s[idx] <= 0.0f) {
        if (pulses == 0) return;   // kein Durchfluss -- taugt nicht als Referenz
        flowMonitorConfig.erwartet_impulse_s[idx] = rate;
        saveFlowMonitorConfig();
        dbgPrintf("[FLOWMON] Behaelter %u: Sollwert automatisch ermittelt: %.1f Impulse/s\n", idx + 1, rate);
        return;  // dieser Zyklus ist die neue Referenz, nicht gegen sich selbst vergleichen
    }

    if (!flowMonitorConfig.enabled) return;

    float erwartet  = flowMonitorConfig.erwartet_impulse_s[idx] * dauer_s;
    float tolFactor = flowMonitorConfig.toleranz_prozent / 100.0f;
    char  msg[160];

    if (pulses < erwartet * (1.0f - tolFactor)) {
        snprintf(msg, sizeof(msg),
                 "⚠️ Behälter %u: Düse evtl. verstopft (erwartet ~%.0f Impulse, gemessen %u)",
                 idx + 1, erwartet, pulses);
        sendNotification(msg);
    } else if (pulses > erwartet * (1.0f + tolFactor)) {
        snprintf(msg, sizeof(msg),
                 "⚠️ Behälter %u: Zu viel Durchfluss, evtl. Leck (erwartet ~%.0f Impulse, gemessen %u)",
                 idx + 1, erwartet, pulses);
        sendNotification(msg);
    }
}

// Messfenster: Ventil auf bis Ende Nachlauf. Ein Druckspeicher hinter dem Durchflussmesser liefert
// beim Oeffnen einen Teil des Spruehwassers selbst und wird erst im Nachlauf (Ventil zu, Pumpe an)
// wieder aufgefuellt -- erst dann ist die gesamte gespruehte Menge durch den Zaehler gelaufen.
// Als Dauer (Sollwert Impulse/s) zaehlt nur die Ventil-offen-Zeit.
void loopFlowMonitor() {
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) {
        VentilZustand z = behaelterZustand[i].zustand;
        VentilZustand l = lastZustand[i];

        if (z == V_AKTIV && l != V_AKTIV) {
            // Zyklus beginnt -- Zaehler leeren (verwirft Reste aus dem Druckaufbau, die in den
            // Druckspeicher bzw. ueber den Ruecklauf-Kugelhahn liefen, nicht ueber diese Duese).
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
