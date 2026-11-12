#include "vpdctrl.h"
#include "config.h"
#include "dbg.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>

VpdConfig vpdConfig;
VpdState  vpdState = { false, 0, 0, 0, 0, false, false, -1, VPD_R_SENSOR, false, 0, 0 };

// Richtwerte (kPa) je Wachstumsphase: { Tag min, Tag max, Nacht min, Nacht max }
static const float PHASE_BANDS[VPD_PHASE_COUNT - 1][4] = {
    { 0.40f, 0.80f, 0.30f, 0.60f },   // Steckling / Keimling
    { 0.80f, 1.20f, 0.60f, 1.00f },   // Wachstum
    { 1.00f, 1.40f, 0.80f, 1.20f },   // Bluete frueh
    { 1.20f, 1.60f, 1.00f, 1.40f },   // Bluete spaet
};

#define VPD_RAMP_KPA_PER_S   (0.45f / 1800.0f)   // Zielbereich wandert max. 0,45 kPa in 30 min
#define HUMID_MIN_ON_MS      60000UL             // Befeuchter mind. 60 s an ...
#define HUMID_MIN_OFF_MS     60000UL             // ... und mind. 60 s aus (kein Takten)
// Der Befeuchter ist stark und der Feuchtesensor traege: am Stueck laufend schiesst die Feuchte
// weit ueber das Ziel hinaus (Log 12.11.: 46 -> 75 % in 7 min). Bei Bedarf deshalb im Takt:
// 60 s an, 120 s Pause, damit der Sensor die Wirkung sieht, bevor nachgelegt wird.
#define HUMID_PULSE_ON_MS    60000UL
#define HUMID_PULSE_OFF_MS   120000UL
#define FAN_STEP_MAX         5.0f                // % pro Regelschritt (5 s)
#define FAN_UP_STEP_MAX      1.0f                // % pro Regelschritt (5 s) beim Hochfahren

// ---------- EEPROM (feldweise, kein Struct-Padding im Spiel) ----------
void saveVpdConfig() {
    const int b = EEPROM_VPD_BASE;
    EEPROM.write(b + 0,  vpdConfig.enabled ? 1 : 0);
    EEPROM.write(b + 1,  vpdConfig.phase);
    EEPROM.write(b + 2,  vpdConfig.day_min);
    EEPROM.write(b + 3,  vpdConfig.day_max);
    EEPROM.write(b + 4,  vpdConfig.night_min);
    EEPROM.write(b + 5,  vpdConfig.night_max);
    EEPROM.write(b + 6,  (uint8_t)vpdConfig.leaf_offset);
    EEPROM.write(b + 7,  vpdConfig.use_humid ? 1 : 0);
    EEPROM.write(b + 8,  vpdConfig.use_fan ? 1 : 0);
    EEPROM.write(b + 9,  vpdConfig.temp_max);
    EEPROM.write(b + 10, vpdConfig.hum_max);
    EEPROM.write(b + 11, vpdConfig.alarm_min);
    EEPROM.write(b + 12, vpdConfig.temp_min);
    EEPROM.write(EEPROM_VPD_MAGIC_ADDR, EEPROM_VPD_MAGIC_BYTE);
    EEPROM.commit();
}

void loadVpdConfig() {
    const int b = EEPROM_VPD_BASE;
    if (EEPROM.read(EEPROM_VPD_MAGIC_ADDR) == EEPROM_VPD_MAGIC_BYTE) {
        vpdConfig.enabled     = EEPROM.read(b + 0) == 1;
        vpdConfig.phase       = EEPROM.read(b + 1);
        vpdConfig.day_min     = EEPROM.read(b + 2);
        vpdConfig.day_max     = EEPROM.read(b + 3);
        vpdConfig.night_min   = EEPROM.read(b + 4);
        vpdConfig.night_max   = EEPROM.read(b + 5);
        vpdConfig.leaf_offset = (int8_t)EEPROM.read(b + 6);
        vpdConfig.use_humid   = EEPROM.read(b + 7) == 1;
        vpdConfig.use_fan     = EEPROM.read(b + 8) == 1;
        vpdConfig.temp_max    = EEPROM.read(b + 9);
        vpdConfig.hum_max     = EEPROM.read(b + 10);
        vpdConfig.alarm_min   = EEPROM.read(b + 11);
        vpdConfig.temp_min    = EEPROM.read(b + 12);   // kam in 1.21.2 dazu, vorher 0xFF
        if (vpdConfig.phase >= VPD_PHASE_COUNT) vpdConfig.phase = VPD_PHASE_WACHSTUM;
        if (vpdConfig.temp_min < 5 || vpdConfig.temp_min > 30) { vpdConfig.temp_min = 18; saveVpdConfig(); }
    } else {
        vpdConfig.enabled     = false;
        vpdConfig.phase       = VPD_PHASE_WACHSTUM;
        vpdConfig.day_min     = 80;  vpdConfig.day_max   = 120;
        vpdConfig.night_min   = 60;  vpdConfig.night_max = 100;
        vpdConfig.leaf_offset = -15;
        vpdConfig.use_humid   = true;
        vpdConfig.use_fan     = true;
        vpdConfig.temp_max    = 28;
        vpdConfig.hum_max     = 75;
        vpdConfig.alarm_min   = 30;
        vpdConfig.temp_min    = 18;
        saveVpdConfig();
    }
}

void vpdPhaseBand(uint8_t phase, bool day, float& lo, float& hi) {
    if (phase < VPD_PHASE_EIGENE) {
        lo = PHASE_BANDS[phase][day ? 0 : 2];
        hi = PHASE_BANDS[phase][day ? 1 : 3];
    } else {
        lo = (day ? vpdConfig.day_min : vpdConfig.night_min) / 100.0f;
        hi = (day ? vpdConfig.day_max : vpdConfig.night_max) / 100.0f;
    }
    if (hi < lo + 0.05f) hi = lo + 0.05f;   // Schutz gegen leeres/vertauschtes Band
}

// ---------- Physik ----------
static float svp(float t) { return 0.6108f * expf(17.27f * t / (t + 237.3f)); }   // kPa (Tetens)
static float absHum(float t, float rh) {   // g/m³
    return 216.7f * (rh / 100.0f * 6.112f * expf(17.62f * t / (243.12f + t))) / (273.15f + t);
}

// ---------- Regler-Zustand ----------
static bool     rampInit = false, fanInit = false;
static float    curLo = 0, curHi = 0, curOff = 0;
static float    fanCmd = 0;
static uint32_t lastUpdMs = 0, humidSwitchMs = 0, humidPulseMs = 0;

void vpdReset() { rampInit = false; fanInit = false; }

static float approach(float cur, float target, float maxStep) {
    if (cur < target) return fminf(cur + maxStep, target);
    if (cur > target) return fmaxf(cur - maxStep, target);
    return cur;
}
// linear von 0 (bei a) bis 1 (bei b)
static float ramp01(float v, float a, float b) { return b > a ? fminf(fmaxf((v - a) / (b - a), 0.0f), 1.0f) : (v >= b ? 1.0f : 0.0f); }

void vpdUpdate(const VpdInputs& in) {
    uint32_t now = millis();
    float dtS = lastUpdMs ? (now - lastUpdMs) / 1000.0f : 5.0f;
    if (dtS > 60.0f) dtS = 60.0f;
    lastUpdMs = now;

    VpdState& s = vpdState;
    if (!vpdConfig.enabled) {
        s.valid = false; s.humidWant = false; s.humidDemand = false; s.fanPct = -1; s.reasons = 0; s.outOfBand = false;
        vpdReset();
        return;
    }
    if (!in.klimaOk) {
        // Ohne Messwert: Befeuchter sicherheitshalber aus, Luefter behaelt seinen letzten Wert
        s.valid = false; s.humidWant = false; s.humidDemand = false; s.fanPct = -1; s.reasons = VPD_R_SENSOR; s.outOfBand = false;
        return;
    }

    // Zielbereich, bei Licht an/aus gleitend nachgefuehrt
    float lo, hi;
    vpdPhaseBand(vpdConfig.phase, in.lightOn, lo, hi);
    const float offTarget = in.lightOn ? vpdConfig.leaf_offset / 10.0f : 0.0f;
    if (!rampInit) { curLo = lo; curHi = hi; curOff = offTarget; rampInit = true; }
    float step = VPD_RAMP_KPA_PER_S * dtS;
    curLo = approach(curLo, lo, step);
    curHi = approach(curHi, hi, step);
    uint16_t r = 0;
    if (fabsf(curLo - lo) > 0.001f || fabsf(curHi - hi) > 0.001f) r |= VPD_R_RAMP;

    // Blatt-VPD: Offset nur bei Licht an (nachts kein Strahlungsunterschied Blatt/Luft)
    float svpAir = svp(in.tempC);
    float avp    = svpAir * in.humPct / 100.0f;
    // Offset gleitet wie der Zielbereich (max. 3 °C in 30 min): beim Sonnenaufgang der Lampe
    // sprang das Blatt-VPD sonst schlagartig um ~0,2 kPa nach unten, waehrend das Band erst
    // langsam hochwanderte -> "zu feucht" und Abluft auf Hoechstdrehzahl (Log 12.11., 03:50)
    curOff = approach(curOff, offTarget, (3.0f / 1800.0f) * dtS);
    float leafT  = in.tempC + curOff;
    s.vpdAir  = svpAir - avp;
    s.vpdLeaf = fmaxf(0.0f, svp(leafT) - avp);
    s.sollMin = curLo; s.sollMax = curHi;
    s.tentAbsHum = absHum(in.tempC, in.humPct);
    s.roomAbsHum = in.roomOk ? absHum(in.roomTempC, in.roomHumPct) : 0;

    const float v = s.vpdLeaf, w = curHi - curLo, mid = (curLo + curHi) / 2.0f;
    const bool tooHumid = v < curLo, tooDry = v > curHi;
    const bool humLimit = in.humPct >= vpdConfig.hum_max;

    float fMin = in.fanMin, fMax = in.fanMax < in.fanMin ? in.fanMin : in.fanMax;
    if (!fanInit) { fanCmd = in.fanNow >= 0 ? constrain((float)in.fanNow, fMin, fMax) : fMin; fanInit = true; }
    // Erst die Abluft drosseln, dann befeuchten: solange der Luefter noch ueber seiner
    // Mindestdrehzahl laeuft, springt der Befeuchter nur an, wenn das VPD schon ueber dem Band liegt
    const bool fanAtMin = !vpdConfig.use_fan || fanCmd <= fMin + 0.5f;

    // ---- Befeuchter: an im oberen Teil des Bandes, aus unterhalb der Mitte, Mindestzeiten ----
    bool want = s.humidDemand;
    if (humLimit)                          want = false;            // Grenze: sofort aus
    else if (tooDry)                       want = true;
    else if (v > curHi - 0.2f * w && fanAtMin) want = true;
    else if (v < mid)                      want = false;            // Nachlauf der Feuchte reicht bis in die untere Haelfte
    if (want != s.humidDemand) {
        uint32_t minT = s.humidDemand ? HUMID_MIN_ON_MS : HUMID_MIN_OFF_MS;
        if (humLimit || humidSwitchMs == 0 || now - humidSwitchMs >= minT) {
            s.humidDemand = want;
            humidSwitchMs = now;
            if (want) humidPulseMs = now;   // Takt beginnt mit einer An-Phase
        }
    }
    if (!vpdConfig.use_humid) s.humidDemand = false;
    s.humidWant = s.humidDemand &&
                  (now - humidPulseMs) % (HUMID_PULSE_ON_MS + HUMID_PULSE_OFF_MS) < HUMID_PULSE_ON_MS;

    // ---- Abluftluefter: schrittweise (integrierend) im Band halten, Grenzen haben Vorrang ----
    float scale = dtS / 5.0f;
    const bool cold = in.tempC <= vpdConfig.temp_min;
    if (tooHumid) {
        bool roomWet = in.roomOk && s.roomAbsHum >= s.tentAbsHum * 0.95f;
        if (cold)                r |= VPD_R_COLD;        // nicht weiter auskuehlen
        else if (roomWet)        r |= VPD_R_ROOM_WET;    // mehr Abluft bringt keine trockenere Luft
        else if (in.co2Dosing)   r |= VPD_R_CO2;         // CO2 nicht rausblasen
        // sanft hoch (max. 1 %/5 s = 12 %/min): nach einem Feuchte-Ueberschwinger nicht gleich
        // mit Hoechstdrehzahl das halbe Zelt austauschen -- unter 25 % laeuft der MARS ohnehin im Takt
        else fanCmd += fminf(fmaxf((curLo - v) * 10.0f, 0.3f), FAN_UP_STEP_MAX) * scale;
    } else if (tooDry) {
        fanCmd -= fminf(fmaxf((v - curHi) * 50.0f, 1.0f), FAN_STEP_MAX) * scale;
    } else if (s.humidDemand || v > mid) {
        // Obere Bandhaelfte (Richtung zu trocken) oder Befeuchter laeuft: Abluft zuruecknehmen,
        // bevor/statt Feuchte nachzuliefern -- je weiter ueber der Mitte, desto schneller
        float d = v > mid ? (v - mid) / (0.5f * w) : 0.0f;   // 0 = Mitte, 1 = Obergrenze
        fanCmd -= fminf(1.0f + 2.0f * d, FAN_STEP_MAX) * scale;
    }
    // Unter temp_min die VPD-Abluft langsam zuruecknehmen (1 %/5 s), Grenzen unten bleiben wirksam
    if (in.tempC < vpdConfig.temp_min) { fanCmd -= 1.0f * scale; r |= VPD_R_COLD; }
    fanCmd = constrain(fanCmd, fMin, fMax);
    if (tooHumid && fanCmd >= fMax - 0.5f) r |= VPD_R_FAN_MAX;

    float fanOut = fanCmd;
    if (in.tempC >= vpdConfig.temp_max - 1.0f) {
        float f = fMin + ramp01(in.tempC, vpdConfig.temp_max - 1.0f, vpdConfig.temp_max + 2.0f) * (fMax - fMin);
        if (f > fanOut) fanOut = f;
        if (in.tempC >= vpdConfig.temp_max) r |= VPD_R_TEMP_LIMIT;
    }
    if (humLimit) {
        float f = fMin + ramp01(in.humPct, vpdConfig.hum_max, vpdConfig.hum_max + 5.0f) * (fMax - fMin);
        if (f > fanOut) fanOut = f;
        r |= VPD_R_HUM_LIMIT;
    }
    s.fanPct = vpdConfig.use_fan ? (int)roundf(fanOut) : -1;

    r |= tooHumid ? VPD_R_TOO_HUMID : tooDry ? VPD_R_TOO_DRY : VPD_R_IN_BAND;
    s.reasons   = r;
    s.outOfBand = tooHumid || tooDry;
    s.valid     = true;
}
