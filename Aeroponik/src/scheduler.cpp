#include "scheduler.h"
#include "config.h"
#include <EEPROM.h>
#include <math.h>

LightScheduleConfig schedConfig;
LichtKombiConfig    kombiConfig = { 100, 0x0F };

static void loadKombiConfig() {
    if (EEPROM.read(EEPROM_LICHTKOMBI_MAGIC_ADDR) == EEPROM_LICHTKOMBI_MAGIC_BYTE) {
        kombiConfig.max_pct = EEPROM.read(EEPROM_LICHTKOMBI_BASE);
        kombiConfig.mask    = EEPROM.read(EEPROM_LICHTKOMBI_BASE + 1) & 0x0F;
        if (kombiConfig.max_pct < 1 || kombiConfig.max_pct > 100) kombiConfig.max_pct = 100;
    } else {
        kombiConfig = { 100, 0x0F };
        saveKombiConfig();
    }
}

void saveKombiConfig() {
    EEPROM.write(EEPROM_LICHTKOMBI_BASE,     kombiConfig.max_pct);
    EEPROM.write(EEPROM_LICHTKOMBI_BASE + 1, kombiConfig.mask);
    EEPROM.write(EEPROM_LICHTKOMBI_MAGIC_ADDR, EEPROM_LICHTKOMBI_MAGIC_BYTE);
    EEPROM.commit();
}

void loadScheduleConfig() {
    if (EEPROM.read(EEPROM_SCHED_MAGIC_ADDR) == EEPROM_SCHED_MAGIC_BYTE) {
        EEPROM.get(EEPROM_SCHED_BASE, schedConfig);
    } else {
        schedConfig.enabled        = false;
        schedConfig.manual_percent = 0;
        schedConfig.dawn_start  = SCHED_DEFAULT_DAWN_START;
        schedConfig.dawn_end    = SCHED_DEFAULT_DAWN_END;
        schedConfig.dusk_start  = SCHED_DEFAULT_DUSK_START;
        schedConfig.dusk_end    = SCHED_DEFAULT_DUSK_END;
        schedConfig.num_ssr     = SCHED_DEFAULT_NUM_SSR;
        saveScheduleConfig();
    }
    loadKombiConfig();
}

void saveScheduleConfig() {
    EEPROM.put(EEPROM_SCHED_BASE, schedConfig);
    EEPROM.write(EEPROM_SCHED_MAGIC_ADDR, EEPROM_SCHED_MAGIC_BYTE);
    EEPROM.commit();
}

// Alle Zeitpunkte relativ zum Sonnenaufgang (dawn_start = 0) statt zu Mitternacht. Damit
// funktionieren Fenster ueber Mitternacht ohne Sonderfaelle -- z.B. Untergang 23:00-00:10:
// absolut waere dusk_end (10) < dusk_start (1380), "now >= dusk_end" galt dann ab 00:10 den
// ganzen Tag und das Licht blieb aus. Relativ liegen alle Punkte aufsteigend in 0..1440.
// dusk_end == dawn_start bedeutet 24 h Licht (dd = 1440 statt 0).
struct SchedRel { int t, de, ds, dd; };

static SchedRel schedRel(uint16_t now_min) {
    auto rel = [](int x) { return ((x - (int)schedConfig.dawn_start) % 1440 + 1440) % 1440; };
    SchedRel r = { rel(now_min), rel(schedConfig.dawn_end), rel(schedConfig.dusk_start),
                   rel(schedConfig.dusk_end) };
    if (r.dd == 0) r.dd = 1440;
    // Unsinnige Reihenfolge (z.B. Untergang vor Aufgangsende) abfangen statt Unsinn zu rechnen
    if (r.ds < r.de) r.ds = r.de;
    if (r.dd < r.ds) r.dd = r.ds;
    return r;
}

uint8_t schedComputeRelayMask(uint16_t now_min) {
    uint8_t n = schedConfig.num_ssr;
    if (n == 0 || n > 4) n = 4;
    SchedRel r = schedRel(now_min);

    if (r.t >= r.dd)
        return 0x00;
    if (r.t >= r.de && r.t < r.ds)
        return (uint8_t)((1 << n) - 1);  // alle n SSRs an

    // e = Fensterfortschritt 0.0 (Fensterbeginn) → 1.0 (Fensterende), getrennt fuer
    // Auf-/Untergang berechnet (NICHT dieselbe Formel fuer beide — das war der Bug:
    // eine gemeinsame "t"-Variable mit ceil() macht die erste Auf-Stufe zwar sofort
    // scharf, laesst aber beim Untergang die erste Ab-Stufe symmetrisch falsch erst
    // nach 1/n des Fensters greifen, weil ceil(n-x) mathematisch n-floor(x) entspricht,
    // nicht n-ceil(x)).
    uint8_t active;
    if (r.t < r.de) {
        // Aufgang: Stufe 1 zuendet sofort nach Fensterbeginn, Stufe n exakt am Ende.
        float e = (float)r.t / (float)r.de;
        if (e < 0.0f) e = 0.0f;
        if (e > 1.0f) e = 1.0f;
        active = (uint8_t)ceilf(e * n);
    } else {
        // Untergang: erste Stufe dimmt sofort nach Fensterbeginn ab, 0 exakt am Ende.
        float e = (float)(r.t - r.ds) / (float)(r.dd - r.ds);
        if (e < 0.0f) e = 0.0f;
        if (e > 1.0f) e = 1.0f;
        uint8_t drop = (uint8_t)ceilf(e * n);
        active = (drop >= n) ? 0 : n - drop;
    }
    if (active > n) active = n;
    return (active == 0) ? 0x00 : (uint8_t)((1 << active) - 1);
}

// Fuer den analogen 0-10V-Lichtausgang: dieselben Zeitfenster wie oben, aber ohne
// Stufen-Quantisierung — ein Analogsignal kann stufenlos der Rampe folgen, daher
// bewusst eine eigene, einfachere Berechnung statt schedComputeRelayMask() anzufassen.
uint8_t schedComputeBrightnessPercent(uint16_t now_min) {
    SchedRel r = schedRel(now_min);
    if (r.t >= r.dd) return 0;
    if (r.t >= r.de && r.t < r.ds) return 100;

    float frac;
    if (r.t < r.de) {
        // Aufgang: 0.0 -> 1.0
        frac = (float)r.t / (float)r.de;
    } else {
        // Untergang: 1.0 -> 0.0
        frac = 1.0f - (float)(r.t - r.ds) / (float)(r.dd - r.ds);
    }
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    return (uint8_t)roundf(frac * 100.0f);
}

void schedComputeKombi(uint16_t now_min, uint8_t& analogPct, uint8_t& mask) {
    analogPct = (uint8_t)roundf(schedComputeBrightnessPercent(now_min) * kombiConfig.max_pct / 100.0f);
    SchedRel r = schedRel(now_min);
    // Zusatzkanaele nur im Tag-Abschnitt: morgens erst, wenn die Hauptlampe ihr Max erreicht hat,
    // abends sofort mit Beginn des Untergangs aus (danach dimmt die Hauptlampe herunter).
    mask = (r.t >= r.de && r.t < r.ds) ? kombiConfig.mask : 0x00;
}

void schedComputeKombiManual(uint8_t manualPct, uint8_t& analogPct, uint8_t& mask) {
    analogPct = (uint8_t)roundf(manualPct * kombiConfig.max_pct / 100.0f);
    mask = (manualPct >= 100) ? kombiConfig.mask : 0x00;
}

bool schedLichtIstAn(uint16_t now_min) {
    if (!schedConfig.enabled) return schedConfig.manual_percent > 0;
    SchedRel r = schedRel(now_min);
    return r.t < r.dd;   // Aufgang bis Ende Untergang, auch ueber Mitternacht
}
