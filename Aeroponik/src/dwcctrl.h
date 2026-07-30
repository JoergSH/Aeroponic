#ifndef DWCCTRL_H
#define DWCCTRL_H

#include <stdint.h>
#include <stdbool.h>

// Einfacher Ein/Aus-Beleuchtungstimer fuer ein separates DWC-System, geschaltet ueber
// einen Ausgang eines ESP-NOW-Steckdosen-Nodes (kein Dawn/Dusk-Ramp wie beim
// Haupt-Lichtscheduler — bewusst simpel, wie bei einer haushaltsueblichen Zeitschaltuhr).
struct DwcTimerConfig {
    bool     enabled;
    uint8_t  target_node_id;    // ESP-NOW Steckdosen-Node (0 = keiner ausgewählt)
    uint8_t  target_relay_bit;  // 0-3, Bit-Position in der Relaismaske des Ziel-Nodes
    uint16_t on_min;            // Minuten seit Mitternacht, Licht AN
    uint16_t off_min;           // Minuten seit Mitternacht, Licht AUS (< on_min = über Mitternacht)
};

extern DwcTimerConfig dwcConfig;

void loadDwcConfig();
void saveDwcConfig();

#endif
