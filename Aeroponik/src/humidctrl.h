#ifndef HUMIDCTRL_H
#define HUMIDCTRL_H

#include <stdint.h>
#include <stdbool.h>

// Luftbefeuchtung — 1:1 Kopie der CO2-Steuerung (co2ctrl.h), nur mit Luftfeuchte statt CO2
// als Regelgroesse. Gleiche Hysterese-Logik: Feuchte <= Min -> Ausgang EIN (befeuchten),
// Feuchte >= Max -> Ausgang AUS, dazwischen bleibt der Zustand erhalten.
struct HumidConfig {
    uint16_t humid_min;         // %*10 — darunter: Ausgang EIN
    uint16_t humid_max;         // %*10 — darüber: Ausgang AUS
    bool     enabled;
    uint8_t  target_node_id;    // ESP-NOW Steckdosen-Node (0 = keiner ausgewählt)
    uint8_t  target_relay_bit;  // 0-3, Bit-Position in der Relaismaske des Ziel-Nodes
};

extern HumidConfig humidConfig;

void loadHumidConfig();
void saveHumidConfig();

#endif
