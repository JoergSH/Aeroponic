#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>

// Adress-Raum: 0x01-0x0E = Mars Hydro Original, 0x20-0x3F = eigene Sensoren,
// 0x40-0x5F = eigene Aktoren
#define RS485_SLAVE_ADDR   0x40  // Lichtx4

#define RS485_REG_MASK     0     // Relaismaske, Bit0-3 = Relais 1-4
#define RS485_NUM_REGS     1

// Aufgerufen, wenn der Master per FC06 ein Register beschreibt.
typedef void (*rs485_slave_write_cb_t)(uint16_t reg, uint16_t value);

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();
void rs485_slave_set_write_cb(rs485_slave_write_cb_t cb);

// Spiegelt den Ist-Zustand in RS485_REG_MASK, damit ein FC03-Poll ihn liefert.
void rs485_slave_update(uint8_t mask);

bool rs485_slave_got_request();

#endif
