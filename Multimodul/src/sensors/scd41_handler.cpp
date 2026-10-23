#include "scd41_handler.h"
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#include <Arduino.h>

// Haengt an Wire1 (I2C1, siehe pinout.h) statt am Standard-Wire (I2C0).
static SensirionI2cScd4x scd4x;
static bool    error_flag  = false;
static uint8_t error_count = 0;

bool scd41_init() {
    scd4x.begin(Wire1, SCD41_I2C_ADDR_62);
    scd4x.stopPeriodicMeasurement();
    delay(500);
    int16_t err = scd4x.startPeriodicMeasurement();
    if (err) {
        // Nicht gefunden/nicht gestartet ist kein Fehlerzustand im Sinne der Hang-Recovery --
        // analog zu as7341_handler.cpp wird das nur als "keine Daten" behandelt, OHNE
        // error_flag zu setzen. Sonst wuerde ein schlicht fehlender/nicht verbundener Sensor
        // denselben 3-Versuche-dann-Neustart-Pfad wie ein echter Laufzeit-Hang ausloesen
        // (siehe sensor_manager.cpp) und das Board endlos neu starten.
        Serial.printf("[SCD41] nicht gefunden/startPeriodicMeasurement Fehler: %d\n", err);
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
    // Serial.printf("%f") liefert auf diesem Core teils leere Ausgabe statt der Zahl (fehlende
    // Gleitkomma-Unterstuetzung) -- String(..,1) umgeht das zuverlaessig.
    Serial.printf("[SCD41] CO2=%u ppm  T=%s°C  H=%s%%\n",
                  co2, String(temp, 1).c_str(), String(hum, 1).c_str());
    return true;
}

bool scd41_has_error() { return error_flag; }

void scd41_reinit() {
    error_count = 0;
    error_flag  = false;
    scd41_init();
}
