#include "dbg.h"
#include <SD.h>
#include <stdarg.h>

static File     captureFile;
static bool     captureActive = false;
static uint32_t captureEndMs  = 0;

static TaskHandle_t captureTask = nullptr;   // Task, der die Aufzeichnung gestartet hat (loop())

// Nur aus dem Task schreiben, der die Datei geoeffnet hat -- der Benachrichtigungs-Task (siehe
// notify.cpp) loggt ebenfalls, gleichzeitige SD-Zugriffe aus zwei Tasks waeren nicht sicher.
static void writeToCapture(const char* s) {
    if (captureActive && captureFile && xTaskGetCurrentTaskHandle() == captureTask) captureFile.print(s);
}

void dbgPrint(const String& s) { Serial.print(s); writeToCapture(s.c_str()); }
void dbgPrint(const char* s)   { Serial.print(s); writeToCapture(s); }

void dbgPrintln(const String& s) { Serial.println(s); writeToCapture(s.c_str()); writeToCapture("\n"); }
void dbgPrintln(const char* s)   { Serial.println(s); writeToCapture(s);         writeToCapture("\n"); }
void dbgPrintln()                { Serial.println();  writeToCapture("\n"); }

void dbgPrintf(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
    writeToCapture(buf);
}

void dbgStartCapture(uint32_t durationMs) {
    if (captureActive && captureFile) captureFile.close();
    char filename[32];
    snprintf(filename, sizeof(filename), "/debug_%lu.txt", (unsigned long)millis());
    captureFile = SD.open(filename, FILE_WRITE);
    if (!captureFile) {
        Serial.printf("[DBG] Konnte %s nicht oeffnen\n", filename);
        captureActive = false;
        return;
    }
    captureActive = true;
    captureTask   = xTaskGetCurrentTaskHandle();
    captureEndMs  = millis() + durationMs;
    Serial.printf("[DBG] Aufzeichnung gestartet: %s (%lus)\n", filename, durationMs / 1000UL);
}

bool dbgCaptureActive() { return captureActive; }

void dbgLoop() {
    if (captureActive && millis() >= captureEndMs) {
        captureFile.close();
        captureActive = false;
        Serial.println("[DBG] Aufzeichnung beendet");
    }
}
