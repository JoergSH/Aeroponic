#ifndef FW_UPDATE_H
#define FW_UPDATE_H

#include <stdint.h>
#include <stdbool.h>

// Firmware-Version dieses Moduls -- bei jeder Aenderung hochzaehlen. Der Master fragt sie per
// FW_SUB_INFO ab und zeigt sie an (vor und nach einem Update), damit erkennbar ist, ob ein
// Update tatsaechlich angekommen ist.
#define FW_VERSION_MAJOR  1
#define FW_VERSION_MINOR  2
#define FW_VERSION_PATCH  0

// Firmware-Update ueber RS485 -- eigener Funktionscode im benutzerdefinierten Modbus-Bereich
// (65-72), nur auf der Luefter-Adresse 0x51 (immer aktiv, unabhaengig von der I2C-Bestueckung).
// Gegenstueck im Master: rs485_ota_*() in Aeroponik/src/rs485.cpp -- Konstanten dort identisch.
//
// Anfrage: [addr][0x41][sub][arg ...][crc_lo][crc_hi]  (immer >= 8 Bytes, ggf. mit 0 aufgefuellt)
//   FW_SUB_INFO   arg = 4 Byte 0                 -> value = Version (major<<16 | minor<<8 | patch)
//   FW_SUB_BEGIN  arg = Groesse u32 BE + MD5 16 Byte (binaer)
//   FW_SUB_DATA   arg = Offset u32 BE + Laenge u8 + Daten
//                                                -> value = naechster erwarteter Offset
//   FW_SUB_END    arg = 4 Byte 0                 -> nach OK-Antwort Neustart, der OTA-Bootloader
//                                                   kopiert das neue Image beim Boot
//   FW_SUB_ABORT  arg = 4 Byte 0
// Antwort:  [addr][0x41][sub][status][value u32 BE][crc_lo][crc_hi]  (10 Bytes)
#define RS485_FC_FWUPDATE  0x41

#define FW_SUB_INFO   0x00
#define FW_SUB_BEGIN  0x01
#define FW_SUB_DATA   0x02
#define FW_SUB_END    0x03
#define FW_SUB_ABORT  0x04

#define FW_ST_OK          0
#define FW_ST_NOT_ACTIVE  1   // DATA/END ohne vorheriges BEGIN
#define FW_ST_OFFSET      2   // Offset passt nicht, value = erwarteter Offset (Master setzt dort fort)
#define FW_ST_UPDATER     3   // Fehler des Updaters (value = Update.getError())
#define FW_ST_BAD_LEN     4   // Frame zu kurz / Laengenangabe passt nicht
#define FW_ST_BAD_SUB     5   // unbekannte Sub-Funktion
#define FW_ST_BAD_IMAGE   6   // erster Block passt nicht zu diesem Board (kein RP2040-Image / anderer boot2)

#define FW_DATA_MAX       200 // max. Nutzdaten pro DATA-Frame (RX-Puffer des Slaves: 256)

// Verarbeitet eine FC41-Anfrage (CRC bereits geprueft). Fuellt status/value fuer die Antwort.
// Rueckgabe true = nach dem Senden der Antwort neu starten (Update erfolgreich gestaged).
bool fw_update_handle(const uint8_t* frame, uint8_t len, uint8_t* status, uint32_t* value);

#endif
