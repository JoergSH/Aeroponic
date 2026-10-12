#ifndef FLOWSENSOR_H
#define FLOWSENSOR_H

#include <stdint.h>

// Duenne Zugriffsschicht auf den Durchfluss-Impulszaehler: Interrupt + Variable leben weiter
// in main.cpp (siehe flowPulseCount/FLOW_SENSOR_PIN), damit die bestehende Kalibrierroutine
// dort unveraendert bleibt. flowmonitor.cpp nutzt nur diese beiden Funktionen.

// Gibt die seit dem letzten Aufruf gezaehlten Impulse zurueck und setzt den Zaehler auf 0
// zurueck (atomar, siehe Implementierung in main.cpp).
uint32_t flowsensor_take_pulses();

// Aktuelle Kalibrierung (siehe Durchflussmesser-Kalibrierung im Web-UI), 0 = noch nicht
// kalibriert.
float flowsensor_pulses_per_liter();

#endif
