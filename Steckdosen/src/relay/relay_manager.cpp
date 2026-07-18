#include "relay_manager.h"
#include <Arduino.h>
#include <Preferences.h>

// HIGH-aktiv SSR-Modul (HIGH = Relais EIN)
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

static const uint8_t RELAY_PINS[RELAY_COUNT] = {1, 2, 3, 4};
static uint8_t current_mask = 0x00;
static Preferences prefs;

static void apply_mask(uint8_t mask) {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        bool on = (mask >> i) & 0x01;
        digitalWrite(RELAY_PINS[i], on ? RELAY_ON : RELAY_OFF);
    }
    current_mask = mask;
}

void relay_manager_init() {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], RELAY_OFF);  // sicher AUS beim Start
    }

    prefs.begin("relay", true);
    uint8_t saved = prefs.getUChar("mask", 0x00);
    prefs.end();

    Serial.printf("[RELAY] Gespeicherter Zustand: 0x%02X\n", saved);
    apply_mask(saved);
}

void relay_manager_set_mask(uint8_t mask) {
    mask &= 0x0F;  // nur 4 Bits gueltig
    apply_mask(mask);

    prefs.begin("relay", false);
    prefs.putUChar("mask", mask);
    prefs.end();

    Serial.printf("[RELAY] Maske -> 0x%02X  (R1=%d R2=%d R3=%d R4=%d)\n",
                  mask,
                  (mask >> 0) & 1, (mask >> 1) & 1,
                  (mask >> 2) & 1, (mask >> 3) & 1);
}

uint8_t relay_manager_get_mask() {
    return current_mask;
}
