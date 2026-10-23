#ifndef RAUMLICHTCTRL_H
#define RAUMLICHTCTRL_H

#include <stdint.h>

// Raumlicht: rein manueller An/Aus-Schalter fuer einen Ausgang eines ESP-NOW-Steckdosen-Nodes
// (keine Automatik/Hysterese/Zeitplan wie bei CO2/Luftbefeuchtung/DWC) -- der tatsaechliche
// Ein/Aus-Zustand wird nicht separat gespeichert, sondern live aus nodeData[].relay_mask des
// Ziel-Nodes gelesen (siehe main.cpp), damit UI und echter Relaiszustand nie auseinanderlaufen.
struct RaumlichtConfig {
    uint8_t target_node_id;    // ESP-NOW Steckdosen-Node (0 = keiner ausgewählt -> Karte auf Startseite versteckt)
    uint8_t target_relay_bit;  // 0-3, Bit-Position in der Relaismaske des Ziel-Nodes
};

extern RaumlichtConfig raumlichtConfig;

void loadRaumlichtConfig();
void saveRaumlichtConfig();

#endif
