#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>

// Generischer Modbus-RTU-Slave (RP2040, UART0) — gleiches Framework wie beim Analogmodul.
// Register-Map dieses Lueftermoduls:
//   0-3: PWM-Sollwert Luefter 1-4 in Prozent (0-100) — FC06 schreiben / FC03 lesen
//   4-7: Tacho-Drehzahl Luefter 1-4 in U/min — read-only (FC03), von der Firmware gesetzt
#define RS485_SLAVE_ADDR   0x51   // Bereich "Eigene Aktoren" 0x40-0x5F (Lichtx4=0x40, Analog=0x50)
#define RS485_NUM_REGS     8

// Aufgerufen, wenn der Master per FC06 ein Register beschreibt.
typedef void (*rs485_slave_write_cb_t)(uint16_t reg, uint16_t value);

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();
void rs485_slave_set_write_cb(rs485_slave_write_cb_t cb);

// Spiegelt den Ist-Zustand eines Registers, damit ein FC03-Poll ihn liefert.
void rs485_slave_update(uint16_t reg, uint16_t value);

bool rs485_slave_got_request();

#endif
