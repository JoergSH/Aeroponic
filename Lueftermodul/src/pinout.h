#ifndef PINOUT_H
#define PINOUT_H

// ============================================================================
// RP2040-Zero — RS485-Lueftermodul
//   4x PWM-Ausgang (25 kHz) fuer 4-Pin-PC-Luefter + 4x Tacho-Eingang.
//   RS485 ueber UART0 (MAX13487 Auto-Direction, kein DE/RE noetig).
// ============================================================================

// RS485 (UART0) — GP28 = TX -> DI, GP29 = RX <- RO des Transceivers
#define RS485_TX   28
#define RS485_RX   29

// 4x PWM-Ausgang (25 kHz) an den PWM-Eingang der Luefter + 4x Tacho-Eingang, PWM/Tacho
// pro Luefter jeweils auf benachbarten GPIO (vereinfacht das Platinenlayout: beide Signale
// eines Luefter-Steckers liegen nebeneinander). Jeder PWM-Pin landet dabei auf Kanal A einer
// eigenen PWM-Slice (Slice = GPIO/2 % 8) — Kanal B bleibt ungenutzt, kein Konflikt. Die
// Tacho-Pins sind reine GPIO-Interrupt-Eingaenge, deren Position ist unabhaengig von der
// PWM-Slice-Zuordnung.
#define PWM1_PIN   0
#define TACHO1_PIN 1
#define PWM2_PIN   2
#define TACHO2_PIN 3
#define PWM3_PIN   4
#define TACHO3_PIN 5
#define PWM4_PIN   6
#define TACHO4_PIN 7

// Onboard WS2812 RGB-Status-LED
#define LED_PIN    16

#define FAN_COUNT  4

// Tacho-Pulse pro Umdrehung (Standard-PC-Luefter: 2)
#define TACHO_PULSES_PER_REV 2

#endif
