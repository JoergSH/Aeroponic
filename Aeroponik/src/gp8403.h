#ifndef GP8403_H
#define GP8403_H

#include <stdint.h>
#include <stdbool.h>

// GP8403: 2-Kanal I2C-DAC (12-bit) mit 0-10V-Ausgang, Alternative zum RS485-Analogmodul.
// Haengt am Haupt-I2C-Bus (Wire). Adresse via GP8403_ADDR (pinout.h). Reines Schreibgeraet
// (kein Ruecklesewert) — online() spiegelt nur, ob der letzte I2C-Zugriff bestaetigt wurde.

// Einmalig aufrufen (nach Wire.begin()): setzt beide Kanaele auf 0-10V-Bereich.
void gp8403_init();

// Setzt Kanal 0 oder 1 auf einen 12-bit-Wert (0-4095 = 0-10V). true = I2C-ACK erhalten.
bool gp8403_set_channel(uint8_t channel, uint16_t value_0_4095);

// Letzter I2C-Zugriff erfolgreich bestaetigt?
bool gp8403_online();

#endif
