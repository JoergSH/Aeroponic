#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>

// Adress-Raum: 0x01-0x0E = Mars Hydro Original, 0x20-0x3F = eigene Sensoren,
// 0x40-0x5F = eigene Aktoren
#define RS485_SLAVE_ADDR   0x40  // Lichtx4

#define RS485_REG_MASK     0     // Relaismaske, Bit0-3 = Relais 1-4
// Neustart-Diagnose (nur lesbar, ab FW 1.2.0): ein kurzer Reset des Moduls schaltet alle SSRs fuer
// ~1 s aus und stellt danach die gespeicherte Maske wieder her -- ohne diese Register sieht der
// Master davon nichts.
#define RS485_REG_BOOTS    1     // Anzahl Starts seit Erstinbetriebnahme (Preferences, laeuft ueber)
#define RS485_REG_RESET    2     // esp_reset_reason() des letzten Starts
#define RS485_REG_UPTIME   3     // Laufzeit seit Start in Minuten (laeuft nach ~45 Tagen ueber)
#define RS485_NUM_REGS     4

// Aufgerufen, wenn der Master per FC06 ein Register beschreibt.
typedef void (*rs485_slave_write_cb_t)(uint16_t reg, uint16_t value);

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();
void rs485_slave_set_write_cb(rs485_slave_write_cb_t cb);

// Spiegelt den Ist-Zustand in RS485_REG_MASK, damit ein FC03-Poll ihn liefert.
void rs485_slave_update(uint8_t mask);

// Setzt ein beliebiges (nur lesbares) Register, z.B. die Neustart-Diagnose.
void rs485_slave_set_reg(uint16_t reg, uint16_t value);

bool rs485_slave_got_request();

#endif
