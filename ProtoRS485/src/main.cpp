#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pinout.h"
#include "rs485_slave.h"

// ============================================================================
// Generischer RS485-Modbus-Slave-Simulator mit WS2812-LED-Anzeige
// ============================================================================
// Simuliert hier konkret das ATO RS485-Analog-Ausgangsmodul (Adresse 0x50, 2 Register
// fuer 0-10V-Kanaele) auf einem WS2812-Streifen: Kanal 1 (Luefter) und Kanal 2 (Licht)
// je als Helligkeit einer einzelnen LED-Adresse, damit der Master unveraendert gegen
// dieses Geraet getestet werden kann, bevor die echte Hardware da ist.
//
// Fuer eigene Anwendungen anpassen (anderes Modbus-RTU-Slave-Geraet simulieren):
//   - rs485_slave.h:   RS485_SLAVE_ADDR (Modbus-Adresse des simulierten Geraets),
//                      RS485_REG_* / RS485_NUM_REGS (welche Register es gibt und was
//                      sie bedeuten).
//   - pinout.h:        RS485_TX/RS485_RX (UART-Pins zum RS485-Transceiver),
//                      LED_PIN/LED_COUNT (Streifenlaenge). Achtung bei 12V-Streifen:
//                      oft nur 1 ansteuerbare Adresse pro 3 physischen LEDs in Serie
//                      (siehe Kommentar dort) — LED_COUNT ist dann kleiner als die
//                      Anzahl sichtbarer LEDs!
//   - main.cpp:        onWrite()/redraw() unten — wie ankommende Registerwerte
//                      visualisiert werden (hier: Helligkeit einer LED pro Kanal,
//                      liesse sich z.B. auch als Bargraph oder Textausgabe umsetzen).
//   - rs485_slave.cpp: generischer Modbus-RTU-Slave-Kern (CRC16, FC03 Read Holding
//                      Registers, FC06 Write Single Register, Exception-Handling) —
//                      i.d.R. unveraendert wiederverwendbar, unabhaengig vom
//                      simulierten Geraet.
// ============================================================================

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_BGR + NEO_KHZ800);

static uint16_t ch1Value = 0;  // 0-4095, Luefter
static uint16_t ch2Value = 0;  // 0-4095, Licht

static void setChannelLed(uint8_t idx, uint16_t value, uint8_t r, uint8_t g, uint8_t b) {
    float frac = value / 4095.0f;
    strip.setPixelColor(idx, strip.Color((uint8_t)(r * frac), (uint8_t)(g * frac), (uint8_t)(b * frac)));
}

static void redraw() {
    setChannelLed(LED_FAN,   ch1Value, 0, 120, 255);   // Luefter: Blau
    setChannelLed(LED_LIGHT, ch2Value, 255, 200, 0);   // Licht: Gelb
    strip.show();
}

static void onWrite(uint16_t reg, uint16_t value) {
    if (value > 4095) value = 4095;
    if (reg == RS485_REG_CH1)      ch1Value = value;
    else if (reg == RS485_REG_CH2) ch2Value = value;
    rs485_slave_update(reg, value);
    redraw();
    Serial.printf("[SIM] Ch1(Luefter)=%4u (%5.2fV)   Ch2(Licht)=%4u (%5.2fV)\n",
                  ch1Value, ch1Value / 4095.0f * 10.0f,
                  ch2Value, ch2Value / 4095.0f * 10.0f);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nRS485-Analogmodul-Simulator (ATO 2-Kanal 0-10V, Adresse 0x50)");

    strip.begin();
    strip.setBrightness(60);
    strip.show();  // alle LEDs aus

    rs485_slave_set_write_cb(onWrite);
    rs485_slave_init(RS485_TX, RS485_RX);
}

void loop() {
    rs485_slave_loop();
}
