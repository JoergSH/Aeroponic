#ifndef DBG_H
#define DBG_H

#include <Arduino.h>

// Ersetzt Serial.print/println/printf 1:1 im ganzen Projekt. Verhaelt sich normalerweise
// exakt wie Serial.*, spiegelt die Ausgabe aber zusaetzlich in eine Datei auf der SD-Karte,
// waehrend eine Aufzeichnung aktiv ist (siehe dbgStartCapture()) — damit man Fehler auch
// dann nachvollziehen kann, wenn das Board schon fest verbaut ist und kein serieller
// Monitor griffbereit ist. Die Datei landet im selben Ordner/Format wie die Datenlogs und
// ist ueber die bestehende Log-Download-UI abrufbar.
void dbgPrint(const String& s);
void dbgPrint(const char* s);
void dbgPrintln(const String& s);
void dbgPrintln(const char* s);
void dbgPrintln();
void dbgPrintf(const char* fmt, ...);

// Startet eine Aufzeichnung fuer durationMs Millisekunden in eine neue Datei auf der
// SD-Karte. Bricht eine evtl. laufende Aufzeichnung vorher sauber ab.
void dbgStartCapture(uint32_t durationMs);
bool dbgCaptureActive();

// Aus loop() aufrufen, damit eine laufende Aufzeichnung nach Ablauf der Zeit sauber
// beendet (Datei geschlossen) wird.
void dbgLoop();

#endif
