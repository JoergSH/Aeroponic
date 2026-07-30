#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>

// Simuliert das ATO RS485-zu-0-10V-Analogmodul (siehe Aeroponik-Doku "Analog-Ausgangsmodul"):
// gleiche Adresse/Register wie das echte Geraet, damit der Master unveraendert dagegen
// getestet werden kann.
//
// Fuer ein anderes Geraet simulieren: hier nur Adresse/Register-Layout an das
// Zielgeraet anpassen (Modbus-Adresse laut dessen Handbuch, Register-Anzahl/-Bedeutung
// je nach FC03/FC06-Register-Map) — der Rest von rs485_slave.cpp bleibt unveraendert,
// solange es sich um Standard-Modbus-RTU mit Read Holding Registers (FC03) und
// Write Single Register (FC06) handelt.
#define RS485_SLAVE_ADDR   0x50  // Analog-Ausgangsmodul (Simulator)

#define RS485_REG_CH1      0     // Kanal 1 (Luefter), 0-4095 = 0-10V
#define RS485_REG_CH2      1     // Kanal 2 (Licht),   0-4095 = 0-10V
#define RS485_NUM_REGS     2

// Aufgerufen, wenn der Master per FC06 ein Register beschreibt.
typedef void (*rs485_slave_write_cb_t)(uint16_t reg, uint16_t value);

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();
void rs485_slave_set_write_cb(rs485_slave_write_cb_t cb);

// Spiegelt den Ist-Zustand eines Registers, damit ein FC03-Poll ihn liefert.
void rs485_slave_update(uint16_t reg, uint16_t value);

bool rs485_slave_got_request();

#endif
