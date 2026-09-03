#include "valves.h"
#include "config.h"
#include "pinout.h"
#include "dbg.h"
#include <EEPROM.h>

VentilConfig     ventilConfig;
BehaelterZustand behaelterZustand[3];
UmwaelzConfig    umwaelzConfig;

static bool testMode = false;

// Alle Ausgaenge (Ventile, Pumpe, Umwälzpumpe) sind direkte GPIO, kein I2C-Expander
// (PCF8574) mehr — siehe OUTPUT_ACTIVE_LOW in pinout.h fuer die Treiber-Polaritaet.
static void setOut(uint8_t pin, bool on) {
#if OUTPUT_ACTIVE_LOW
    digitalWrite(pin, on ? LOW : HIGH);
#else
    digitalWrite(pin, on ? HIGH : LOW);
#endif
}

static const uint8_t VENTIL_PIN[3] = { VENTIL1_PIN, VENTIL2_PIN, VENTIL3_PIN };

static bool rueckOffenState = false;
static bool pumpeState      = false;
static bool umwaelzState    = false;

static void setVentil(uint8_t pin, bool on) {
    if (pin == VENTIL4_PIN) rueckOffenState = on;
    setOut(pin, on);
}

static void setPumpeOut(bool on)   { pumpeState = on;   setOut(PUMPE_GPIO, on); }
static void setUmwaelzOut(bool on) { umwaelzState = on; setOut(UMWAELZPUMPE_GPIO, on); }

bool ventilRueckOffen()   { return rueckOffenState; }
bool pumpeAktiv()         { return pumpeState; }
bool umwaelzpumpeAktiv()  { return umwaelzState; }

// ========== Umwälzpumpe (Vorratsbehälter-Zirkulation) ==========
// Lauf-/Pausenzyklus im konfigurierten Takt; pausiert (Zyklus haelt an, Timer wird beim
// Wiederanlaufen neu gestartet), solange KEIN Behälter aktiviert (in der Konfiguration
// eingeschaltet) ist — unabhaengig davon, ob dieser Behälter gerade tatsaechlich waessert
// oder nur zwischen zwei Zyklen pausiert.
enum UmwaelzZustand { U_PAUSE, U_LAUF };
static UmwaelzZustand umwaelzZustand    = U_PAUSE;
static unsigned long  umwaelzTimerStart = 0;

static void loopUmwaelzpumpe(bool behaelterEnabled) {
    unsigned long now = millis();

    if (!umwaelzConfig.aktiv || !behaelterEnabled) {
        if (umwaelzState) setUmwaelzOut(false);
        umwaelzZustand    = U_PAUSE;
        umwaelzTimerStart = now;
        return;
    }

    unsigned long vergangen = now - umwaelzTimerStart;
    switch (umwaelzZustand) {
        case U_PAUSE:
            if (vergangen >= (unsigned long)umwaelzConfig.pausenzeit_min * 60000UL) {
                umwaelzZustand    = U_LAUF;
                umwaelzTimerStart = now;
                setUmwaelzOut(true);
            }
            break;
        case U_LAUF:
            if (vergangen >= (unsigned long)umwaelzConfig.laufzeit_s * 1000UL) {
                umwaelzZustand    = U_PAUSE;
                umwaelzTimerStart = now;
                setUmwaelzOut(false);
            }
            break;
    }
}

void saveUmwaelzConfig() {
    EEPROM.put(EEPROM_UMWAELZ_BASE,     (uint8_t)umwaelzConfig.aktiv);
    EEPROM.put(EEPROM_UMWAELZ_BASE + 1, umwaelzConfig.laufzeit_s);
    EEPROM.put(EEPROM_UMWAELZ_BASE + 2, umwaelzConfig.pausenzeit_min);
    EEPROM.write(EEPROM_UMWAELZ_MAGIC_ADDR, EEPROM_UMWAELZ_MAGIC_BYTE);
    EEPROM.commit();
}

void loadUmwaelzConfig() {
    if (EEPROM.read(EEPROM_UMWAELZ_MAGIC_ADDR) == EEPROM_UMWAELZ_MAGIC_BYTE) {
        uint8_t ak;
        EEPROM.get(EEPROM_UMWAELZ_BASE,     ak);
        EEPROM.get(EEPROM_UMWAELZ_BASE + 1, umwaelzConfig.laufzeit_s);
        EEPROM.get(EEPROM_UMWAELZ_BASE + 2, umwaelzConfig.pausenzeit_min);
        umwaelzConfig.aktiv = (bool)ak;
    } else {
        umwaelzConfig.aktiv          = UMWAELZ_AKTIV_DEFAULT;
        umwaelzConfig.laufzeit_s     = UMWAELZ_LAUFZEIT_DEFAULT_S;
        umwaelzConfig.pausenzeit_min = UMWAELZ_PAUSE_DEFAULT_MIN;
        saveUmwaelzConfig();
    }
    umwaelzZustand    = U_PAUSE;
    umwaelzTimerStart = millis();
}

// ========== Ausgangs-Direkttest (Verkabelungspruefung) ==========
static const uint8_t OUTPUT_TEST_PINS[OUTPUT_TEST_COUNT] = {
    VENTIL1_PIN, VENTIL2_PIN, VENTIL3_PIN, VENTIL4_PIN, UMWAELZPUMPE_GPIO, PUMPE_GPIO
};
static const char* OUTPUT_TEST_NAMES[OUTPUT_TEST_COUNT] = {
    "Ventil 1 (Beh. 1)", "Ventil 2 (Beh. 2)", "Ventil 3 (Beh. 3)", "Rücklaufventil",
    "Umwälzpumpe", "Pumpe"
};
static uint8_t testState = 0x00;

void output_test_mode(bool enable) {
    testMode = enable;
    if (!enable) {
        for (uint8_t i = 0; i < OUTPUT_TEST_COUNT; i++) setOut(OUTPUT_TEST_PINS[i], false);
        testState        = 0x00;
        rueckOffenState   = false;
        pumpeState        = false;
        umwaelzState      = false;
    }
}

void output_test_set(uint8_t index, bool on) {
    if (index >= OUTPUT_TEST_COUNT) return;
    if (on) testState |=  (1 << index);
    else    testState &= ~(1 << index);
    setOut(OUTPUT_TEST_PINS[index], on);
}

bool        output_test_active()        { return testMode; }
uint8_t     output_test_get_state()     { return testState; }
const char* output_test_name(uint8_t index) {
    if (index >= OUTPUT_TEST_COUNT) return "?";
    return OUTPUT_TEST_NAMES[index];
}

const char* ventilZustandText(VentilZustand z) {
    switch (z) {
        case V_PAUSE:   return "Pause";
        case V_VORLAUF: return "Vorlauf";
        case V_AKTIV:   return "Aktiv";
        default:        return "?";
    }
}

void setupVentile() {
    for (uint8_t i = 0; i < OUTPUT_TEST_COUNT; i++) {
        pinMode(OUTPUT_TEST_PINS[i], OUTPUT);
        setOut(OUTPUT_TEST_PINS[i], false);
    }
    loadVentilConfig();
    for (int i = 0; i < 3; i++) {
        behaelterZustand[i].zustand    = V_PAUSE;
        behaelterZustand[i].timerStart = millis();
        behaelterZustand[i].timerStart -= (unsigned long)i * 30000UL;
    }

    loadUmwaelzConfig();

    dbgPrintln("Ventile bereit (direkte GPIO, kein PCF8574)");
}

// ========== Batch-Modus ("Gleiche Zeiten fuer alle") ==========
// Statt 3 unabhaengiger Timer laeuft hier EIN gemeinsamer Ablauf: Ruecklauf-Vorlauf
// einmal vor dem ersten aktiven Behaelter, dann alle aktiven Behaelter direkt
// nacheinander (kein erneuter Vorlauf dazwischen), danach eine gemeinsame Pause, dann
// wiederholt sich der ganze Batch. Individuelle "Aktiv"-Haken bleiben nutzbar — inaktive
// Behaelter werden im Batch einfach uebersprungen.
static int8_t       batchActiveIdx  = -1;    // -1 = gemeinsame Pause-Phase
static bool         batchInVorlauf  = false;
static unsigned long batchPauseStart = 0;

static int8_t findNextActive(int8_t fromIdx) {
    for (int8_t i = fromIdx; i < 3; i++)
        if (ventilConfig.behaelter[i].aktiv) return i;
    return -1;
}

static void loopVentileBatch(bool lichtAn) {
    unsigned long now = millis();

    // Sicherheits-Sweep: alles ausser dem gerade laufenden Behaelter explizit aus
    // (deckt sowohl inaktive Behaelter als auch bereits abgearbeitete im Batch ab).
    for (int i = 0; i < 3; i++)
        if (i != batchActiveIdx) setVentil(VENTIL_PIN[i], false);

    if (batchActiveIdx < 0) {
        // Gemeinsame Pause-Phase — alle (aktiven) Behaelter zeigen denselben Timer.
        for (int i = 0; i < 3; i++) {
            behaelterZustand[i].zustand    = V_PAUSE;
            behaelterZustand[i].timerStart = batchPauseStart;
        }
        setVentil(VENTIL4_PIN, false);

        uint8_t pause_min = lichtAn ? ventilConfig.behaelter[0].pausenzeit_min
                                     : ventilConfig.behaelter[0].pausenzeit_min_dunkel;
        uint32_t pause_ms = (uint32_t)pause_min * 60000UL;
        if (now - batchPauseStart >= pause_ms) {
            int8_t first = findNextActive(0);
            if (first < 0) { batchPauseStart = now; return; }  // kein Behaelter aktiv
            batchActiveIdx = first;
            if (ventilConfig.vorlauf_aktiv && ventilConfig.vorlaufzeit_s > 0) {
                batchInVorlauf = true;
                behaelterZustand[batchActiveIdx].zustand    = V_VORLAUF;
                behaelterZustand[batchActiveIdx].timerStart = now;
                dbgPrintf("Behälter %d: Vorlauf (Batch)\n", batchActiveIdx + 1);
            } else {
                batchInVorlauf = false;
                behaelterZustand[batchActiveIdx].zustand    = V_AKTIV;
                behaelterZustand[batchActiveIdx].timerStart = now;
                setVentil(VENTIL_PIN[batchActiveIdx], true);
                dbgPrintf("Behälter %d: Aktiv (Batch)\n", batchActiveIdx + 1);
            }
        }
        return;
    }

    BehaelterZustand& bz = behaelterZustand[batchActiveIdx];
    unsigned long vergangen = now - bz.timerStart;

    if (batchInVorlauf) {
        setVentil(VENTIL4_PIN, true);
        if (vergangen >= (unsigned long)ventilConfig.vorlaufzeit_s * 1000UL) {
            batchInVorlauf = false;
            bz.zustand     = V_AKTIV;
            bz.timerStart  = now;
            setVentil(VENTIL_PIN[batchActiveIdx], true);
            dbgPrintf("Behälter %d: Aktiv (Batch)\n", batchActiveIdx + 1);
        }
        return;
    }

    // V_AKTIV: kein erneuter Ruecklauf-Vorlauf zwischen den Behaeltern desselben Batches.
    setVentil(VENTIL4_PIN, false);
    uint8_t oeffnung_s = lichtAn ? ventilConfig.behaelter[batchActiveIdx].oeffnungszeit_s
                                  : ventilConfig.behaelter[batchActiveIdx].oeffnungszeit_s_dunkel;
    if (vergangen >= (unsigned long)oeffnung_s * 1000UL) {
        setVentil(VENTIL_PIN[batchActiveIdx], false);
        int8_t next = findNextActive(batchActiveIdx + 1);
        if (next < 0) {
            batchActiveIdx  = -1;
            batchPauseStart = now;
            dbgPrintln("Batch fertig -> gemeinsame Pause");
        } else {
            batchActiveIdx = next;
            behaelterZustand[batchActiveIdx].zustand    = V_AKTIV;
            behaelterZustand[batchActiveIdx].timerStart = now;
            setVentil(VENTIL_PIN[batchActiveIdx], true);
            dbgPrintf("Behälter %d: Aktiv (Batch, direkt weiter)\n", batchActiveIdx + 1);
        }
    }
}

void resetVentilRuntimeState() {
    for (int i = 0; i < 3; i++) setVentil(VENTIL_PIN[i], false);
    setVentil(VENTIL4_PIN, false);
    unsigned long now = millis();
    for (int i = 0; i < 3; i++) {
        behaelterZustand[i].zustand    = V_PAUSE;
        behaelterZustand[i].timerStart = now;
    }
    batchActiveIdx  = -1;
    batchInVorlauf  = false;
    batchPauseStart = now;
}

static void loopVentileNormal(bool lichtAn) {
    unsigned long now = millis();
    bool rueckOffen = false;

    for (int i = 0; i < 3; i++) {
        if (!ventilConfig.behaelter[i].aktiv) {
            setVentil(VENTIL_PIN[i], false);
            continue;
        }

        BehaelterZustand& bz  = behaelterZustand[i];
        BehaelterConfig&  cfg = ventilConfig.behaelter[i];
        uint8_t oeffnung_s  = lichtAn ? cfg.oeffnungszeit_s  : cfg.oeffnungszeit_s_dunkel;
        uint8_t pausen_min  = lichtAn ? cfg.pausenzeit_min   : cfg.pausenzeit_min_dunkel;
        unsigned long vergangen = now - bz.timerStart;

        switch (bz.zustand) {
            case V_PAUSE:
                if (vergangen >= (unsigned long)pausen_min * 60000UL) {
                    bool andererAktiv = false;
                    for (int j = 0; j < 3; j++) {
                        if (j != i && ventilConfig.behaelter[j].aktiv &&
                            (behaelterZustand[j].zustand == V_VORLAUF ||
                             behaelterZustand[j].zustand == V_AKTIV)) {
                            andererAktiv = true;
                            break;
                        }
                    }
                    if (andererAktiv) break;

                    if (ventilConfig.vorlauf_aktiv && ventilConfig.vorlaufzeit_s > 0) {
                        bz.zustand    = V_VORLAUF;
                        bz.timerStart = now;
                        dbgPrintf("Behälter %d: Vorlauf\n", i + 1);
                    } else {
                        bz.zustand    = V_AKTIV;
                        bz.timerStart = now;
                        setVentil(VENTIL_PIN[i], true);
                        dbgPrintf("Behälter %d: Aktiv\n", i + 1);
                    }
                }
                break;

            case V_VORLAUF:
                rueckOffen = true;
                if (vergangen >= (unsigned long)ventilConfig.vorlaufzeit_s * 1000UL) {
                    bz.zustand    = V_AKTIV;
                    bz.timerStart = now;
                    setVentil(VENTIL_PIN[i], true);
                    dbgPrintf("Behälter %d: Aktiv\n", i + 1);
                }
                break;

            case V_AKTIV:
                if (vergangen >= (unsigned long)oeffnung_s * 1000UL) {
                    setVentil(VENTIL_PIN[i], false);
                    bz.zustand    = V_PAUSE;
                    bz.timerStart = now;
                    dbgPrintf("Behälter %d: Pause\n", i + 1);
                }
                break;
        }
    }

    setVentil(VENTIL4_PIN, rueckOffen);
}

void loopVentile(bool lichtAn) {
    if (testMode) return;
    if (ventilConfig.gleiche_zeiten) loopVentileBatch(lichtAn);
    else                             loopVentileNormal(lichtAn);

    // Pumpe: automatisch an, sobald irgendein Behaelter gerade aktiv waessert (Ventil
    // offen) — unabhaengig vom Zeitmodus (normal oder Batch), da beide behaelterZustand[]
    // konsistent pflegen. Umwaelzpumpe: laeuft, sobald irgendein Behaelter ueberhaupt
    // aktiviert ist (Konfig-Haken), unabhaengig vom aktuellen Bewaesserungstakt.
    bool anyAktiv   = false;
    bool anyEnabled = false;
    for (int i = 0; i < 3; i++) {
        if (ventilConfig.behaelter[i].aktiv) {
            anyEnabled = true;
            if (behaelterZustand[i].zustand == V_AKTIV) anyAktiv = true;
        }
    }
    setPumpeOut(anyAktiv);
    loopUmwaelzpumpe(anyEnabled);
}

void saveVentilConfig() {
    for (int i = 0; i < 3; i++) {
        int base = EEPROM_BEHAELTER_BASE + i * 5;
        EEPROM.put(base,     ventilConfig.behaelter[i].oeffnungszeit_s);
        EEPROM.put(base + 1, ventilConfig.behaelter[i].pausenzeit_min);
        EEPROM.put(base + 2, (uint8_t)ventilConfig.behaelter[i].aktiv);
        EEPROM.put(base + 3, ventilConfig.behaelter[i].oeffnungszeit_s_dunkel);
        EEPROM.put(base + 4, ventilConfig.behaelter[i].pausenzeit_min_dunkel);
    }
    EEPROM.put(EEPROM_VORLAUF_ADDR,    ventilConfig.vorlaufzeit_s);
    EEPROM.put(EEPROM_VORLAUF_AK_ADDR, (uint8_t)ventilConfig.vorlauf_aktiv);
    EEPROM.put(EEPROM_MAGIC_ADDR,      (uint32_t)EEPROM_MAGIC_NUMBER);
    EEPROM.write(EEPROM_VENTIL_GLEICH_ADDR, (uint8_t)ventilConfig.gleiche_zeiten);
    EEPROM.write(EEPROM_VENTIL_GLEICH_MAGIC_ADDR, EEPROM_VENTIL_GLEICH_MAGIC_BYTE);
    EEPROM.write(EEPROM_VENTIL_DUNKEL_MAGIC_ADDR, EEPROM_VENTIL_DUNKEL_MAGIC_BYTE);
    EEPROM.commit();
    dbgPrintln("Ventilkonfig gespeichert");
}

void loadVentilConfig() {
    uint32_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);

    if (magic == EEPROM_MAGIC_NUMBER) {
        for (int i = 0; i < 3; i++) {
            int base = EEPROM_BEHAELTER_BASE + i * 5;
            uint8_t oe, pa, ak;
            EEPROM.get(base,     oe);
            EEPROM.get(base + 1, pa);
            EEPROM.get(base + 2, ak);
            ventilConfig.behaelter[i].oeffnungszeit_s = oe;
            ventilConfig.behaelter[i].pausenzeit_min  = pa;
            ventilConfig.behaelter[i].aktiv           = (bool)ak;
        }
        uint8_t vz, va;
        EEPROM.get(EEPROM_VORLAUF_ADDR,    vz);
        EEPROM.get(EEPROM_VORLAUF_AK_ADDR, va);
        ventilConfig.vorlaufzeit_s = vz;
        ventilConfig.vorlauf_aktiv = (bool)va;
        dbgPrintln("Ventilkonfig geladen");
    } else {
        for (int i = 0; i < 3; i++) {
            ventilConfig.behaelter[i].oeffnungszeit_s = VENTIL_OEFFNUNG_DEFAULT_S;
            ventilConfig.behaelter[i].pausenzeit_min  = VENTIL_PAUSE_DEFAULT_MIN;
            ventilConfig.behaelter[i].aktiv           = false;
        }
        ventilConfig.vorlaufzeit_s = VENTIL_VORLAUF_DEFAULT_S;
        ventilConfig.vorlauf_aktiv = VENTIL_VORLAUF_AKTIV;
        ventilConfig.gleiche_zeiten = false;
        for (int i = 0; i < 3; i++) {
            ventilConfig.behaelter[i].oeffnungszeit_s_dunkel = VENTIL_OEFFNUNG_DEFAULT_S;
            ventilConfig.behaelter[i].pausenzeit_min_dunkel  = VENTIL_PAUSE_DEFAULT_MIN;
        }
        saveVentilConfig();
        dbgPrintln("Ventilkonfig: Standardwerte");
        return;
    }

    // Eigener Magic-Bereich fuer gleiche_zeiten (siehe Kommentar in config.h)
    if (EEPROM.read(EEPROM_VENTIL_GLEICH_MAGIC_ADDR) == EEPROM_VENTIL_GLEICH_MAGIC_BYTE) {
        uint8_t g = EEPROM.read(EEPROM_VENTIL_GLEICH_ADDR);
        ventilConfig.gleiche_zeiten = (bool)g;
    } else {
        ventilConfig.gleiche_zeiten = false;
        EEPROM.write(EEPROM_VENTIL_GLEICH_ADDR, (uint8_t)0);
        EEPROM.write(EEPROM_VENTIL_GLEICH_MAGIC_ADDR, EEPROM_VENTIL_GLEICH_MAGIC_BYTE);
        EEPROM.commit();
    }

    // Dunkelphase-Zeiten (Bytes 3/4 jedes Behaelter-Blocks) — eigener Magic-Bereich, siehe
    // config.h. Bei Bestandsinstallationen (Magic noch nicht gesetzt) auf die Lichtphase-
    // Werte initialisieren, damit sich am Verhalten erstmal nichts aendert.
    if (EEPROM.read(EEPROM_VENTIL_DUNKEL_MAGIC_ADDR) == EEPROM_VENTIL_DUNKEL_MAGIC_BYTE) {
        for (int i = 0; i < 3; i++) {
            int base = EEPROM_BEHAELTER_BASE + i * 5;
            uint8_t oeD, paD;
            EEPROM.get(base + 3, oeD);
            EEPROM.get(base + 4, paD);
            ventilConfig.behaelter[i].oeffnungszeit_s_dunkel = oeD;
            ventilConfig.behaelter[i].pausenzeit_min_dunkel  = paD;
        }
    } else {
        for (int i = 0; i < 3; i++) {
            ventilConfig.behaelter[i].oeffnungszeit_s_dunkel = ventilConfig.behaelter[i].oeffnungszeit_s;
            ventilConfig.behaelter[i].pausenzeit_min_dunkel  = ventilConfig.behaelter[i].pausenzeit_min;
        }
        saveVentilConfig();
    }
}
