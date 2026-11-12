#include "valves.h"
#include "config.h"
#include "pinout.h"
#include "dbg.h"
#include "notify.h"
#include <EEPROM.h>

VentilConfig     ventilConfig;
BehaelterZustand behaelterZustand[BEHAELTER_ANZAHL];
UmwaelzConfig    umwaelzConfig;

static bool testMode        = false;
static bool testModePending = false;  // angefordert, wartet auf Ende des laufenden Zyklus

// Sicherheitsnetz: ein vergessener Testmodus pausiert die Bewaesserung -- bei Aeroponik
// trocknen die Wurzeln dann schnell aus. Nach dieser Zeit beendet loopVentile() den Testmodus
// selbst und schickt eine Benachrichtigung.
#define TEST_MODE_TIMEOUT_MS (30UL * 60UL * 1000UL)
static unsigned long testModeSince = 0;

// Batch-Modus-Zustand ("Gleiche Zeiten fuer alle") -- vorgezogen deklariert, da setupVentile()
// batchPauseStart schon mitinitialisiert (siehe dort); Details/Nutzung siehe loopVentileBatch().
static int8_t        batchActiveIdx  = -1;    // -1 = gemeinsame Pause-Phase
static bool          batchInDruckaufbau = false;   // Pumpe laeuft, erstes Ventil noch zu
static bool          batchInNachlauf    = false;   // Ventil schon zu, Pumpe fuellt Druckspeicher nach
static unsigned long batchPauseStart = 0;

// Alle Ausgaenge (Ventile, Pumpe, Umwälzpumpe) sind direkte GPIO, kein I2C-Expander
// (PCF8574) mehr — siehe OUTPUT_ACTIVE_LOW in pinout.h fuer die Treiber-Polaritaet.
static void setOut(uint8_t pin, bool on) {
#if OUTPUT_ACTIVE_LOW
    digitalWrite(pin, on ? LOW : HIGH);
#else
    digitalWrite(pin, on ? HIGH : LOW);
#endif
}

static const uint8_t VENTIL_PIN[BEHAELTER_ANZAHL] = { VENTIL1_PIN, VENTIL2_PIN, VENTIL3_PIN, VENTIL4_PIN };

static bool pumpeState      = false;
static bool umwaelzState    = false;

static void setVentil(uint8_t pin, bool on) { setOut(pin, on); }

static void setPumpeOut(bool on)   { pumpeState = on;   setOut(PUMPE_GPIO, on); }
static void setUmwaelzOut(bool on) { umwaelzState = on; setOut(UMWAELZPUMPE_GPIO, on); }

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
    "Ventil 1 (Beh. 1)", "Ventil 2 (Beh. 2)", "Ventil 3 (Beh. 3)", "Ventil 4 (Beh. 4)",
    "Umwälzpumpe", "Pumpe"
};
static uint8_t testState = 0x00;

// true, solange irgendein Ventil/die Pumpe/die Umwälzpumpe gerade automatisch offen/an ist --
// Testmodus soll keinen laufenden Giesszyklus mitten drin unterbrechen, siehe output_test_mode().
// Beim manuellen Einzelzyklus (keepUmwaelz) zaehlt die Umwaelzpumpe nicht mit -- sie hat mit
// dem Spruehen nichts zu tun und laeuft in ihrem eigenen Takt weiter.
static bool testKeepUmwaelz = false;

static bool anyOutputActive() {
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++)
        if (behaelterZustand[i].zustand == V_AKTIV) return true;
    return pumpeState || (umwaelzState && !testKeepUmwaelz);
}

void output_test_mode(bool enable, bool keepUmwaelz) {
    if (enable) {
        if (testMode) return;  // schon aktiv
        testKeepUmwaelz = keepUmwaelz;
        if (anyOutputActive()) {
            // Laufenden Zyklus nicht abwuergen -- erst aktivieren, sobald loopVentile() von
            // selbst alles geschlossen hat (siehe dortige Pruefung von testModePending).
            testModePending = true;
            return;
        }
        testMode        = true;
        testModePending = false;
        testModeSince   = millis();
        testState       = 0x00;  // alles bereits aus, nichts zu uebernehmen
    } else {
        testModePending = false;  // noch nicht aktiv gewordene Anfrage zurueckziehen
        if (!testMode) return;
        testMode = false;
        for (uint8_t i = 0; i < OUTPUT_TEST_COUNT; i++) {
            if (testKeepUmwaelz && OUTPUT_TEST_PINS[i] == UMWAELZPUMPE_GPIO) continue;
            setOut(OUTPUT_TEST_PINS[i], false);
        }
        testState        = 0x00;
        pumpeState        = false;
        if (!testKeepUmwaelz) umwaelzState = false;
        testKeepUmwaelz  = false;
    }
}

bool output_test_pending() { return testModePending; }

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
        case V_AKTIV:   return "Aktiv";
        case V_DRUCKAUFBAU: return "Druckaufbau";
        case V_NACHLAUF:    return "Nachlauf";
        default:        return "?";
    }
}

void setupVentile() {
    for (uint8_t i = 0; i < OUTPUT_TEST_COUNT; i++) {
        pinMode(OUTPUT_TEST_PINS[i], OUTPUT);
        setOut(OUTPUT_TEST_PINS[i], false);
    }
    loadVentilConfig();
    // Nach einem Neustart soll sofort gewaessert werden, statt erst die volle konfigurierte
    // Pausenzeit abzuwarten (die bis zu 60 Minuten betragen kann) -- timerStart/batchPauseStart
    // daher weit genug in die Vergangenheit setzen, damit die Pausenzeit-Pruefung (siehe
    // loopVentileNormal()/loopVentileBatch()) beim allerersten Durchlauf sofort "abgelaufen"
    // sieht. Unsigned-Unterlauf hier ist beabsichtigt und sicher (uebliches millis()-Rollover-
    // Idiom): "now - timerStart" liefert trotzdem die korrekte (grosse) Differenz.
    unsigned long initialTimerStart = millis() - 61UL * 60000UL;
    for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
        behaelterZustand[i].zustand    = V_PAUSE;
        behaelterZustand[i].timerStart = initialTimerStart;
    }
    batchPauseStart = initialTimerStart;

    loadUmwaelzConfig();

    dbgPrintln("Ventile bereit (direkte GPIO, kein PCF8574)");
}

// ========== Batch-Modus ("Gleiche Zeiten fuer alle") ==========
// Statt unabhaengiger Timer laeuft hier EIN gemeinsamer Ablauf: alle aktiven Behaelter
// direkt nacheinander, danach eine gemeinsame Pause, dann
// wiederholt sich der ganze Batch. Individuelle "Aktiv"-Haken bleiben nutzbar — inaktive
// Behaelter werden im Batch einfach uebersprungen. (Zustandsvariablen siehe Dateianfang.)

static int8_t findNextActive(int8_t fromIdx) {
    for (int8_t i = fromIdx; i < BEHAELTER_ANZAHL; i++)
        if (ventilConfig.behaelter[i].aktiv) return i;
    return -1;
}

// Erst Druckaufbau (Pumpe an, Ventil zu), dann Ventil auf.
static void batchStart(unsigned long now) {
    BehaelterZustand& bz = behaelterZustand[batchActiveIdx];
    bz.timerStart = now;
    if (ventilConfig.druckaufbau_s > 0) {
        batchInDruckaufbau = true;
        bz.zustand = V_DRUCKAUFBAU;
        dbgPrintf("Behälter %d: Druckaufbau %us (Batch)\n", batchActiveIdx + 1, ventilConfig.druckaufbau_s);
    } else {
        bz.zustand = V_AKTIV;
        setVentil(VENTIL_PIN[batchActiveIdx], true);
        dbgPrintf("Behälter %d: Aktiv (Batch)\n", batchActiveIdx + 1);
    }
}

static void loopVentileBatch(bool lichtAn) {
    unsigned long now = millis();

    // Sicherheits-Sweep: alles ausser dem gerade laufenden Behaelter explizit aus
    // (deckt sowohl inaktive Behaelter als auch bereits abgearbeitete im Batch ab).
    for (int i = 0; i < BEHAELTER_ANZAHL; i++)
        if (i != batchActiveIdx) setVentil(VENTIL_PIN[i], false);

    if (batchActiveIdx < 0) {
        // Gemeinsame Pause-Phase — alle (aktiven) Behaelter zeigen denselben Timer.
        for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
            behaelterZustand[i].zustand    = V_PAUSE;
            behaelterZustand[i].timerStart = batchPauseStart;
        }

        uint8_t pause_min = lichtAn ? ventilConfig.behaelter[0].pausenzeit_min
                                     : ventilConfig.behaelter[0].pausenzeit_min_dunkel;
        uint32_t pause_ms = (uint32_t)pause_min * 60000UL;
        if (now - batchPauseStart >= pause_ms) {
            int8_t first = findNextActive(0);
            if (first < 0) { batchPauseStart = now; return; }  // kein Behaelter aktiv
            batchActiveIdx = first;
            batchStart(now);
        }
        return;
    }

    BehaelterZustand& bz = behaelterZustand[batchActiveIdx];
    unsigned long vergangen = now - bz.timerStart;

    // Druckaufbau vor JEDEM Behaelter des Batches (Nutzerwunsch): das vorige Ventil ist zu, die
    // Pumpe laeuft weiter, der Druck steht wieder voll an, erst dann oeffnet das naechste Ventil --
    // so sprueht jeder Behaelter unter denselben Bedingungen (vergleichbare Durchfluss-Impulse).
    if (batchInDruckaufbau) {
        if (vergangen >= (unsigned long)ventilConfig.druckaufbau_s * 1000UL) {
            batchInDruckaufbau = false;
            bz.zustand    = V_AKTIV;
            bz.timerStart = now;
            setVentil(VENTIL_PIN[batchActiveIdx], true);
            dbgPrintf("Behälter %d: Aktiv (Batch)\n", batchActiveIdx + 1);
        }
        return;
    }

    // Nachlauf: Ventil zu, Pumpe fuellt den Druckspeicher wieder auf (Impulse zaehlen noch zu
    // diesem Behaelter, siehe flowmonitor.cpp). Danach steht der Druck schon -- der naechste
    // Behaelter oeffnet direkt, ohne eigenen Druckaufbau.
    if (batchInNachlauf) {
        if (vergangen >= (unsigned long)ventilConfig.druckaufbau_s * 1000UL) {
            batchInNachlauf = false;
            int8_t next = findNextActive(batchActiveIdx + 1);
            bz.zustand    = V_PAUSE;
            bz.timerStart = now;
            if (next < 0) {
                batchActiveIdx  = -1;
                batchPauseStart = now;
                dbgPrintln("Batch fertig -> gemeinsame Pause");
            } else {
                batchActiveIdx = next;
                BehaelterZustand& nz = behaelterZustand[next];
                nz.zustand    = V_AKTIV;
                nz.timerStart = now;
                setVentil(VENTIL_PIN[next], true);
                dbgPrintf("Behälter %d: Aktiv (Batch)\n", next + 1);
            }
        }
        return;
    }

    // V_AKTIV
    uint8_t oeffnung_s = lichtAn ? ventilConfig.behaelter[batchActiveIdx].oeffnungszeit_s
                                  : ventilConfig.behaelter[batchActiveIdx].oeffnungszeit_s_dunkel;
    if (vergangen >= (unsigned long)oeffnung_s * 1000UL) {
        setVentil(VENTIL_PIN[batchActiveIdx], false);
        if (ventilConfig.druckaufbau_s > 0) {
            batchInNachlauf = true;
            bz.zustand    = V_NACHLAUF;
            bz.timerStart = now;
            dbgPrintf("Behälter %d: Nachlauf %us (Batch)\n", batchActiveIdx + 1, ventilConfig.druckaufbau_s);
            return;
        }
        int8_t next = findNextActive(batchActiveIdx + 1);
        if (next < 0) {
            batchActiveIdx  = -1;
            batchPauseStart = now;
            dbgPrintln("Batch fertig -> gemeinsame Pause");
        } else {
            bz.zustand     = V_PAUSE;   // vorigen Behaelter abschliessen (sonst bliebe er "Aktiv")
            bz.timerStart  = now;
            batchActiveIdx = next;
            batchStart(now);   // ohne Druckaufbau: Ventil direkt auf
        }
    }
}

void resetVentilRuntimeState() {
    for (int i = 0; i < BEHAELTER_ANZAHL; i++) setVentil(VENTIL_PIN[i], false);
    unsigned long now = millis();
    for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
        behaelterZustand[i].zustand    = V_PAUSE;
        behaelterZustand[i].timerStart = now;
    }
    batchActiveIdx  = -1;
    batchInDruckaufbau = false;
    batchInNachlauf    = false;
    batchPauseStart = now;
}

// Normalmodus: erst Druckaufbau (Pumpe an, Ventil zu), dann Ventil auf.
static void normalStart(int i, unsigned long now) {
    BehaelterZustand& bz = behaelterZustand[i];
    bz.timerStart = now;
    if (ventilConfig.druckaufbau_s > 0) {
        bz.zustand = V_DRUCKAUFBAU;
        dbgPrintf("Behälter %d: Druckaufbau %us\n", i + 1, ventilConfig.druckaufbau_s);
    } else {
        bz.zustand = V_AKTIV;
        setVentil(VENTIL_PIN[i], true);
        dbgPrintf("Behälter %d: Aktiv\n", i + 1);
    }
}

static void loopVentileNormal(bool lichtAn) {
    unsigned long now = millis();

    for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
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
                    for (int j = 0; j < BEHAELTER_ANZAHL; j++) {
                        if (j != i && ventilConfig.behaelter[j].aktiv &&
                            (behaelterZustand[j].zustand == V_DRUCKAUFBAU ||
                             behaelterZustand[j].zustand == V_AKTIV ||
                             behaelterZustand[j].zustand == V_NACHLAUF)) {
                            andererAktiv = true;
                            break;
                        }
                    }
                    if (andererAktiv) break;
                    normalStart(i, now);
                }
                break;

            case V_DRUCKAUFBAU:
                if (vergangen >= (unsigned long)ventilConfig.druckaufbau_s * 1000UL) {
                    bz.zustand    = V_AKTIV;
                    bz.timerStart = now;
                    setVentil(VENTIL_PIN[i], true);
                    dbgPrintf("Behälter %d: Aktiv\n", i + 1);
                }
                break;

            case V_AKTIV:
                if (vergangen >= (unsigned long)oeffnung_s * 1000UL) {
                    setVentil(VENTIL_PIN[i], false);
                    bz.timerStart = now;
                    if (ventilConfig.druckaufbau_s > 0) {
                        bz.zustand = V_NACHLAUF;   // Pumpe fuellt Druckspeicher nach
                        dbgPrintf("Behälter %d: Nachlauf %us\n", i + 1, ventilConfig.druckaufbau_s);
                    } else {
                        bz.zustand = V_PAUSE;
                        dbgPrintf("Behälter %d: Pause\n", i + 1);
                    }
                }
                break;

            case V_NACHLAUF:
                if (vergangen >= (unsigned long)ventilConfig.druckaufbau_s * 1000UL) {
                    bz.zustand    = V_PAUSE;
                    bz.timerStart = now;
                    dbgPrintf("Behälter %d: Pause\n", i + 1);
                }
                break;
        }
    }
}

void loopVentile(bool lichtAn) {
    if (testModePending && !anyOutputActive()) {
        testMode        = true;
        testModePending = false;
        testModeSince   = millis();
        testState       = 0x00;
        dbgPrintln("Ausgangstest: Zyklus beendet, Testmodus jetzt aktiv");
    }
    if (testMode && millis() - testModeSince >= TEST_MODE_TIMEOUT_MS) {
        output_test_mode(false);   // alle Testausgaenge aus, Automatik laeuft ab hier weiter
        dbgPrintln("Ausgangstest: nach 30 min automatisch beendet");
        sendNotification("⚠️ Testmodus war 30 Minuten aktiv und wurde automatisch beendet – die Bewässerung läuft wieder.");
    }
    if (testMode) {
        // Manueller Einzelzyklus: Umwaelzpumpe laeuft im eigenen Takt weiter.
        if (testKeepUmwaelz) {
            bool anyEnabled = false;
            for (int i = 0; i < BEHAELTER_ANZAHL; i++)
                if (ventilConfig.behaelter[i].aktiv) anyEnabled = true;
            loopUmwaelzpumpe(anyEnabled);
        }
        return;
    }
    if (ventilConfig.gleiche_zeiten) loopVentileBatch(lichtAn);
    else                             loopVentileNormal(lichtAn);

    // Pumpe: automatisch an, sobald irgendein Behaelter gerade aktiv waessert (Ventil
    // offen) — unabhaengig vom Zeitmodus (normal oder Batch), da beide behaelterZustand[]
    // konsistent pflegen. Umwaelzpumpe: laeuft, sobald irgendein Behaelter ueberhaupt
    // aktiviert ist (Konfig-Haken), unabhaengig vom aktuellen Bewaesserungstakt.
    bool anyAktiv   = false;
    bool anyEnabled = false;
    for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
        if (ventilConfig.behaelter[i].aktiv) {
            anyEnabled = true;
            if (behaelterZustand[i].zustand == V_AKTIV ||
                behaelterZustand[i].zustand == V_DRUCKAUFBAU ||
                behaelterZustand[i].zustand == V_NACHLAUF) anyAktiv = true;
        }
    }
    setPumpeOut(anyAktiv);
    loopUmwaelzpumpe(anyEnabled);
}

// Behaelter 1-3 liegen ab EEPROM_BEHAELTER_BASE, Behaelter 4 kam spaeter dazu und hat einen
// eigenen Block mit Magic-Byte (siehe config.h).
static int behaelterEepromBase(int i) {
    return (i < 3) ? EEPROM_BEHAELTER_BASE + i * 5 : EEPROM_BEHAELTER4_BASE;
}

void saveVentilConfig() {
    for (int i = 0; i < BEHAELTER_ANZAHL; i++) {
        int base = behaelterEepromBase(i);
        EEPROM.put(base,     ventilConfig.behaelter[i].oeffnungszeit_s);
        EEPROM.put(base + 1, ventilConfig.behaelter[i].pausenzeit_min);
        EEPROM.put(base + 2, (uint8_t)ventilConfig.behaelter[i].aktiv);
        EEPROM.put(base + 3, ventilConfig.behaelter[i].oeffnungszeit_s_dunkel);
        EEPROM.put(base + 4, ventilConfig.behaelter[i].pausenzeit_min_dunkel);
    }
    EEPROM.write(EEPROM_BEHAELTER4_MAGIC_ADDR, EEPROM_BEHAELTER4_MAGIC_BYTE);
    EEPROM.put(EEPROM_MAGIC_ADDR,      (uint32_t)EEPROM_MAGIC_NUMBER);
    EEPROM.write(EEPROM_VENTIL_GLEICH_ADDR, (uint8_t)ventilConfig.gleiche_zeiten);
    EEPROM.write(EEPROM_VENTIL_GLEICH_MAGIC_ADDR, EEPROM_VENTIL_GLEICH_MAGIC_BYTE);
    EEPROM.write(EEPROM_VENTIL_DUNKEL_MAGIC_ADDR, EEPROM_VENTIL_DUNKEL_MAGIC_BYTE);
    EEPROM.write(EEPROM_DRUCKAUFBAU_BASE, ventilConfig.druckaufbau_s);
    EEPROM.write(EEPROM_DRUCKAUFBAU_MAGIC_ADDR, EEPROM_DRUCKAUFBAU_MAGIC_BYTE);
    EEPROM.commit();
    dbgPrintln("Ventilkonfig gespeichert");
}

static void behaelterDefaults(int i) {
    ventilConfig.behaelter[i].oeffnungszeit_s        = VENTIL_OEFFNUNG_DEFAULT_S;
    ventilConfig.behaelter[i].pausenzeit_min         = VENTIL_PAUSE_DEFAULT_MIN;
    ventilConfig.behaelter[i].aktiv                  = false;
    ventilConfig.behaelter[i].oeffnungszeit_s_dunkel = VENTIL_OEFFNUNG_DEFAULT_S;
    ventilConfig.behaelter[i].pausenzeit_min_dunkel  = VENTIL_PAUSE_DEFAULT_MIN;
}

void loadVentilConfig() {
    // Vorbelegen: saveVentilConfig() im Standardwerte-Zweig schreibt den Druckaufbau mit.
    ventilConfig.druckaufbau_s = VENTIL_DRUCKAUFBAU_DEFAULT_S;
    uint32_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);

    if (magic != EEPROM_MAGIC_NUMBER) {
        for (int i = 0; i < BEHAELTER_ANZAHL; i++) behaelterDefaults(i);
        ventilConfig.gleiche_zeiten = false;
        saveVentilConfig();
        dbgPrintln("Ventilkonfig: Standardwerte");
        return;
    }

    for (int i = 0; i < 3; i++) {
        int base = behaelterEepromBase(i);
        ventilConfig.behaelter[i].oeffnungszeit_s = EEPROM.read(base);
        ventilConfig.behaelter[i].pausenzeit_min  = EEPROM.read(base + 1);
        ventilConfig.behaelter[i].aktiv           = (bool)EEPROM.read(base + 2);
    }

    // Eigener Magic-Bereich fuer gleiche_zeiten (siehe Kommentar in config.h)
    bool gleichOk = EEPROM.read(EEPROM_VENTIL_GLEICH_MAGIC_ADDR) == EEPROM_VENTIL_GLEICH_MAGIC_BYTE;
    ventilConfig.gleiche_zeiten = gleichOk ? (bool)EEPROM.read(EEPROM_VENTIL_GLEICH_ADDR) : false;

    // Dunkelphase-Zeiten (Bytes 3/4 jedes Behaelter-Blocks) — eigener Magic-Bereich, siehe
    // config.h. Bei Bestandsinstallationen (Magic noch nicht gesetzt) auf die Lichtphase-
    // Werte initialisieren, damit sich am Verhalten erstmal nichts aendert.
    bool dunkelOk = EEPROM.read(EEPROM_VENTIL_DUNKEL_MAGIC_ADDR) == EEPROM_VENTIL_DUNKEL_MAGIC_BYTE;
    for (int i = 0; i < 3; i++) {
        int base = behaelterEepromBase(i);
        ventilConfig.behaelter[i].oeffnungszeit_s_dunkel = dunkelOk ? EEPROM.read(base + 3)
                                                                    : ventilConfig.behaelter[i].oeffnungszeit_s;
        ventilConfig.behaelter[i].pausenzeit_min_dunkel  = dunkelOk ? EEPROM.read(base + 4)
                                                                    : ventilConfig.behaelter[i].pausenzeit_min;
    }

    // Behaelter 4 -- eigener Block. Bei Bestandsinstallationen: Zeiten von Behaelter 3
    // uebernehmen (bisher hingen 3+4 am selben Ventil), aber erst einmal inaktiv.
    bool b4Ok = EEPROM.read(EEPROM_BEHAELTER4_MAGIC_ADDR) == EEPROM_BEHAELTER4_MAGIC_BYTE;
    if (b4Ok) {
        int base = behaelterEepromBase(3);
        ventilConfig.behaelter[3].oeffnungszeit_s        = EEPROM.read(base);
        ventilConfig.behaelter[3].pausenzeit_min         = EEPROM.read(base + 1);
        ventilConfig.behaelter[3].aktiv                  = (bool)EEPROM.read(base + 2);
        ventilConfig.behaelter[3].oeffnungszeit_s_dunkel = EEPROM.read(base + 3);
        ventilConfig.behaelter[3].pausenzeit_min_dunkel  = EEPROM.read(base + 4);
    } else {
        ventilConfig.behaelter[3]       = ventilConfig.behaelter[2];
        ventilConfig.behaelter[3].aktiv = false;
    }

    // Druckaufbau -- eigener Magic-Bereich (siehe config.h), Bestandsinstallationen bekommen
    // den Default (Pumpe 5 s vor dem Ventil).
    bool druckOk = EEPROM.read(EEPROM_DRUCKAUFBAU_MAGIC_ADDR) == EEPROM_DRUCKAUFBAU_MAGIC_BYTE;
    if (druckOk) ventilConfig.druckaufbau_s = EEPROM.read(EEPROM_DRUCKAUFBAU_BASE);

    dbgPrintln("Ventilkonfig geladen");
    if (!gleichOk || !dunkelOk || !b4Ok || !druckOk) saveVentilConfig();
}
