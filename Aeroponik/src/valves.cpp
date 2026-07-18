#include "valves.h"
#include "config.h"
#include "pinout.h"
#include <EEPROM.h>
#include <Wire.h>

VentilConfig     ventilConfig;
BehaelterZustand behaelterZustand[3];

// PCF8574AP Zustand (Bit=1 → Ventil AN, Bit=0 → Ventil AUS)
static uint8_t pcf_state = 0x00;
static bool    testMode  = false;

static void pcf_write() {
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(pcf_state);
    Wire.endTransmission();
}

static void setVentil(uint8_t mv_bit, bool on) {
    if (on) pcf_state |=  (1 << mv_bit);
    else    pcf_state &= ~(1 << mv_bit);
    pcf_write();
}

static const uint8_t VENTIL_MV[3] = {
    MV_BEHAELTER_1, MV_BEHAELTER_2, MV_BEHAELTER_3
};

bool ventilRueckOffen() {
    return (pcf_state & (1 << MV_RUECKLAUF)) != 0;
}

void pcf_test_mode(bool enable) {
    testMode = enable;
    if (!enable) {
        pcf_state = 0x00;
        pcf_write();
    }
}

void pcf_test_set(uint8_t pin, bool on) {
    if (pin > 7) return;
    if (on) pcf_state |=  (1 << pin);
    else    pcf_state &= ~(1 << pin);
    pcf_write();
}

bool    pcf_test_active() { return testMode; }
uint8_t pcf_get_state()   { return pcf_state; }

const char* ventilZustandText(VentilZustand z) {
    switch (z) {
        case V_PAUSE:   return "Pause";
        case V_VORLAUF: return "Vorlauf";
        case V_AKTIV:   return "Aktiv";
        default:        return "?";
    }
}

void setupVentile() {
    pcf_state = 0x00;
    pcf_write();  // alle Ventile aus
    loadVentilConfig();
    for (int i = 0; i < 3; i++) {
        behaelterZustand[i].zustand    = V_PAUSE;
        behaelterZustand[i].timerStart = millis();
        behaelterZustand[i].timerStart -= (unsigned long)i * 30000UL;
    }
    Serial.println("Ventile bereit (PCF8574AP 0x38)");
}

void loopVentile() {
    if (testMode) return;
    unsigned long now = millis();
    bool rueckOffen = false;

    for (int i = 0; i < 3; i++) {
        if (!ventilConfig.behaelter[i].aktiv) {
            setVentil(VENTIL_MV[i], false);
            continue;
        }

        BehaelterZustand& bz  = behaelterZustand[i];
        BehaelterConfig&  cfg = ventilConfig.behaelter[i];
        unsigned long vergangen = now - bz.timerStart;

        switch (bz.zustand) {
            case V_PAUSE:
                if (vergangen >= (unsigned long)cfg.pausenzeit_min * 60000UL) {
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
                        Serial.printf("Behälter %d: Vorlauf\n", i + 1);
                    } else {
                        bz.zustand    = V_AKTIV;
                        bz.timerStart = now;
                        setVentil(VENTIL_MV[i], true);
                        Serial.printf("Behälter %d: Aktiv\n", i + 1);
                    }
                }
                break;

            case V_VORLAUF:
                rueckOffen = true;
                if (vergangen >= (unsigned long)ventilConfig.vorlaufzeit_s * 1000UL) {
                    bz.zustand    = V_AKTIV;
                    bz.timerStart = now;
                    setVentil(VENTIL_MV[i], true);
                    Serial.printf("Behälter %d: Aktiv\n", i + 1);
                }
                break;

            case V_AKTIV:
                if (vergangen >= (unsigned long)cfg.oeffnungszeit_s * 1000UL) {
                    setVentil(VENTIL_MV[i], false);
                    bz.zustand    = V_PAUSE;
                    bz.timerStart = now;
                    Serial.printf("Behälter %d: Pause\n", i + 1);
                }
                break;
        }
    }

    setVentil(MV_RUECKLAUF, rueckOffen);
}

void saveVentilConfig() {
    for (int i = 0; i < 3; i++) {
        int base = EEPROM_BEHAELTER_BASE + i * 5;
        EEPROM.put(base,     ventilConfig.behaelter[i].oeffnungszeit_s);
        EEPROM.put(base + 1, ventilConfig.behaelter[i].pausenzeit_min);
        EEPROM.put(base + 2, (uint8_t)ventilConfig.behaelter[i].aktiv);
    }
    EEPROM.put(EEPROM_VORLAUF_ADDR,    ventilConfig.vorlaufzeit_s);
    EEPROM.put(EEPROM_VORLAUF_AK_ADDR, (uint8_t)ventilConfig.vorlauf_aktiv);
    EEPROM.put(EEPROM_MAGIC_ADDR,      (uint32_t)EEPROM_MAGIC_NUMBER);
    EEPROM.commit();
    Serial.println("Ventilkonfig gespeichert");
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
        Serial.println("Ventilkonfig geladen");
    } else {
        for (int i = 0; i < 3; i++) {
            ventilConfig.behaelter[i].oeffnungszeit_s = VENTIL_OEFFNUNG_DEFAULT_S;
            ventilConfig.behaelter[i].pausenzeit_min  = VENTIL_PAUSE_DEFAULT_MIN;
            ventilConfig.behaelter[i].aktiv           = false;
        }
        ventilConfig.vorlaufzeit_s = VENTIL_VORLAUF_DEFAULT_S;
        ventilConfig.vorlauf_aktiv = VENTIL_VORLAUF_AKTIV;
        saveVentilConfig();
        Serial.println("Ventilkonfig: Standardwerte");
    }
}
