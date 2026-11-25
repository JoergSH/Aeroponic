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

    // Mengendosierung: Sollmenge nicht erreicht, Ventil am Sicherheitslimit geschlossen. Immer
    // melden (auch bei ausgeschaltetem Alarm -- der Behaelter hat dann zu wenig bekommen), aber
    // nur einmal pro Abbruch-Serie, damit nicht jeder Zyklus eine Nachricht schickt.
    static bool abbruchGemeldet[BEHAELTER_ANZAHL] = {};
    bool abbruch = ventilConfig.nach_menge && ventilMengeAbgebrochen(idx);
    if (abbruch) {
        if (!abbruchGemeldet[idx]) {
            char m[200];
            snprintf(m, sizeof(m),
                     "⚠️ Behälter %u: Sollmenge nicht erreicht – Ventil nach %.0f s geschlossen (%u Impulse). Düse oder Durchflussmesser prüfen!",
                     idx + 1, dauer_s, pulses);
            sendNotification(m);
            abbruchGemeldet[idx] = true;
        }
    } else if (abbruchGemeldet[idx]) {
        abbruchGemeldet[idx] = false;
        dbgPrintf("[FLOWMON] Behaelter %u: Sollmenge wieder erreicht\n", idx + 1);
    }

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
    if (abbruch) return;   // schon gemeldet (s.o.), kein zweiter Alarm fuer denselben Zyklus

    float tolFactor = flowMonitorConfig.toleranz_prozent / 100.0f;
    char  msg[200];

    // Mengendosierung: die Impulszahl ist fest vorgegeben -- ueberwacht wird die Laufzeit, die
    // fuer diese Menge gebraucht wurde (laenger = Duese setzt zu, kuerzer = Leck).
    if (ventilConfig.nach_menge) {
        float sollDauer = pulses / flowMonitorConfig.erwartet_impulse_s[idx];
        if (dauer_s > sollDauer * (1.0f + tolFactor)) {
            snprintf(msg, sizeof(msg),
                     "⚠️ Behälter %u: Düse evtl. verstopft (Laufzeit %.1f s statt ~%.1f s für %u Impulse)",
                     idx + 1, dauer_s, sollDauer, pulses);
            sendNotification(msg);
        } else if (dauer_s < sollDauer * (1.0f - tolFactor)) {
            snprintf(msg, sizeof(msg),
                     "⚠️ Behälter %u: Zu viel Durchfluss, evtl. Leck (Laufzeit %.1f s statt ~%.1f s für %u Impulse)",
                     idx + 1, dauer_s, sollDauer, pulses);
            sendNotification(msg);
        }
        return;
    }

    float erwartet  = flowMonitorConfig.erwartet_impulse_s[idx] * dauer_s;

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

// Messfenster: nur solange das Duesenventil offen ist (V_AKTIV). Impulse ausserhalb davon gehoeren
// nicht zu dieser Duese und werden verworfen. Gleiches Fenster wie bei der Mengendosierung
// (valves.cpp: Impulse ab Ventil-auf bis Sollmenge).
// Diagnose-Spur: jede Sekunde, in der Impulse kamen oder Pumpe/Ventile aktiv sind, eine Zeile
// "[FLOW] +n Imp | Pumpe an/aus | B2 Aktiv" -- zeigt, in welcher Phase der Durchfluss wirklich
// auftritt (auch Impulse in der Pause oder im verworfenen Druckaufbau werden so sichtbar).
static void flowTrace() {
    static uint32_t lastMs = 0, lastTotal = 0;
    uint32_t now = millis();
    if (now - lastMs < 1000) return;
    lastMs = now;
    uint32_t total = flowsensor_total_pulses();
    uint32_t delta = total - lastTotal;
    lastTotal = total;

    char phase[64] = "";
    size_t len = 0;
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) {
        VentilZustand z = behaelterZustand[i].zustand;
        if (z == V_PAUSE) continue;
        len += snprintf(phase + len, sizeof(phase) - len, " B%u %s", i + 1, ventilZustandText(z));
        if (len >= sizeof(phase)) break;
    }
    if (delta == 0 && !pumpeAktiv() && phase[0] == 0) return;
    dbgPrintf("[FLOW] +%u Imp | Pumpe %s |%s\n", delta, pumpeAktiv() ? "an" : "aus",
              phase[0] ? phase : " Pause");
}

void loopFlowMonitor() {
    flowTrace();
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) {
        VentilZustand z = behaelterZustand[i].zustand;
        VentilZustand l = lastZustand[i];

        if (z == V_AKTIV && l != V_AKTIV) {
            // Zyklus beginnt -- Zaehler leeren (alles vor Ventil-auf gehoert nicht zu dieser Duese).
            uint32_t verworfen = flowsensor_take_pulses();
            if (verworfen) dbgPrintf("[FLOWMON] Behaelter %u: %u Impulse vor Ventil-auf verworfen\n", i + 1, verworfen);
            cycleStartMs[i] = millis();
        } else if (l == V_AKTIV && z != V_AKTIV) {
            // Ventil zu: Zyklus sofort auswerten, nur die Impulse der Ventil-offen-Zeit
            aktivDauerS[i] = (millis() - cycleStartMs[i]) / 1000.0f;
            handleCycleEnd(i, flowsensor_take_pulses(), aktivDauerS[i]);
        } else if (l == V_NACHLAUF && z != V_NACHLAUF) {
            uint32_t nach = flowsensor_take_pulses();   // Nachlauf: nur protokollieren, nicht zaehlen
            if (nach) dbgPrintf("[FLOWMON] Behaelter %u: %u Impulse im Nachlauf verworfen\n", i + 1, nach);
        }

        lastZustand[i] = z;
    }
}
