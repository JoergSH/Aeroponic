#ifndef PINOUT_H
#define PINOUT_H

// nologo_esp32c3_super_mini — gleiche RS485-Pinbelegung wie das Multisensor-Projekt
// (identisches Board), damit Bus-Verkabelung/Erfahrung uebertragbar bleibt.
#define RS485_TX   3
#define RS485_RX   5

// 12V-Streifen: 15 physische LEDs, aber nur 5 ansteuerbare Adressen (je 3 LEDs in
// Serie pro Controller-Chip) — LED_COUNT ist daher 5, nicht 15. Genutzt werden nur
// zwei davon, je ein Kanal ueber die Helligkeit einer einzelnen Adresse dargestellt
// (0-100% = 0-volle Helligkeit), die restlichen Adressen bleiben aus.
// Datenleitung des Streifens hier anschliessen.
#define LED_PIN    10
#define LED_COUNT  5
#define LED_FAN    0   // Kanal 1 (Luefter), blau
#define LED_LIGHT  1   // Kanal 2 (Licht), gelb

#endif
