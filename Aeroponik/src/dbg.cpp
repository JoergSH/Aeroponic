#include "dbg.h"
#include <SD.h>
#include <stdarg.h>

static File     captureFile;
static bool     captureActive = false;
static uint32_t captureEndMs  = 0;

static void writeToCapture(const char* s) {
    if (captureActive && captureFile) captureFile.print(s);
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
