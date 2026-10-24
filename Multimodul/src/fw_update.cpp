#include "fw_update.h"
#include <Arduino.h>
#include <Updater.h>
#include <string.h>

// Nutzt den Updater des arduino-pico-Cores: das neue Image wird zuerst komplett als Datei
// ("firmware.bin") ins LittleFS geschrieben (board_build.filesystem_size in platformio.ini) und
// per MD5 geprueft; erst danach wird es per picoOTA zum Kopieren vorgemerkt. Der eigentliche
// Flash-Tausch passiert im OTA-Bootloader beim naechsten Start. Bricht die Uebertragung ab
// (Bus-Stoerung, Master-Neustart, ...), bleibt die laufende Firmware also unangetastet.

static bool     active   = false;
static uint32_t total    = 0;
static uint32_t expected = 0;   // naechster erwarteter Offset

static uint32_t be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void abort_update() {
    if (active) Update.end(false);   // unvollstaendig -> Updater verwirft die Datei
    active   = false;
    total    = 0;
    expected = 0;
}

bool fw_update_handle(const uint8_t* frame, uint8_t len, uint8_t* status, uint32_t* value) {
    const uint8_t  sub    = frame[2];
    const uint8_t* arg    = frame + 3;
    const uint8_t  argLen = len - 5;   // ohne addr/func/sub und CRC
    *status = FW_ST_OK;
    *value  = 0;

    switch (sub) {
    case FW_SUB_INFO:
        *value = (uint32_t)FW_VERSION_MAJOR << 16 | FW_VERSION_MINOR << 8 | FW_VERSION_PATCH;
        return false;

    case FW_SUB_BEGIN: {
        if (argLen < 20) { *status = FW_ST_BAD_LEN; return false; }
        abort_update();   // evtl. haengengebliebenes vorheriges Update verwerfen
        uint32_t size = be32(arg);
        char md5[33];
        for (int i = 0; i < 16; i++) sprintf(md5 + i * 2, "%02x", arg[4 + i]);
        if (!Update.begin(size)) {
            *status = FW_ST_UPDATER;
            *value  = Update.getError();
            Serial.printf("[FWUPD] begin(%lu) fehlgeschlagen, Fehler %u\n",
                          (unsigned long)size, Update.getError());
            return false;
        }
        Update.setMD5(md5);
        active   = true;
        total    = size;
        expected = 0;
        Serial.printf("[FWUPD] Update gestartet: %lu Bytes, MD5 %s\n", (unsigned long)size, md5);
        return false;
    }

    case FW_SUB_DATA: {
        if (!active) { *status = FW_ST_NOT_ACTIVE; return false; }
        if (argLen < 5) { *status = FW_ST_BAD_LEN; return false; }
        uint32_t off = be32(arg);
        uint8_t  n   = arg[4];
        if (argLen != 5 + n || off + n > total) { *status = FW_ST_BAD_LEN; return false; }
        if (off != expected) {
            // Typisch: unsere letzte Antwort ging verloren und der Master wiederholt einen schon
            // geschriebenen Block -- nicht nochmal schreiben, nur den richtigen Offset melden.
            *status = FW_ST_OFFSET;
            *value  = expected;
            return false;
        }
        if (off == 0) {
            // Plausibilitaet: ein Image beginnt mit boot2 (256 Byte ab 0x10000000), der bei
            // gleichem Board/Flash-Chip identisch mit dem laufenden ist. Der Updater selbst prueft
            // den Inhalt NICHT -- ohne diesen Vergleich wuerde z.B. eine versehentlich gewaehlte
            // ESP32-.bin geflasht und das Modul waere nur noch per USB (BOOTSEL) zu retten.
            uint8_t cmp = n < 256 ? n : 256;
            if (memcmp(arg + 5, (const void*)0x10000000, cmp) != 0) {
                *status = FW_ST_BAD_IMAGE;
                Serial.println("[FWUPD] Erster Block passt nicht zu diesem Board -- Update abgelehnt");
                abort_update();
                return false;
            }
        }
        if (Update.write((uint8_t*)(arg + 5), n) != n) {
            *status = FW_ST_UPDATER;
            *value  = Update.getError();
            Serial.printf("[FWUPD] write bei Offset %lu fehlgeschlagen, Fehler %u\n",
                          (unsigned long)off, Update.getError());
            abort_update();
            return false;
        }
        expected += n;
        *value = expected;
        return false;
    }

    case FW_SUB_END:
        if (!active) { *status = FW_ST_NOT_ACTIVE; return false; }
        if (expected != total) {
            *status = FW_ST_OFFSET;
            *value  = expected;
            return false;
        }
        if (!Update.end()) {   // prueft MD5, merkt das Image fuer den OTA-Bootloader vor
            *status = FW_ST_UPDATER;
            *value  = Update.getError();
            Serial.printf("[FWUPD] end() fehlgeschlagen (MD5?), Fehler %u\n", Update.getError());
            abort_update();
            return false;
        }
        active = false;
        Serial.println("[FWUPD] Update OK -- Neustart, Bootloader uebernimmt das neue Image");
        return true;

    case FW_SUB_ABORT:
        abort_update();
        Serial.println("[FWUPD] Update abgebrochen");
        return false;

    default:
        *status = FW_ST_BAD_SUB;
        return false;
    }
}
