#ifndef RS485LOG_H
#define RS485LOG_H

#include <Arduino.h>

// Dauer-Mitschnitt des RS485-Busses auf die SD-Karte (eine Datei pro Tag, /rs485_JJJJ-MM-TT.txt,
// abrufbar ueber die normale Log-Download-UI). Ein-/ausschaltbar im Interface, Zustand bleibt
// ueber Neustarts erhalten. Jede Zeile: Uhrzeit, Richtung, Rohbytes in Hex, ggf. Zusatzinfo.
//   TX  = vom Master gesendet
//   RX  = Antwort auf die letzte Anfrage (mit Antwortzeit)
//   TO  = keine (vollstaendige) Antwort bis zum Timeout, ggf. mit den Teil-Bytes
//   RX? = Bytes, die ohne laufende Anfrage ankamen (Stoerungen, fremder Sender, Slave-Reset)
// Gepuffert im RAM und alle paar Sekunden geschrieben -- bei einem Absturz fehlen hoechstens
// die letzten Sekunden. Waehrend eines Slave-Firmware-Updates wird nicht mitgeschnitten.

void rs485log_init();                    // Zustand aus Preferences laden (nach storage_init)
void rs485log_set_enabled(bool on);      // speichert den Zustand dauerhaft
bool rs485log_enabled();
String rs485log_current_file();          // aktueller Dateiname ohne "/" (leer wenn aus)

// Uhrzeit fuer die Zeitstempel: aus loop() mit einer plausiblen RTC-Zeit (Lokalzeit als
// unixtime, wie storage_log) aufrufen -- dazwischen wird per millis() weitergezaehlt.
void rs485log_set_time(uint32_t localUnix);

void rs485log_frame(const char* dir, const uint8_t* buf, uint16_t len, const char* info = nullptr);
void rs485log_note(const char* text);

void rs485log_loop();                    // Puffer periodisch auf die SD schreiben

#endif
