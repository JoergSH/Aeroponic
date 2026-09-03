#ifndef DS18B20CFG_H
#define DS18B20CFG_H

#include <stdint.h>
#include <stdbool.h>

// Ordnet DS18B20-Sensoren am gemeinsamen 1-Wire-Bus (DS18B20_PIN) über ihre eindeutige
// 64-bit-ROM-Adresse einer Rolle zu, statt sich auf die Bus-Scan-Reihenfolge zu verlassen
// (die sich bei vertauschten oder unterschiedlich langen Kabeln ändert). Solange eine
// Rolle nicht zugewiesen ist, fällt main.cpp auf die alte Index-Reihenfolge zurück
// (0=Vorrat, 1=Pflanze, 2=Innentemperatur), damit Bestandsinstallationen ohne Zutun
// weiterlaufen.
struct Ds18b20Config {
    bool    vorrat_set;
    uint8_t vorrat_addr[8];
    bool    pflanze_set;
    uint8_t pflanze_addr[8];
    bool    innen_set;      // Innentemperatur Steuerung — optional
    uint8_t innen_addr[8];
};

extern Ds18b20Config ds18b20Config;

void loadDs18b20Config();
void saveDs18b20Config();

#endif
