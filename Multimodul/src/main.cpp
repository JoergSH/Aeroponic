#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "pinout.h"
#include "rs485_slave.h"
#include "gp8403.h"
#include "sensors/sensor_manager.h"
#include "sensors/scd41_handler.h"

// Multimodul (RP2040-Zero): Basis ist das Lueftermodul (4x PWM-Luefter + Tacho, Modbus-RTU
// Adresse 0x51) -- zusaetzlich haengt an I2C1 (SDA=10/SCL=11) optionale Peripherie: SCD41
// (CO2), AS7341 (Licht/PPFD) und GP8403 (2-Kanal 0-10V-DAC). Alle drei werden beim Boot per
// I2C-Scan erkannt; fehlt ein Geraet, bleibt die zugehoerige RS485-Adresse (MS2=0x20,
// Analog=0x50) unbeantwortet -- der Master kennt diese Adressen schon unabhaengig vom
// Lueftermodul und behandelt eine fehlende Antwort wie ein nicht verbautes Geraet.
// Dadurch sind am Master KEINE Code-Aenderungen noetig (siehe rs485_slave.h).

static const uint8_t PWM_PINS[FAN_COUNT]   = {PWM1_PIN, PWM2_PIN, PWM3_PIN, PWM4_PIN};
static const uint8_t TACHO_PINS[FAN_COUNT] = {TACHO1_PIN, TACHO2_PIN, TACHO3_PIN, TACHO4_PIN};

static Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static volatile uint32_t tachoPulses[FAN_COUNT] = {0, 0, 0, 0};
static uint8_t  fanPercent[FAN_COUNT] = {0, 0, 0, 0};
static uint32_t lastWriteMs = 0;

static bool scd41Present  = false;
static bool as7341Present = false;
static bool gp8403Present = false;

static void tacho0_isr() { tachoPulses[0]++; }
static void tacho1_isr() { tachoPulses[1]++; }
static void tacho2_isr() { tachoPulses[2]++; }
static void tacho3_isr() { tachoPulses[3]++; }
static void (*const TACHO_ISR[FAN_COUNT])() = { tacho0_isr, tacho1_isr, tacho2_isr, tacho3_isr };

static void setFanPercent(uint8_t ch, uint8_t pct) {
    if (ch >= FAN_COUNT) return;
    if (pct > 100) pct = 100;
    fanPercent[ch] = pct;
    analogWrite(PWM_PINS[ch], (int)roundf(pct / 100.0f * 255));
}

// Master schreibt PWM-Sollwert (Reg 0-3) im Luefter-Register-Satz (immer aktiv).
static void onLueftWrite(uint16_t reg, uint16_t value) {
    if (reg < FAN_COUNT) {
        uint8_t pct = value > 100 ? 100 : (uint8_t)value;
        setFanPercent(reg, pct);
        rs485_slave_update_lueft(reg, pct);   // Sollwert fuer FC03-Rueckmeldung spiegeln
        lastWriteMs = millis();
        Serial.printf("[LUEFT] Kanal %u -> %u%%\n", reg + 1, pct);
    }
}

// Master schreibt Kanal 0/1 (Reg 0-1) im Analog-Register-Satz -> GP8403. Nur registriert,
// wenn beim Boot ein GP8403 gefunden wurde (siehe setup()).
static void onAnalogWrite(uint16_t reg, uint16_t value) {
    if (reg < 2) {
        gp8403_set_channel((uint8_t)reg, value);
        rs485_slave_update_analog(reg, value);
        lastWriteMs = millis();
        Serial.printf("[ANALOG] Kanal %u -> %u\n", reg + 1, value);
    }
}

static bool i2cProbe(uint8_t addr) {
    Wire1.beginTransmission(addr);
    return Wire1.endTransmission() == 0;
}

// Ueberträgt einen Sensor-Messzyklus in den MS2-Register-Satz (0x20) -- Skalierung und
// Layout sind identisch zum Multisensor, damit die Parse-Logik am Master unveraendert passt.
static void updateMs2Regs(const sensor_data_t& d, bool scd41Ok, bool as7341Ok) {
    rs485_slave_update_ms2(RS485_MS2_REG_CO2, d.co2_ppm);
    int16_t t = d.temperature_cdeg;
    rs485_slave_update_ms2(RS485_MS2_REG_TEMP, (uint16_t)((t < 0 ? t - 5 : t + 5) / 10));
    rs485_slave_update_ms2(RS485_MS2_REG_HUMI, (d.humidity_cpct + 5) / 10);
    rs485_slave_update_ms2(RS485_MS2_REG_PPFD, d.ppfd_dppfd);
    uint16_t ch[8] = {d.ch_415nm, d.ch_445nm, d.ch_480nm, d.ch_515nm,
                       d.ch_555nm, d.ch_590nm, d.ch_630nm, d.ch_680nm};
    for (int i = 0; i < 8; i++) rs485_slave_update_ms2(RS485_MS2_REG_CH0 + i, (uint8_t)ch[i]);
    rs485_slave_update_ms2(RS485_MS2_REG_GAIN, d.as7341_gain);
    rs485_slave_update_ms2(RS485_MS2_REG_STATUS, (scd41Ok ? 0x01 : 0) | (as7341Ok ? 0x02 : 0));
}

void setup() {
    Serial.begin(115200);
    // RP2040 hat keine eigene ROM-Bootmeldung wie der ESP32 — das native USB-CDC braucht
    // nach dem Boot etwas Zeit, bis der PC-Treiber wirklich verbunden ist. Hoechstens 3s
    // warten, damit das Modul auch ohne angeschlossenes Terminal normal weiterlaeuft.
    uint32_t waitStart = millis();
    while (!Serial && millis() - waitStart < 3000) delay(10);
    Serial.println("\n\nMultimodul (RP2040-Zero): RS485-Luefter (0x51) + optional MS2 (0x20) / Analog (0x50)");

    led.begin();
    led.setBrightness(40);
    led.setPixelColor(0, led.Color(0, 0, 30));  // dim blau = bereit/idle
    led.show();

    // PWM: 25 kHz fuer alle Kanaele (PC-Luefter-Standard), 8-bit Aufloesung (0-255)
    analogWriteFreq(25000);
    analogWriteRange(255);
    for (uint8_t i = 0; i < FAN_COUNT; i++) {
        analogWrite(PWM_PINS[i], 0);                       // Luefter aus
        pinMode(TACHO_PINS[i], INPUT_PULLUP);              // Open-Collector -> Pull-up 3,3V
        attachInterrupt(digitalPinToInterrupt(TACHO_PINS[i]), TACHO_ISR[i], FALLING);
    }

    // I2C1 (SDA=10/SCL=11) -- optionale Peripherie per Scan erkennen, BEVOR ueberhaupt
    // versucht wird sie zu initialisieren (vermeidet I2C-Timeouts durch fehlende Geraete).
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);
    Wire1.begin();
    Wire1.setClock(50000);
    delay(50);

    scd41Present  = i2cProbe(I2C_ADDR_SCD41);
    as7341Present = i2cProbe(I2C_ADDR_AS7341);
    gp8403Present = i2cProbe(I2C_ADDR_GP8403);
    Serial.printf("[I2C] SCD41=%s AS7341=%s GP8403=%s\n",
                  scd41Present  ? "gefunden" : "fehlt",
                  as7341Present ? "gefunden" : "fehlt",
                  gp8403Present ? "gefunden" : "fehlt");

    if (scd41Present) {
        sensor_manager_init(I2C_SDA, I2C_SCL);
    }
    if (gp8403Present) {
        gp8403_init();
        rs485_slave_set_analog_write_cb(onAnalogWrite);
    }
    rs485_slave_set_ms2_active(scd41Present);
    rs485_slave_set_analog_active(gp8403Present);

    rs485_slave_set_lueft_write_cb(onLueftWrite);
    rs485_slave_init(RS485_TX, RS485_RX);
    Serial.printf("[RS485] Adressen: 0x51 (Luefter, immer) 0x20 (MS2, %s) 0x50 (Analog, %s)\n",
                  scd41Present ? "aktiv" : "inaktiv", gp8403Present ? "aktiv" : "inaktiv");
}

void loop() {
    rs485_slave_loop();

    uint32_t now = millis();

    // Drehzahl alle 1 s aus den gezaehlten Tacho-Pulsen berechnen.
    //   RPM = (Pulse / Pulse_pro_Umdrehung) / (dt_ms / 60000)
    static uint32_t lastRpm = 0;
    if (now - lastRpm >= 1000) {
        uint32_t dt = now - lastRpm;
        lastRpm = now;
        uint16_t rpm[FAN_COUNT];
        for (uint8_t i = 0; i < FAN_COUNT; i++) {
            noInterrupts();
            uint32_t p = tachoPulses[i];
            tachoPulses[i] = 0;
            interrupts();
            rpm[i] = (uint16_t)((uint64_t)p * 60000UL / (TACHO_PULSES_PER_REV * dt));
            rs485_slave_update_lueft(4 + i, rpm[i]);
        }
        Serial.printf("[STATUS] 1:%u%%/%uU  2:%u%%/%uU  3:%u%%/%uU  4:%u%%/%uU\n",
                      fanPercent[0], rpm[0], fanPercent[1], rpm[1],
                      fanPercent[2], rpm[2], fanPercent[3], rpm[3]);
    }

    if (scd41Present) {
        sensor_manager_loop();
        if (sensor_manager_data_ready()) {
            sensor_data_t d;
            sensor_manager_get_data(&d);
            updateMs2Regs(d, !scd41_has_error(), sensor_manager_as7341_ok());
        }
    }

    // Status-LED: kurz gruen nach RS485-Schreibaktivitaet, sonst dim blau.
    static bool actLit = false;
    bool active = (lastWriteMs != 0) && (now - lastWriteMs < 150);
    if (active != actLit) {
        led.setPixelColor(0, active ? led.Color(0, 60, 0) : led.Color(0, 0, 30));
        led.show();
        actLit = active;
    }
}
