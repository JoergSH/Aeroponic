#include "scd41_handler.h"
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#include <Arduino.h>

static SensirionI2cScd4x scd4x;
static bool    error_flag  = false;
static uint8_t error_count = 0;

bool scd41_init() {
    scd4x.begin(Wire, SCD41_I2C_ADDR_62);
    scd4x.stopPeriodicMeasurement();
    delay(500);
    int16_t err = scd4x.startPeriodicMeasurement();
    if (err) {
        Serial.printf("[SCD41] startPeriodicMeasurement Fehler: %d\n", err);
        error_flag = true;
        return false;
    }
    error_flag  = false;
    error_count = 0;
    Serial.println("[SCD41] Periodische Messung gestartet (alle 5s)");
    return true;
}

bool scd41_data_ready() {
    bool ready = false;
    int16_t err = scd4x.getDataReadyStatus(ready);
    if (err) return false;
    return ready;
}

bool scd41_read(int16_t* temp_cdeg, uint16_t* hum_cpct, uint16_t* co2_ppm) {
    uint16_t co2;
    float    temp, hum;
    int16_t err = scd4x.readMeasurement(co2, temp, hum);
    if (err || co2 == 0) {
        error_count++;
        if (error_count >= 3) error_flag = true;
        Serial.printf("[SCD41] Lesefehler %d (count=%d)\n", err, error_count);
        return false;
    }
    error_count = 0;
    error_flag  = false;
    *temp_cdeg  = (int16_t)(temp * 100.0f);
    *hum_cpct   = (uint16_t)(hum  * 100.0f);
    *co2_ppm    = co2;
    Serial.printf("[SCD41] CO2=%u ppm  T=%.1f°C  H=%.1f%%\n", co2, temp, hum);
    return true;
}

bool scd41_has_error() { return error_flag; }

void scd41_reinit() {
    error_count = 0;
    error_flag  = false;
    scd41_init();
}
