#ifndef CO2CTRL_H
#define CO2CTRL_H

#include <stdint.h>
#include <stdbool.h>

struct Co2Config {
    uint16_t co2_min;           // ppm — darunter: Ausgang EIN
    uint16_t co2_max;           // ppm — darüber: Ausgang AUS
    bool     enabled;
    uint8_t  target_node_id;    // ESP-NOW Steckdosen-Node (0 = keiner ausgewählt)
    uint8_t  target_relay_bit;  // 0-3, Bit-Position in der Relaismaske des Ziel-Nodes
};

extern Co2Config co2Config;

void loadCo2Config();
void saveCo2Config();

#endif
