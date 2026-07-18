#include "sensor_manager.h"
#include "scd41_handler.h"
#include "as7341_handler.h"
#include <Arduino.h>
#include <Wire.h>

#define SENSOR_READ_INTERVAL_MS_DEFAULT  10000UL

typedef enum {
    SM_IDLE = 0,
    SM_AS7341_WAIT,
} sm_state_t;

static sm_state_t  sm_state        = SM_IDLE;
static uint32_t    last_measure_ms = 0;
static uint32_t    interval_ms     = SENSOR_READ_INTERVAL_MS_DEFAULT;
static bool        force_measure   = false;
static bool        send_raw_next   = false;
static bool        data_ready      = false;
static sensor_data_t latest;

static bool    sensors_ok     = false;
static uint8_t g_sda          = 8;
static uint8_t g_scl          = 9;
static uint8_t g_recovery_cnt = 0;

static void i2c_hard_reset() {
    Serial.println("[SM] I2C Hard-Reset...");
    Wire.end();
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
    Wire.begin(g_sda, g_scl);
    Wire.setClock(50000);
    Wire.setTimeOut(500);
    delay(200);
}

sm_init_result_t sensor_manager_init(uint8_t sda_pin, uint8_t scl_pin) {
    g_sda = sda_pin;
    g_scl = scl_pin;
    bool scd_ok = scd41_init();
    bool as_ok  = as7341_init();
    Serial.printf("[SM] Sensor-Init: SCD41=%d AS7341=%d%s\n",
                  scd_ok, as_ok, as_ok ? "" : " (PSEUDO)");
    sensors_ok = scd_ok;
    return sensors_ok ? SM_OK : SM_INIT_FAILED;
}

static void start_cycle() {
    bool ok = scd41_read(&latest.temperature_cdeg, &latest.humidity_cpct, &latest.co2_ppm);
    if (!ok) return;
    as7341_start();
    sm_state = SM_AS7341_WAIT;
}

void sensor_manager_loop() {
    if (!sensors_ok) return;

    uint32_t now = millis();

    if (sm_state == SM_IDLE) {
        bool time_ok = (now - last_measure_ms >= interval_ms) || (last_measure_ms == 0);
        bool force   = force_measure;
        if (force) force_measure = false;
        if ((time_ok || force) && scd41_data_ready()) {
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
            ESP.restart();
        }
        Serial.printf("[SM] SCD41 Fehler — I2C Hard-Reset (Versuch %d/3)...\n", g_recovery_cnt);
        i2c_hard_reset();
        scd41_reinit();
        bool ok = !scd41_has_error();
        Serial.printf("[SM] Recovery: SCD41=%d\n", ok);
        if (ok) g_recovery_cnt = 0;
    }
}

void sensor_manager_on_command(const uint8_t* payload, uint8_t len) {
    if (len < 1) return;
    switch (payload[0]) {
        case CMD_SET_INTERVAL:
            if (len >= 3) {
                uint16_t secs = ((uint16_t)payload[1] << 8) | payload[2];
                interval_ms = (uint32_t)secs * 1000UL;
                Serial.printf("[SM] Intervall → %u s\n", secs);
            }
            break;
        case CMD_REQUEST_DATA:
            force_measure = true;
            Serial.println("[SM] Sofortmessung angefordert");
            break;
        case CMD_REQUEST_RAWSPEC:
            send_raw_next = true;
            Serial.println("[SM] Rohspektrum beim nächsten Senden aktiviert");
            break;
        default:
            break;
    }
}

bool sensor_manager_data_ready() { return data_ready; }

void sensor_manager_get_data(sensor_data_t* out) {
    *out       = latest;
    data_ready = false;
}

bool sensor_manager_send_raw_next() {
    bool v = send_raw_next;
    send_raw_next = false;
    return v;
}
