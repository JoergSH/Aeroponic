#include "sensor_manager.h"
#include "scd41_handler.h"
#include "as7341_handler.h"
#include <Arduino.h>
#include <Wire.h>

#define SENSOR_READ_INTERVAL_MS  10000UL
// Muss den schlimmsten Fall beim Auto-Gain abdecken, nicht nur eine einzelne Messung: ab
// GAIN_1X (Index 1) kann die Zustandsmaschine bis zu 10x Gain anpassen + neu messen, jede Runde
// ~250-300ms (2 Integrationsphasen a ca. 100ms + I2C-Overhead) -- im Extremfall ca. 3s. Ohne
// Timeout bliebe die Zustandsmaschine bei einem echten Hang fuer immer in SM_AS7341_WAIT.
//
// Hinweis (23.10.2026): die frueher hier beobachteten "AS7341-Haenger" waren KEIN Sensor-,
// Auto-Gain- oder Timeout-Laengen-Problem, sondern ein unsigned-Ueberlauf in der
// Timeout-Pruefung unten (Vergleich mit `now`, das vor as7341_wait_start_ms gesetzt wurde) --
// der Timeout feuerte dadurch direkt nach jedem Messstart.
#define AS7341_TIMEOUT_MS       6000UL
// Bleibt der Sensor nach 3 Reinit-Versuchen haengen, wird AS7341 fuer eine Weile abgeschaltet
// (Board/SCD41/RS485/Luefter/Analog bleiben online) und danach automatisch neu versucht.
#define AS7341_DISABLE_MS       (10UL * 60UL * 1000UL)

typedef enum {
    SM_IDLE = 0,
    SM_AS7341_WAIT,
} sm_state_t;

static sm_state_t  sm_state        = SM_IDLE;
static uint32_t    last_measure_ms = 0;
static uint32_t    as7341_wait_start_ms = 0;
static uint8_t     as7341_recovery_cnt  = 0;
static bool        as7341_disabled          = false;
static uint32_t    as7341_disabled_until_ms = 0;
static bool        data_ready      = false;
static bool        scd41_ready     = false;
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
    scd41_ready = true;   // unabhaengig von AS7341 sofort verfuegbar, siehe sensor_manager.h

    if (as7341_disabled) return;   // wartet nur noch auf Reaktivierung, siehe SM_IDLE unten
    as7341_start();
    as7341_wait_start_ms = millis();
    sm_state = SM_AS7341_WAIT;
}

void sensor_manager_loop() {
    uint32_t now = millis();

    if (sm_state == SM_IDLE) {
        if (as7341_disabled && now > as7341_disabled_until_ms) {
            Serial.println("[SM] AS7341-Pause abgelaufen -- neuer Versuch");
            as7341_disabled      = false;
            as7341_recovery_cnt  = 0;
            as7341_init();
        }
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
            as7341_recovery_cnt = 0;
        // millis() statt `now`: start_cycle() setzt as7341_wait_start_ms erst NACH dem
        // SCD41-Lesen, also spaeter als `now` -- `now - start` lief dann im selben Durchlauf
        // unsigned ueber (~4 Mrd.) und loeste den Timeout sofort nach dem Messstart aus.
        } else if (millis() - as7341_wait_start_ms > AS7341_TIMEOUT_MS) {
            as7341_recovery_cnt++;
            if (as7341_recovery_cnt >= 3) {
                Serial.printf("[SM] AS7341 %d× haengen geblieben — deaktiviere fuer %lu Minuten "
                              "(Board/SCD41/RS485 bleiben online)\n",
                              as7341_recovery_cnt, AS7341_DISABLE_MS / 60000UL);
                as7341_disabled          = true;
                as7341_disabled_until_ms = now + AS7341_DISABLE_MS;
                as7341_recovery_cnt      = 0;
                // Ungueltige Werte signalisieren (Gain=0xFF Sentinel, siehe as7341_handler.cpp)
                // statt alte Messwerte fuer die ganze Pause einzufrieren.
                latest.ppfd_dppfd  = 0;
                latest.ch_415nm    = 0;
                latest.ch_445nm    = 0;
                latest.ch_480nm    = 0;
                latest.ch_515nm    = 0;
                latest.ch_555nm    = 0;
                latest.ch_590nm    = 0;
                latest.ch_630nm    = 0;
                latest.ch_680nm    = 0;
                latest.as7341_gain = 0xFF;
                data_ready = true;
                sm_state   = SM_IDLE;
            } else {
                // Kein i2c_hard_reset()/scd41_reinit(): ein Bus-Reset wegen AS7341 wuerde den
                // einwandfrei laufenden SCD41 unnoetig mitreissen -- nur AS7341 neu adressieren.
                Serial.printf("[SM] AS7341 haengt (Versuch %d/3) — Reinit...\n", as7341_recovery_cnt);
                as7341_init();
                sm_state = SM_IDLE;
            }
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

bool sensor_manager_as7341_ok() { return as7341_is_ok() && !as7341_disabled; }

bool sensor_manager_scd41_ready() {
    bool r = scd41_ready;
    scd41_ready = false;
    return r;
}

void sensor_manager_get_scd41(int16_t* temp_cdeg, uint16_t* hum_cpct, uint16_t* co2_ppm) {
    *temp_cdeg = latest.temperature_cdeg;
    *hum_cpct  = latest.humidity_cpct;
    *co2_ppm   = latest.co2_ppm;
}
