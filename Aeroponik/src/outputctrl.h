#ifndef OUTPUTCTRL_H
#define OUTPUTCTRL_H

#include <stdint.h>

// Welche Hardware Luefter bzw. Licht tatsaechlich ansteuert. Wird als Setup-Entscheidung
// in der Konfiguration getroffen (nicht zur Laufzeit umgeschaltet) — steuert sowohl,
// welches Geraet Schreibbefehle bekommt (main.cpp), als auch welches RS485-Geraet
// ueberhaupt gepollt wird (rs485.cpp), siehe rs485_set_licht_polling() etc.
#define FAN_OUTPUT_MARS       0   // MARS Hydro Luefter, RS485-Adresse 6
#define FAN_OUTPUT_ANALOG     1   // RS485-Analog-Modul Kanal 1 (0-10V)
#define FAN_OUTPUT_GP8403     2   // GP8403 I2C-DAC Kanal 0 (0-10V)
#define LIGHT_OUTPUT_LICHTX4  0   // Lichtx4, RS485-Adresse 0x40 (4x Relais-Stufen)
#define LIGHT_OUTPUT_ANALOG   1   // RS485-Analog-Modul Kanal 2 (0-10V, stufenlos)
#define LIGHT_OUTPUT_GP8403   2   // GP8403 I2C-DAC Kanal 1 (0-10V, stufenlos)
// Kombi-Betrieb: Hauptlampe am 0-10V-Ausgang (stufenlos, bis zu ihrem Max-Wert) PLUS ausgewaehlte
// Lichtx4-Kanaele als Zusatzlicht -- die werden erst zugeschaltet, wenn die Hauptlampe morgens ihren
// Max-Wert erreicht hat (Ende Sonnenaufgang), und abends zu Beginn des Untergangs wieder
// abgeschaltet, bevor die Hauptlampe herunterdimmt. Siehe schedComputeKombi() in scheduler.cpp.
#define LIGHT_OUTPUT_KOMBI_ANALOG 3   // RS485-Analogmodul Kanal 2 + Lichtx4
#define LIGHT_OUTPUT_KOMBI_GP8403 4   // GP8403 Kanal 1 + Lichtx4
#define LIGHT_OUTPUT_MAX          4

struct OutputConfig {
    uint8_t fan_output;    // FAN_OUTPUT_*
    uint8_t light_output;  // LIGHT_OUTPUT_*
};

extern OutputConfig outputConfig;

// Was der gewaehlte Lichtausgang umfasst
static inline bool lichtIstKombi()     { return outputConfig.light_output >= LIGHT_OUTPUT_KOMBI_ANALOG; }
static inline bool lichtHatAnalog()    { return outputConfig.light_output != LIGHT_OUTPUT_LICHTX4; }
static inline bool lichtHatLichtx4()   { return outputConfig.light_output == LIGHT_OUTPUT_LICHTX4 || lichtIstKombi(); }
static inline bool lichtAnalogGp8403() { return outputConfig.light_output == LIGHT_OUTPUT_GP8403 ||
                                                outputConfig.light_output == LIGHT_OUTPUT_KOMBI_GP8403; }
static inline bool lichtAnalogModul()  { return outputConfig.light_output == LIGHT_OUTPUT_ANALOG ||
                                                outputConfig.light_output == LIGHT_OUTPUT_KOMBI_ANALOG; }

void loadOutputConfig();
void saveOutputConfig();

#endif
