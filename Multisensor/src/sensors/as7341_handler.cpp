#include "as7341_handler.h"
#include <Adafruit_AS7341.h>
#include <Wire.h>
#include <Arduino.h>

// Gain-Tabelle: Index → enum, Multiplikator
static const as7341_gain_t GAIN_ENUM[] = {
    AS7341_GAIN_0_5X,  // 0
    AS7341_GAIN_1X,    // 1  ← Default (Kalibrierungsreferenz)
    AS7341_GAIN_2X,    // 2
    AS7341_GAIN_4X,    // 3
    AS7341_GAIN_8X,    // 4
    AS7341_GAIN_16X,   // 5
    AS7341_GAIN_32X,   // 6
    AS7341_GAIN_64X,   // 7
    AS7341_GAIN_128X,  // 8
    AS7341_GAIN_256X,  // 9
    AS7341_GAIN_512X   // 10
};
static const float GAIN_FACTOR[] = {
    0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 512.0f
};
#define GAIN_DEFAULT_IDX  1   // GAIN_1X — Kalibrierungsreferenz
#define GAIN_TABLE_SIZE  11

// PPFD-Berechnung: physikalisches Modell statt MLR-Regression (Rahman et al. 2025) -- die MLR
// hatte negative Gewichte (F1,F2,F6) und lieferte bei Spektren, die den Trainingslampen nicht
// aehnelten, spuerbar falsche Werte. Vollstaendige Herleitung (Responsivity/Photonenenergie/
// Avogadro, Quelle ams AS7341 Datasheet DS000504 v3-00) siehe Licht-Meter/src/main.cpp --
// hier nur die Kurzfassung, identische Konstanten/Formel, K_CAL empirisch mit dem
// Licht-Meter-Handgeraet gegen ein Referenzmessgeraet ermittelt.
static const float CENTER_WL_NM[8]      = { 415, 445, 480, 515, 555, 590, 630, 680 };
static const float REF_RESPONSIVITY[8]  = { 55, 110, 210, 390, 590, 840, 1350, 1070 };  // counts bei Ee=107.67uW/cm^2
static const float REF_IRRADIANCE_UWCM2 = 107.67f;
static const float REF_GAIN_X           = 64.0f;
static const float REF_TINT_MS          = 27.8f;
static const float OUR_GAIN_X           = 1.0f;  // K_PPFD gilt fuer GAIN_1X-normierte Zaehlwerte (siehe scale unten)
static const double H_PLANCK = 6.62607015e-34;   // J*s
static const double C_LIGHT  = 2.99792458e8;     // m/s
static const double NA_UMOL  = 6.02214076e17;    // Teilchen pro Mikromol
static float K_PPFD[8];                          // in as7341_init() berechnet, siehe compute_ppfd_weights()
static const float K_CAL = 0.0731f;               // siehe Licht-Meter/src/main.cpp (gleicher Sensor/gleiche Einstellungen)

// ADC-Vollausschlag = (ATIME+1)*(ASTEP+1) (Datasheet Equation 2), bei ATIME=35/ASTEP=999 also
// 36000, NICHT 65535 -- die alte feste Schwelle 60000 fuer die Auto-Gain-Saettigungserkennung
// konnte dadurch nie erreicht werden.
static const uint32_t ADC_MAX        = (uint32_t)(35 + 1) * (999 + 1);
static const uint32_t SAT_THRESHOLD  = (uint32_t)(ADC_MAX * 0.97f);

static void compute_ppfd_weights() {
    float ourTintMs = (35 + 1) * (999 + 1) * 2.78f / 1000.0f;  // ATIME=35/ASTEP=999, siehe as7341_init()
    double scale = (OUR_GAIN_X / REF_GAIN_X) * (ourTintMs / REF_TINT_MS);
    for (int i = 0; i < 8; i++) {
        double rePerUwcm2Ref = REF_RESPONSIVITY[i] / REF_IRRADIANCE_UWCM2;
        double reOurs   = rePerUwcm2Ref * scale;
        double ePhotonJ = H_PLANCK * C_LIGHT / (CENTER_WL_NM[i] * 1e-9);
        K_PPFD[i] = (float)(0.01 / (reOurs * ePhotonJ * NA_UMOL));  // 0.01 = uW/cm^2 -> W/m^2
    }
}

typedef enum {
    AS_IDLE = 0,
    AS_FIRST_MEASURE,
    AS_REMEASURE,
    AS_DONE
} as_state_t;

static Adafruit_AS7341 as7341;
static bool      initialized = false;
static int       gain_idx    = GAIN_DEFAULT_IDX;
static as_state_t as_state  = AS_IDLE;
static as7341_data_t result;
static void start_reading();  // siehe unten (setzt zusaetzlich sat_flags zurueck)

bool as7341_init() {
    initialized = as7341.begin();
    if (!initialized) {
        Serial.println("[AS7341] nicht gefunden");
        return false;
    }
    as7341.setATIME(35);
    as7341.setASTEP(999);
    as7341.setGain(GAIN_ENUM[gain_idx]);
    compute_ppfd_weights();
    Serial.println("[AS7341] OK (ATIME=35, ASTEP=999, GAIN_1X)");
    return true;
}

bool as7341_is_ok() { return initialized; }

static void fill_invalid_data() {
    // Sensor nicht vorhanden/defekt: alles 0, keine erfundenen Werte mehr. Gueltigkeit
    // wird primaer ueber as7341_is_ok() / RS485_REG_STATUS Bit1 signalisiert; gain=0xFF
    // (kein echter Gain-Index, gueltig sind nur 0-10) dient zusaetzlich als Sentinel fuer
    // Master-Seiten, die nur das ESP-NOW-Lichtpaket sehen (kein eigenes Status-Feld dort).
    result.ppfd_dppfd = 0;
    result.ch_415nm   = 0;
    result.ch_445nm   = 0;
    result.ch_480nm   = 0;
    result.ch_515nm   = 0;
    result.ch_555nm   = 0;
    result.ch_590nm   = 0;
    result.ch_630nm   = 0;
    result.ch_680nm   = 0;
    result.gain       = 0xFF;
}

void as7341_start() {
    if (!initialized) {
        // Kein Sensor: Zyklus nicht blockieren, sofort mit ungueltigen (0er) Werten
        // "fertig" melden, damit die SCD41-Daten trotzdem gesendet werden.
        fill_invalid_data();
        as_state = AS_DONE;
        return;
    }
    start_reading();
    as_state = AS_FIRST_MEASURE;
}

// Saettigungs-Flags aus STATUS2 (0xA3): Bit4 = ASAT_DIG (Zaehler voll), Bit3 = ASAT_ANA
// (analoger Frontend-Teil gesaettigt). Waehrend einer laufenden Messung (beide SMUX-Phasen)
// gesammelt, da die Lib nur auf AVALID schaut. Noetig, weil bei hohem Gain und langsam
// steigendem Licht zuerst die ANALOGE Saettigung einsetzt -- die Zaehlwerte bleiben dann auf
// einem Plateau unterhalb von SAT_THRESHOLD stehen, die reine Zaehlwert-Pruefung greift nie
// und der Gain wird nicht heruntergeregelt.
static uint8_t sat_flags = 0;

static uint8_t read_status2() {
    Wire.beginTransmission(AS7341_I2CADDR_DEFAULT);
    Wire.write(0xA3);
    if (Wire.endTransmission(false) != 0) return 0;
    if (Wire.requestFrom((uint8_t)AS7341_I2CADDR_DEFAULT, (uint8_t)1) != 1) return 0;
    return Wire.read();
}

static void start_reading() {
    sat_flags = 0;
    as7341.startReading();
}

static bool process_reading() {
    uint16_t raw[12];
    if (!as7341.getAllChannels(raw)) return false;

    // readAllChannels-Layout:
    // [0]=F1(415) [1]=F2(445) [2]=F3(480) [3]=F4(515) [4]=CLEAR_0 [5]=NIR_0
    // [6]=F5(555) [7]=F6(590) [8]=F7(630) [9]=F8(680) [10]=CLEAR  [11]=NIR
    uint16_t ch[8] = {
        raw[0], raw[1], raw[2], raw[3],   // 415, 445, 480, 515
        raw[6], raw[7], raw[8], raw[9]    // 555, 590, 630, 680
    };
    uint16_t clearVal = raw[10];

    uint16_t max_ch  = 0;
    bool     all_dark = true;
    for (int i = 0; i < 8; i++) {
        if (ch[i] > max_ch) max_ch = ch[i];
        if (ch[i] >= 100) all_dark = false;
    }
    if (clearVal > max_ch) max_ch = clearVal;  // auch sehr breitbandiges/weisses Licht erkennen
    if (clearVal >= 100) all_dark = false;     // sonst Oszillation moeglich: Clear saettigt
                                                // (Gain runter), aber F1-F8 gelten bei halbem Gain
                                                // dann als "dunkel" (Gain wieder hoch), usw.

    // Auto-Gain: bei Saettigung/Dunkelheit anpassen und neu messen -- auch nach einem
    // vorherigen Schritt (AS_REMEASURE) erneut pruefen, damit bei starken Lichtspruengen
    // (z.B. gestaffelte Lichtsteigerung) der Gain in einem Zyklus bis zum richtigen Wert
    // konvergiert, statt pro Poll-Zyklus nur einen Schritt zu machen.
    if (as_state == AS_FIRST_MEASURE || as_state == AS_REMEASURE) {
        bool saturated = (max_ch > SAT_THRESHOLD) || (sat_flags & 0x18);
        if (saturated && gain_idx > 0) {
            gain_idx--;
            as7341.setGain(GAIN_ENUM[gain_idx]);
            Serial.printf("[AS7341] Sättigung (max=%u%s%s) → Gain idx %d\n", max_ch,
                          (sat_flags & 0x08) ? " ANA" : "", (sat_flags & 0x10) ? " DIG" : "",
                          gain_idx);
            start_reading();
            as_state = AS_REMEASURE;
            return false;
        }
        if (all_dark && gain_idx < GAIN_TABLE_SIZE - 1) {
            gain_idx++;
            as7341.setGain(GAIN_ENUM[gain_idx]);
            Serial.printf("[AS7341] Zu dunkel → Gain idx %d\n", gain_idx);
            start_reading();
            as_state = AS_REMEASURE;
            return false;
        }
    }

    // PPFD via physikalisches Modell — Normierung auf GAIN_1X (Kalibrierungsreferenz, Index 1),
    // K_PPFD ist fuer genau diese Normierung hergeleitet (siehe compute_ppfd_weights()).
    float scale = 1.0f / GAIN_FACTOR[gain_idx];  // bei GAIN_1X: scale = 1.0
    float ppfd = 0.0f;
    for (int i = 0; i < 8; i++) ppfd += K_PPFD[i] * ((float)ch[i] * scale);
    ppfd *= K_CAL;
    if (ppfd < 0.0f) ppfd = 0.0f;

    result.ppfd_dppfd = (uint16_t)(ppfd * 10.0f + 0.5f);
    result.ch_415nm   = ch[0];
    result.ch_445nm   = ch[1];
    result.ch_480nm   = ch[2];
    result.ch_515nm   = ch[3];
    result.ch_555nm   = ch[4];
    result.ch_590nm   = ch[5];
    result.ch_630nm   = ch[6];
    result.ch_680nm   = ch[7];
    result.gain       = (uint8_t)gain_idx;

    Serial.printf("[AS7341] Gain %sx  max=%u  PPFD=%s%s\n",
                  String(GAIN_FACTOR[gain_idx], 1).c_str(), max_ch, String(ppfd, 1).c_str(),
                  (sat_flags & 0x18) ? "  (gesaettigt, Gain bereits minimal!)" : "");

    as_state = AS_DONE;
    return true;
}

bool as7341_update() {
    if (!initialized) return as_state == AS_DONE;  // kein Sensor: sofort "fertig" mit 0er-Werten
    if (as_state == AS_IDLE || as_state == AS_DONE) return as_state == AS_DONE;

    sat_flags |= read_status2() & 0x18;
    if (as7341.checkReadingProgress()) {
        return process_reading();
    }
    return false;
}

bool as7341_get_data(as7341_data_t* out) {
    if (as_state != AS_DONE) return false;
    *out     = result;
    as_state = AS_IDLE;
    return true;
}
