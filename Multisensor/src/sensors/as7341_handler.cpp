#include "as7341_handler.h"
#include <Adafruit_AS7341.h>
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

// MLR-Koeffizienten für PPFD-Berechnung
// Rahman et al. (2025), doi:10.20944/preprints202504.1750.v1, Table 6
// Kalibriert bei: GAIN_1X, ATIME=35, ASTEP=999 (≈100ms), R²=0.9998
static const float MLR_B0   = -1.83008f;
static const float MLR_B[8] = {
    -0.10893f, -0.19323f,  0.149401f, 0.234282f,
     0.019283f, -0.162300f, 0.133297f, 0.087622f,
};

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

bool as7341_init() {
    initialized = as7341.begin();
    if (!initialized) {
        Serial.println("[AS7341] nicht gefunden -> Pseudo-Daten aktiv");
        return false;
    }
    as7341.setATIME(35);
    as7341.setASTEP(999);
    as7341.setGain(GAIN_ENUM[gain_idx]);
    Serial.println("[AS7341] OK (ATIME=35, ASTEP=999, GAIN_1X)");
    return true;
}

static void fill_fake_data() {
    // Plausible Testwerte — leicht variierend damit das Web-Interface lebt
    uint32_t t = millis() / 1000;
    result.ppfd_dppfd = 2500 + (t % 20) * 10;  // 250.0 – 269.0 µmol/m²/s
    result.ch_415nm   = 1200 + (t % 15) * 8;
    result.ch_445nm   = 1850 + (t % 12) * 10;
    result.ch_480nm   = 2100 + (t % 18) * 7;
    result.ch_515nm   = 1950 + (t % 10) * 12;
    result.ch_555nm   = 1700 + (t % 14) * 9;
    result.ch_590nm   = 1400 + (t % 16) * 8;
    result.ch_630nm   = 2200 + (t % 11) * 11;
    result.ch_680nm   = 2450 + (t % 13) * 9;
    result.gain       = GAIN_DEFAULT_IDX;
}

void as7341_start() {
    if (!initialized) {
        fill_fake_data();
        as_state = AS_DONE;
        return;
    }
    as7341.startReading();
    as_state = AS_FIRST_MEASURE;
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

    uint16_t max_ch  = 0;
    bool     all_dark = true;
    for (int i = 0; i < 8; i++) {
        if (ch[i] > max_ch) max_ch = ch[i];
        if (ch[i] >= 100) all_dark = false;
    }

    // Auto-Gain: nur beim ersten Durchlauf anpassen, dann neu messen
    if (as_state == AS_FIRST_MEASURE) {
        if (max_ch > 60000 && gain_idx > 0) {
            gain_idx--;
            as7341.setGain(GAIN_ENUM[gain_idx]);
            Serial.printf("[AS7341] Sättigung → Gain idx %d\n", gain_idx);
            as7341.startReading();
            as_state = AS_REMEASURE;
            return false;
        }
        if (all_dark && gain_idx < GAIN_TABLE_SIZE - 1) {
            gain_idx++;
            as7341.setGain(GAIN_ENUM[gain_idx]);
            Serial.printf("[AS7341] Zu dunkel → Gain idx %d\n", gain_idx);
            as7341.startReading();
            as_state = AS_REMEASURE;
            return false;
        }
    }

    // PPFD via MLR — Normierung auf GAIN_1X (Kalibrierungsreferenz, Index 1)
    float scale = 1.0f / GAIN_FACTOR[gain_idx];  // bei GAIN_1X: scale = 1.0
    float ppfd = MLR_B0;
    for (int i = 0; i < 8; i++) ppfd += MLR_B[i] * ((float)ch[i] * scale);
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

    as_state = AS_DONE;
    return true;
}

bool as7341_update() {
    if (!initialized) return as_state == AS_DONE;  // Fake-Daten: Zustand auswerten
    if (as_state == AS_IDLE || as_state == AS_DONE) return as_state == AS_DONE;

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
