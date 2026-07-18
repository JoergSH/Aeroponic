#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// ========== Paketstruktur (identisch in Master und Slave) ==========

typedef struct {
    uint8_t  node_id;       // 1–254, 0 = Master, 255 = Broadcast
    uint8_t  msg_type;
    uint8_t  seq;
    uint8_t  payload_len;   // 0–16
    uint8_t  payload[16];
    uint8_t  crc8;
} __attribute__((packed)) espnow_packet_t;  // 21 Byte

// ========== Nachrichtentypen ==========

#define MSG_REGISTER      0x01
#define MSG_REGISTER_ACK  0x02
#define MSG_HEARTBEAT     0x03
#define MSG_HEARTBEAT_ACK 0x04
#define MSG_SENSOR_DATA   0x10
#define MSG_DATA_ACK      0x11
#define MSG_COMMAND       0x20
#define MSG_COMMAND_ACK   0x21
#define MSG_CONFIG        0x30
#define MSG_CONFIG_ACK    0x31
#define MSG_ERROR         0xFF

// ========== OTA-Update ==========

#define MSG_OTA_START   0x40  // Master→Slave: payload[0..7] = AP-Passwort (8 Byte)
#define MSG_OTA_ACK     0x41  // Slave→Master: OTA startet

#define OTA_AP_SSID    "Aeroponik-OTA"
#define OTA_MASTER_IP  "192.168.4.1"

// ========== Knotentypen ==========

#define NODE_TYPE_LIGHT      0x01   // Lichtsteuerung (PWM/Licht-Meter)
#define NODE_TYPE_SENSOR     0x02   // Multisensor (Luft + Licht)
#define NODE_TYPE_SOCKET     0x03   // Steckdose (allgemein)
#define NODE_TYPE_SPEKTRUM   0x04   // AS7341 Spektrum-Lichtsensor
#define NODE_TYPE_LIGHT_SSR  0x05   // Licht-SSR (Sonnenauf/-untergang via Scheduler)

#define ESPNOW_DEFAULT_CHANNEL  6   // Fester Kanal wenn kein WLAN-Router

// ========== Befehls-Subtypen (MSG_COMMAND payload[0]) ==========

#define CMD_SET_PWM   0x01   // payload[1..2] = uint16 PWM (0–1000 = 0–100.0%)
#define CMD_SET_ONOFF 0x02   // payload[1] = Bitmaske (Bit0=R1 Bit1=R2 Bit2=R3 Bit3=R4)

// ========== Daten-Subtypen (MSG_SENSOR_DATA payload[0]) ==========

#define SUBTYPE_RELAY_STATUS   0x10  // payload[1] = Bitmaske aktiver Relais
#define SUBTYPE_PWM_STATUS     0x11  // payload[1..2]=CH1, payload[3..4]=CH2 (0-1000)

// AS7341 Spektrum-Zusammenfassung (16 Byte):
// payload[0]  = 0x20, [1..2]=ppfd×10 uint16BE, [3..4]=rb×100 uint16BE,
// [5]=bluepct, [6]=redpct, [7]=sat, [8..15]=Kanäle F1-F8 normiert 0-255
#define SUBTYPE_LICHT_SUMMARY  0x20

// ========== CRC8 (Polynom 0x07) ==========

static inline uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    return crc;
}

#endif
