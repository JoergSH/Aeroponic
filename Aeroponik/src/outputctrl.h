#ifndef OUTPUTCTRL_H
#define OUTPUTCTRL_H

#include <stdint.h>

// Welche Hardware Luefter bzw. Licht tatsaechlich ansteuert. Wird als Setup-Entscheidung
// in der Konfiguration getroffen (nicht zur Laufzeit umgeschaltet) — steuert sowohl,
// welches Geraet Schreibbefehle bekommt (main.cpp), als auch welches RS485-Geraet
// ueberhaupt gepollt wird (rs485.cpp), siehe rs485_set_licht_polling() etc.
#define FAN_OUTPUT_MARS       0   // MARS Hydro Luefter, RS485-Adresse 6
#define FAN_OUTPUT_ANALOG     1   // Analog-Modul Kanal 1 (0-10V)
#define LIGHT_OUTPUT_LICHTX4  0   // Lichtx4, RS485-Adresse 0x40 (4x Relais-Stufen)
#define LIGHT_OUTPUT_ANALOG   1   // Analog-Modul Kanal 2 (0-10V, stufenlos)

struct OutputConfig {
    uint8_t fan_output;    // FAN_OUTPUT_*
    uint8_t light_output;  // LIGHT_OUTPUT_*
};

extern OutputConfig outputConfig;

void loadOutputConfig();
void saveOutputConfig();

#endif
