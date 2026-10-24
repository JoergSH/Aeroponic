#include "fw_update.h"
#include <Arduino.h>
#include <Update.h>
#include <esp_app_format.h>
#include <string.h>

// Nutzt das ESP32-Update: das neue Image wird in die jeweils andere OTA-App-Partition
// geschrieben (Standard-Partitionstabelle, 2 App-Slots) und erst nach erfolgreicher MD5-Pruefung
// in Update.end() als Boot-Partition markiert. Bricht die Uebertragung ab, bootet das Modul
// weiter die laufende Firmware.

static bool     active   = false;
static uint32_t total    = 0;
static uint32_t expected = 0;   // naechster erwarteter Offset

static uint32_t be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void abort_update() {
    if (active) Update.abort();
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
        if (!Update.begin(size, U_FLASH)) {
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
            // Plausibilitaet: ESP-Image-Header (Magic 0xE9) mit passender Chip-ID -- verhindert
            // z.B., dass die Firmware eines ESP32-S3 (Master) oder RP2040 (Multimodul) hier landet.
            const esp_image_header_t* h = (const esp_image_header_t*)(arg + 5);
            if (n < sizeof(esp_image_header_t) || h->magic != ESP_IMAGE_HEADER_MAGIC ||
                h->chip_id != (esp_chip_id_t)CONFIG_IDF_FIRMWARE_CHIP_ID) {
                *status = FW_ST_BAD_IMAGE;
                Serial.println("[FWUPD] Erster Block ist kein Image fuer diesen Chip -- Update abgelehnt");
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
        if (!Update.end()) {   // prueft MD5, setzt erst dann die neue Boot-Partition
            *status = FW_ST_UPDATER;
            *value  = Update.getError();
            Serial.printf("[FWUPD] end() fehlgeschlagen (MD5?), Fehler %u\n", Update.getError());
            abort_update();
            return false;
        }
        active = false;
        Serial.println("[FWUPD] Update OK -- Neustart in die neue Firmware");
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
