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

// ========== Feste Kopplung Node <-> Steuerung ==========
// Ein Node merkt sich die MAC des Masters, der ihm zuerst ein REGISTER_ACK geschickt hat
// (NVS), und nimmt danach nur noch Pakete von diesem Master an. Der Master antwortet nur
// Nodes aus seiner Kopplungsliste -- so stoeren sich zwei Steuerungen in Funkreichweite nicht.
#define MSG_UNPAIR      0x50  // Master→Slave: Kopplung loesen, Node vergisst seinen Master

// ========== Node-ID-Vergabe beim Koppeln ==========
// REGISTER     payload: [0] Knotentyp, [1] Faehigkeiten (NODE_CAP_*; aeltere Nodes senden nur [0])
// REGISTER_ACK payload: [0] Node-ID, [1] Flags (ACK_FLAG_*; aeltere Master senden nur [0])
// Ein Node mit NODE_CAP_ID_ASSIGN uebernimmt die ID aus einem REGISTER_ACK mit
// ACK_FLAG_ID_ASSIGN und speichert sie -- die ID wird so beim Koppeln im Webinterface des
// Masters vergeben, ohne serielles Setup. Ein Node ohne ID meldet sich mit NODE_ID_UNASSIGNED.
#define NODE_CAP_ID_ASSIGN  0x01
#define ACK_FLAG_ID_ASSIGN  0x01
#define NODE_ID_UNASSIGNED  0

#define OTA_AP_SSID    "Aeroponik-OTA"
#define OTA_MASTER_IP  "192.168.4.1"

// ========== Knotentypen ==========

#define NODE_TYPE_LIGHT      0x01   // Lichtsteuerung (PWM/Licht-Meter)
#define NODE_TYPE_SENSOR     0x02   // Multisensor (Luft + Licht)
#define NODE_TYPE_SOCKET     0x03   // Steckdose (allgemein)
#define NODE_TYPE_LIGHT_SSR  0x05   // Licht-SSR (Sonnenauf/-untergang via Scheduler)
#define NODE_TYPE_EC_METER   0x06   // EC-Handmessgeraet (meist ausgeschaltet -- kein Offline-Alarm)

#define ESPNOW_DEFAULT_CHANNEL  6   // Fester Kanal wenn kein WLAN-Router

// ========== Befehls-Subtypen (MSG_COMMAND payload[0]) ==========

#define CMD_SET_PWM   0x01   // payload[1..2] = uint16 PWM (0–1000 = 0–100.0%)
#define CMD_SET_ONOFF 0x02   // payload[1] = Bitmaske (Bit0=R1 Bit1=R2 Bit2=R3 Bit3=R4)

// ========== Daten-Subtypen (MSG_SENSOR_DATA payload[0]) ==========

#define SUBTYPE_RELAY_STATUS   0x10  // payload[1] = Bitmaske aktiver Relais
#define SUBTYPE_PWM_STATUS     0x11  // payload[1..2]=CH1, payload[3..4]=CH2 (0-1000)

// EC-Messung vom EC-Meter (auf Tastendruck, nicht periodisch):
//   [1]    Kategorie (EC_KAT_*)
//   [2..3] EC bei 25 °C [uS/cm], uint16 big-endian
//   [4..5] Temperatur * 100 [°C], int16 big-endian
//   [6]    Flags: Bit0 = Temperatur gueltig (sonst ohne Kompensation), Bit1 = Messbereich ueberschritten
//   [7..8] EC roh (bei Messtemperatur) [uS/cm], uint16 big-endian
//   [9]    Mess-ID -- Wiederholungen derselben Messung (ACK verloren) haben dieselbe ID
//   nur EC/pH-Meter (aeltere Master werten nur [0..9] aus):
//   [10..11] pH * 100, uint16 big-endian (0 wenn ungueltig)
//   [12]     pH-Flags: Bit0 = pH gueltig, Bit1 = pH-Sonde zweipunkt-kalibriert
#define SUBTYPE_EC_MEASUREMENT 0x12
#define EC_MEAS_LEN            10
#define EC_MEAS_LEN_PH         13
#define EC_PHFLAG_OK           0x01
#define EC_PHFLAG_CAL          0x02
#define EC_KAT_NAEHRLOESUNG    1
#define EC_KAT_RUECKLAUF       2
#define EC_KAT_CALMAG          3
#define EC_KAT_ROHWASSER       4
#define EC_KAT_VE_WASSER       5
#define EC_KAT_ANZAHL          5   // gueltig: 1..EC_KAT_ANZAHL (aeltere Master verwerfen 4/5)
#define EC_FLAG_TEMP_OK        0x01
#define EC_FLAG_OVERRANGE      0x02

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
