#include "sensor_manager.h"
#include "scd41_handler.h"
#include "as7341_handler.h"
#include <Arduino.h>
#include <Wire.h>

#define SENSOR_READ_INTERVAL_MS  10000UL

typedef enum {
    SM_IDLE = 0,
    SM_AS7341_WAIT,
} sm_state_t;

static sm_state_t  sm_state        = SM_IDLE;
static uint32_t    last_measure_ms = 0;
static bool        data_ready      = false;
static sensor_data_t latest;

static uint8_t g_sda          = 10;
static uint8_t g_scl          = 11;
static uint8_t g_recovery_cnt = 0;

// I2C1-Bus-Recovery (Bit-Bang-Clockout), identisch zum Multisensor-Original, nur auf
// Wire1/setSDA/setSCL (RP2040-I2C1) umgestellt statt Wire.begin(sda,scl) (ESP32-Stil).
static void i2c_hard_reset() {
    Serial.println("[SM] I2C Hard-Reset...");
    Wire1.end();
    delay(50);
    pinMode(g_sda, INPUT_PULLUP);
    pinMode(g_scl, INPUT_PULLUP);
    delay(10);
    if (digitalRead(g_sda) == LOW) {
        pinMode(g_scl, OUTPUT);
        for (int i = 0; i < 9; i++) {
            digitalWrite(g_scl, LOW);  delayMicroseconds(10);
            digitalWrite(g_scl, HIGH); delayMicroseconds(10);
            if (digitalRead(g_sda) == HIGH) break;
        }
        pinMode(g_sda, OUTPUT);
        digitalWrite(g_sda, LOW);  delayMicroseconds(10);
        digitalWrite(g_scl, HIGH); delayMicroseconds(10);
        digitalWrite(g_sda, HIGH); delayMicroseconds(10);
        pinMode(g_sda, INPUT_PULLUP);
        pinMode(g_scl, INPUT_PULLUP);
        delay(10);
    }
    Wire1.setSDA(g_sda);
    Wire1.setSCL(g_scl);
    Wire1.begin();
    Wire1.setClock(50000);
    delay(200);
}

bool sensor_manager_init(uint8_t sda_pin, uint8_t scl_pin) {
    g_sda = sda_pin;
    g_scl = scl_pin;
    bool scd_ok = scd41_init();
    bool as_ok  = as7341_init();
    Serial.printf("[SM] Sensor-Init: SCD41=%d AS7341=%d\n", scd_ok, as_ok);
    return scd_ok;
}

static void start_cycle() {
    bool ok = scd41_read(&latest.temperature_cdeg, &latest.humidity_cpct, &latest.co2_ppm);
    if (!ok) return;
    as7341_start();
    sm_state = SM_AS7341_WAIT;
}

void sensor_manager_loop() {
    uint32_t now = millis();

    if (sm_state == SM_IDLE) {
        bool time_ok = (now - last_measure_ms >= SENSOR_READ_INTERVAL_MS) || (last_measure_ms == 0);
        if (time_ok && scd41_data_ready()) {
            last_measure_ms = now;
            start_cycle();
        }
    }

    if (sm_state == SM_AS7341_WAIT) {
        if (as7341_update()) {
            as7341_data_t ad;
            as7341_get_data(&ad);
            latest.ppfd_dppfd  = ad.ppfd_dppfd;
            latest.ch_415nm    = ad.ch_415nm;
            latest.ch_445nm    = ad.ch_445nm;
            latest.ch_480nm    = ad.ch_480nm;
            latest.ch_515nm    = ad.ch_515nm;
            latest.ch_555nm    = ad.ch_555nm;
            latest.ch_590nm    = ad.ch_590nm;
            latest.ch_630nm    = ad.ch_630nm;
            latest.ch_680nm    = ad.ch_680nm;
            latest.as7341_gain = ad.gain;
            data_ready = true;
            sm_state   = SM_IDLE;
        }
    }

    if (scd41_has_error()) {
        g_recovery_cnt++;
        if (g_recovery_cnt >= 3) {
            Serial.printf("[SM] %d× Recovery erfolglos — Neustart!\n", g_recovery_cnt);
            delay(200);
            rp2040.reboot();
        }
        Serial.printf("[SM] SCD41 Fehler — I2C Hard-Reset (Versuch %d/3)...\n", g_recovery_cnt);
        i2c_hard_reset();
        scd41_reinit();
        bool ok = !scd41_has_error();
        Serial.printf("[SM] Recovery: SCD41=%d\n", ok);
        if (ok) g_recovery_cnt = 0;
    }
}

bool sensor_manager_data_ready() { return data_ready; }

void sensor_manager_get_data(sensor_data_t* out) {
    *out       = latest;
    data_ready = false;
}

bool sensor_manager_as7341_ok() { return as7341_is_ok(); }
