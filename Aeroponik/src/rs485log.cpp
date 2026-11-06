#include "rs485log.h"
#include "storage.h"
#include "dbg.h"
#include <SD.h>
#include <Preferences.h>
#include <time.h>

#define RS485LOG_BUF_SIZE     8192
#define RS485LOG_FLUSH_AT     6144     // ab diesem Fuellstand sofort schreiben
#define RS485LOG_FLUSH_MS     5000UL   // sonst spaetestens nach dieser Zeit

static bool     enabled      = false;
static char     buf[RS485LOG_BUF_SIZE];
static uint16_t bufLen       = 0;
static uint32_t lastFlushMs  = 0;
static uint32_t lostLines    = 0;      // Puffer voll -> verworfen (sollte nie vorkommen)
static uint32_t timeBase     = 0;      // Lokalzeit als unixtime, 0 = noch unbekannt
static uint32_t timeBaseMs   = 0;
static char     fileName[28] = "";     // Datei, in die der aktuelle Pufferinhalt gehoert

static void currentTime(uint32_t& sec, uint16_t& ms) {
    uint32_t delta = millis() - timeBaseMs;
    sec = timeBase + delta / 1000;
    ms  = delta % 1000;
}

static void fileNameFor(uint32_t sec, char* out, size_t n) {
    if (timeBase == 0) { snprintf(out, n, "/rs485_ohne-uhrzeit.txt"); return; }
    time_t t = (time_t)sec;
    struct tm tmv;
    gmtime_r(&t, &tmv);   // Lokalzeit steckt schon in sec (siehe rs485log_set_time)
    snprintf(out, n, "/rs485_%04d-%02d-%02d.txt", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
}

static void flush() {
    if (bufLen == 0) return;
    if (!storage_is_ok() || fileName[0] == 0) { bufLen = 0; return; }
    File f = SD.open(fileName, FILE_APPEND);
    if (f) {
        f.write((const uint8_t*)buf, bufLen);
        f.close();
    } else {
        dbgPrintf("[RS485LOG] Oeffnen fehlgeschlagen: %s\n", fileName);
    }
    bufLen      = 0;
    lastFlushMs = millis();
}

// Haengt eine fertige Zeile an. Beim Tageswechsel wird vorher der alte Puffer in die alte
// Datei geschrieben, damit keine Zeilen in der falschen Tagesdatei landen.
static void appendLine(const char* line, uint16_t n) {
    uint32_t sec; uint16_t ms;
    currentTime(sec, ms);
    char fn[28];
    fileNameFor(sec, fn, sizeof(fn));
    if (strcmp(fn, fileName) != 0) {
        flush();
        strcpy(fileName, fn);
    }
    if (bufLen + n > RS485LOG_BUF_SIZE) flush();
    if (bufLen + n > RS485LOG_BUF_SIZE) { lostLines++; return; }
    memcpy(buf + bufLen, line, n);
    bufLen += n;
}

static int stampPrefix(char* out, size_t n) {
    uint32_t sec; uint16_t ms;
    currentTime(sec, ms);
    if (timeBase == 0)
        return snprintf(out, n, "%10lu.%03u ", (unsigned long)sec, ms);   // Sekunden seit Boot
    return snprintf(out, n, "%02lu:%02lu:%02lu.%03u ",
                    (unsigned long)((sec / 3600) % 24), (unsigned long)((sec / 60) % 60),
                    (unsigned long)(sec % 60), ms);
}

void rs485log_frame(const char* dir, const uint8_t* data, uint16_t len, const char* info) {
    if (!enabled) return;
    char line[3 * 256 + 96];
    int p = stampPrefix(line, sizeof(line));
    p += snprintf(line + p, sizeof(line) - p, "%-3s", dir);
    for (uint16_t i = 0; i < len && p < (int)sizeof(line) - 64; i++)
        p += snprintf(line + p, sizeof(line) - p, " %02X", data[i]);
    if (info && info[0]) p += snprintf(line + p, sizeof(line) - p, "  %s", info);
    p += snprintf(line + p, sizeof(line) - p, "\n");
    appendLine(line, (uint16_t)p);
}

void rs485log_note(const char* text) {
    if (!enabled) return;
    char line[160];
    int p = stampPrefix(line, sizeof(line));
    p += snprintf(line + p, sizeof(line) - p, "--  %s\n", text);
    appendLine(line, (uint16_t)p);
}

void rs485log_set_time(uint32_t localUnix) {
    if (localUnix == 0) return;
    timeBase   = localUnix;
    timeBaseMs = millis();
}

void rs485log_init() {
    Preferences p;
    p.begin("rs485log", true);
    enabled = p.getBool("en", false);
    p.end();
    lastFlushMs = millis();
    if (enabled) dbgPrintln("[RS485LOG] Mitschnitt aktiv");
}

void rs485log_set_enabled(bool on) {
    if (on == enabled) return;
    if (on) {
        enabled = true;
        rs485log_note("Mitschnitt eingeschaltet");
    } else {
        rs485log_note("Mitschnitt ausgeschaltet");
        flush();
        enabled = false;
    }
    Preferences p;
    p.begin("rs485log", false);
    p.putBool("en", on);
    p.end();
}

bool rs485log_enabled() { return enabled; }

String rs485log_current_file() {
    if (!enabled) return String();
    uint32_t sec; uint16_t ms;
    currentTime(sec, ms);
    char fn[28];
    fileNameFor(sec, fn, sizeof(fn));
    return String(fn + 1);
}

void rs485log_loop() {
    if (!enabled) return;
    static bool startNoted = false;
    if (!startNoted && timeBase != 0) {   // erste Zeile nach dem Boot, sobald die Uhrzeit da ist
        startNoted = true;
        rs485log_note("Master gestartet");
    }
    if (lostLines) {
        char t[64];
        snprintf(t, sizeof(t), "%lu Zeilen verloren (Puffer voll)", (unsigned long)lostLines);
        lostLines = 0;
        rs485log_note(t);
    }
    if (bufLen >= RS485LOG_FLUSH_AT || (bufLen && millis() - lastFlushMs >= RS485LOG_FLUSH_MS))
        flush();
}
