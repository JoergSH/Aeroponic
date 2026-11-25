#ifndef FLOWSENSOR_H
#define FLOWSENSOR_H

#include <stdint.h>

// Duenne Zugriffsschicht auf den Durchfluss-Impulszaehler: Interrupt + Variable leben weiter
// in main.cpp (siehe flowPulseCount/FLOW_SENSOR_PIN).

// Gibt die seit dem letzten Aufruf gezaehlten Impulse zurueck und setzt den Zaehler auf 0
// zurueck (atomar, siehe Implementierung in main.cpp).
uint32_t flowsensor_take_pulses();

// Gesamtzahl aller Impulse seit dem Start (wird nie zurueckgesetzt) -- fuer die Diagnose-Spur
// in flowmonitor.cpp, die unabhaengig vom Messfenster zeigt, wann wirklich Impulse kommen.
uint32_t flowsensor_total_pulses();

#endif
