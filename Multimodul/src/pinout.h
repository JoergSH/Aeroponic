#ifndef PINOUT_H
#define PINOUT_H

// ============================================================================
// RP2040-Zero — Multimodul: RS485-Luefteransteuerung + optionale I2C-Peripherie
//   4x PWM-Ausgang (25 kHz) fuer 4-Pin-PC-Luefter + 4x Tacho-Eingang (wie Lueftermodul).
//   Zusaetzlich I2C1 (SDA=10/SCL=11): optional SCD41 (CO2), AS7341 (Licht/PPFD) und
//   GP8403 (2-Kanal 0-10V-DAC) -- werden beim Boot per I2C-Scan erkannt; fehlt ein
//   Geraet, wird es danach nicht mehr abgefragt (siehe main.cpp).
// ============================================================================

// RS485 (UART0) — GP28 = TX -> DI, GP29 = RX <- RO des Transceivers
#define RS485_TX   28
#define RS485_RX   29

// 4x PWM-Ausgang (25 kHz) + 4x Tacho-Eingang, identisch zum Lueftermodul (siehe dort fuer
// die Begruendung der Pin-Reihenfolge: PWM/Tacho eines Luefters liegen nebeneinander).
#define PWM1_PIN   0
#define TACHO1_PIN 1
#define PWM2_PIN   2
#define TACHO2_PIN 3
#define PWM3_PIN   4
#define TACHO3_PIN 5
#define PWM4_PIN   6
#define TACHO4_PIN 7

// I2C1 fuer optionale Sensoren/Ausgangsmodul (GP10=SDA, GP11=SCL -> beide I2C1, gueltiges
// Paar: I2C-Instanz = gpio % 4, hier beide = 2/3 -> I2C1).
#define I2C_SDA    10
#define I2C_SCL    11

// I2C-Adressen der optionalen Peripherie (Scan beim Boot, siehe main.cpp)
#define I2C_ADDR_SCD41   0x62
#define I2C_ADDR_AS7341  0x39
#define I2C_ADDR_GP8403  0x58

// Onboard WS2812 RGB-Status-LED
#define LED_PIN    16

#define FAN_COUNT  4
#define TACHO_PULSES_PER_REV 2

#endif
