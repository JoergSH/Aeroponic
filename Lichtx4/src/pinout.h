#ifndef PINOUT_H
#define PINOUT_H

#include <stdint.h>

// ========== Relais (SSR-Ausgänge, HIGH-aktiv) ==========
#define RELAY_PINS_COUNT 4
static const uint8_t RELAY_PINS[RELAY_PINS_COUNT] = {1, 2, 3, 4};

// ========== RS485 (UART1) ==========
#define RS485_TX  6
#define RS485_RX  7

#endif
