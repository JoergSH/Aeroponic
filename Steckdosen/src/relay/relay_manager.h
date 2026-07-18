#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define RELAY_COUNT  4

// Initialisiert GPIO-Pins und stellt gespeicherten Zustand wieder her.
void relay_manager_init();

// Setzt alle 4 Relais per Bitmaske (Bit 0 = Relais 1, ..., Bit 3 = Relais 4).
void relay_manager_set_mask(uint8_t mask);

// Gibt aktuelle Bitmaske zurück.
uint8_t relay_manager_get_mask();

#endif
