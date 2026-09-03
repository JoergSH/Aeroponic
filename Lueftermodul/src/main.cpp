#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pinout.h"
#include "rs485_slave.h"

// RS485-Lueftermodul auf RP2040-Zero: 4 PWM-Ausgaenge (25 kHz) fuer PC-Luefter,
// 4 Tacho-Eingaenge (Drehzahl per Flanken-Interrupt), Modbus-RTU-Slave (Adresse 0x51).
//   Register 0-3 (FC06 schreiben): PWM-Sollwert Luefter 1-4 in Prozent (0-100)
//   Register 4-7 (FC03 lesen):     Tacho-Drehzahl Luefter 1-4 in U/min

static const uint8_t PWM_PINS[FAN_COUNT]   = {PWM1_PIN, PWM2_PIN, PWM3_PIN, PWM4_PIN};
static const uint8_t TACHO_PINS[FAN_COUNT] = {TACHO1_PIN, TACHO2_PIN, TACHO3_PIN, TACHO4_PIN};

static Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static volatile uint32_t tachoPulses[FAN_COUNT] = {0, 0, 0, 0};
static uint8_t  fanPercent[FAN_COUNT] = {0, 0, 0, 0};
static uint32_t lastWriteMs = 0;

// Eine kleine ISR pro Kanal — zaehlt nur die Tacho-Flanken.
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

// Master schreibt ein Register (FC06): Reg 0-3 = PWM-Sollwert, Reg 4-7 (Tacho) read-only.
static void onWrite(uint16_t reg, uint16_t value) {
    if (reg < FAN_COUNT) {
        uint8_t pct = value > 100 ? 100 : (uint8_t)value;
        setFanPercent(reg, pct);
        rs485_slave_update(reg, pct);   // Sollwert fuer FC03-Rueckmeldung spiegeln
        lastWriteMs = millis();
        Serial.printf("[LUEFT] Kanal %u -> %u%%\n", reg + 1, pct);
    }
}

void setup() {
    Serial.begin(115200);
    // RP2040 hat keine eigene ROM-Bootmeldung wie der ESP32 — das native USB-CDC braucht
    // nach dem Boot etwas Zeit, bis der PC-Treiber wirklich verbunden ist. Alles, was vorher
    // gesendet wird, geht ungepuffert verloren. Hoechstens 3s warten, damit das Modul auch
    // ohne angeschlossenes Terminal normal weiterlaeuft.
    uint32_t waitStart = millis();
    while (!Serial && millis() - waitStart < 3000) delay(10);
    Serial.println("\n\nRS485-Lueftermodul (RP2040-Zero, Adresse 0x51)");

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

    rs485_slave_set_write_cb(onWrite);
    rs485_slave_init(RS485_TX, RS485_RX);
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
            rs485_slave_update(4 + i, rpm[i]);
        }
        // Statusausgabe fuers eigenstaendige Testen, auch ganz ohne RS485-Traffic vom Master.
        Serial.printf("[STATUS] 1:%u%%/%uU  2:%u%%/%uU  3:%u%%/%uU  4:%u%%/%uU\n",
                      fanPercent[0], rpm[0], fanPercent[1], rpm[1],
                      fanPercent[2], rpm[2], fanPercent[3], rpm[3]);
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
