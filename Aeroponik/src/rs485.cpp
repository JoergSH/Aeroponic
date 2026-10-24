#include "rs485.h"
#include "pinout.h"
#include "dbg.h"
#include <HardwareSerial.h>
#include <Arduino.h>
#include <string.h>
#include "driver/gpio.h"

#define RS485_BAUD           115200   // alle Slaves (Lichtx4, Multisensor, Multimodul, ProtoRS485)

// Optionale lokale Einstellungen (src/local_config.h, per .gitignore NICHT im Repo).
// Dort z.B. `#define LICHTX4_LEGACY_9600` fuer ein Lichtx4, das noch mit alter Firmware auf
// 9600 Baud laeuft und (eingebaut) nicht neu geflasht werden kann: der Master schaltet dann
// nur fuer die Lichtx4-Transaktionen kurz auf 9600 Baud um. Ohne die Datei laufen alle
// Module (inkl. Lichtx4) mit RS485_BAUD.
#if __has_include("local_config.h")
#include "local_config.h"
#endif

#ifdef LICHTX4_LEGACY_9600
#define LICHT_BAUD_BEGIN()   RS485Serial.updateBaudRate(9600)
#define LICHT_BAUD_END()     RS485Serial.updateBaudRate(RS485_BAUD)
#else
#define LICHT_BAUD_BEGIN()   do {} while (0)
#define LICHT_BAUD_END()     do {} while (0)
#endif
#define POLL_INTERVAL_MS     15000UL
#define LICHT_POLL_INTERVAL_MS 15000UL
#define FAN_POLL_INTERVAL_MS  3000UL  // Luefter-Leistung aendert sich schneller als Zeitplan/Ventile
#define FAN_HEARTBEAT_INTERVAL_MS 6000UL  // Takt des echten iControl-Broadcasts, siehe unten
#define RESPONSE_TIMEOUT_MS  2000UL  // DS18B20 requestTemperatures() blockiert bis 750ms
#define FRAME_TIMEOUT_US     4000
#define RX_BUF_SIZE          64

static HardwareSerial RS485Serial(1);

static uint16_t crc16(const uint8_t* d, size_t n) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        crc ^= d[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

// ========== Gespeicherte Daten ==========

static Rs485SensorData ms2    = {false, 0, 0, 0, 0, 0, {}, 0, 0};
static Rs485LichtData  licht  = {false, 0, 0};
static Rs485FanData    fan    = {false, 0, 0, 0};
static Rs485AnalogData analog = {false, 0, 0, 0};
static Rs485LueftData  lueft  = {false, {0,0,0,0}, {0,0,0,0}, 0};

// ========== Zustandsmaschine ==========
// Ein Bus, ein Master, immer nur eine Anfrage gleichzeitig unterwegs.
// Reihenfolge bei freiem Bus: anstehende Schreibbefehle (Licht, Fan, Analog Ch1, Analog Ch2)
// > MS2-Poll > Licht-Poll > Fan-Poll > Analog-Poll.
// Der Luefter (Adresse 6) ist ein fremdes MARS-Hydro-Geraet, auf dessen Bus auch
// der iControl-Hub als eigener Master pollt/schreibt — siehe CLAUDE_CODE_SPEC_RS485_MARS_FAN.md.
// Licht/Fan/Analog koennen einzeln vom Pollen ausgenommen werden (siehe rs485_set_*_polling),
// je nachdem welches Geraet laut Konfiguration (outputctrl.h) tatsaechlich verbaut ist.

typedef enum { MB_IDLE, MB_WAIT_RESPONSE } mb_state_t;
typedef enum { TARGET_MS2_READ, TARGET_LICHT_WRITE, TARGET_LICHT_READ,
               TARGET_FAN_ENABLE_WRITE, TARGET_FAN_WRITE, TARGET_FAN_READ,
               TARGET_ANALOG_WRITE_CH1, TARGET_ANALOG_WRITE_CH2, TARGET_ANALOG_READ,
               TARGET_LUEFT_WRITE, TARGET_LUEFT_READ } mb_target_t;

static mb_state_t  mb_state      = MB_IDLE;
static mb_target_t active_target = TARGET_MS2_READ;
static uint32_t     poll_ms       = 0;
static uint32_t     licht_poll_ms = 0;
static uint32_t     fan_poll_ms   = 0;
static uint32_t     analog_poll_ms = 0;
static uint32_t     lueft_poll_ms  = 0;
static uint32_t     fan_heartbeat_ms = 0;
static uint32_t     send_ms       = 0;
static bool         first_poll    = true;

static bool licht_polling_enabled  = true;
static bool fan_polling_enabled    = true;
static bool analog_polling_enabled = true;
static bool lueft_polling_enabled  = true;

static bool    licht_write_pending = false;
static uint8_t licht_write_value   = 0;

static bool    fan_write_pending  = false;
static uint8_t fan_write_value    = 0;
static bool    fan_enable_pending = false;  // siehe RS485_FAN_REG_ENABLE in rs485.h

static bool     analog_ch1_write_pending = false;
static uint16_t analog_ch1_write_value   = 0;
static bool     analog_ch2_write_pending = false;
static uint16_t analog_ch2_write_value   = 0;

static bool    lueft_write_pending[RS485_LUEFT_COUNT] = {false, false, false, false};
static uint8_t lueft_write_value[RS485_LUEFT_COUNT]   = {0, 0, 0, 0};
static uint8_t lueft_write_ch_active = 0;   // welcher Kanal gerade in Bearbeitung ist

static uint8_t    rx_buf[RX_BUF_SIZE];
static uint8_t    rx_len       = 0;
static uint32_t   last_byte_us = 0;

// Selbst-Echo des MAX13487 (RO spiegelt die eigene Sendung auf DI zurueck, RE haengt
// offenbar fest auf "immer empfangen") wird HIER direkt nach dem Senden aktiv weggeraeumt
// -- genau das Muster, das in allen Slave-Firmwares (Lichtx4, Lueftermodul, ...) schon
// funktioniert. Eine inhaltliche Erkennung im Nachhinein (wie zuvor versucht) ist bei FC06
// nicht zuverlaessig moeglich: die gueltige Bestaetigung ist laut Modbus-Spec selbst ein
// Echo der Anfrage, also inhaltlich identisch mit dem eigenen Sende-Echo -- beide waeren
// nicht unterscheidbar, wuerde man sie erst hinterher am Inhalt vergleichen.
static void drainSelfEcho() {
    RS485Serial.flush();  // sicherstellen, dass der Frame komplett raus ist
    delay(1);              // Turnaround-Reserve fuer den Transceiver
    while (RS485Serial.available()) RS485Serial.read();
}

static void send_fc03(uint8_t addr, uint16_t start, uint16_t count) {
    uint8_t req[8];
    req[0] = addr;
    req[1] = 0x03;
    req[2] = start >> 8;
    req[3] = start & 0xFF;
    req[4] = count >> 8;
    req[5] = count & 0xFF;
    uint16_t c = crc16(req, 6);
    req[6] = c & 0xFF;
    req[7] = c >> 8;
    RS485Serial.write(req, 8);
    drainSelfEcho();
    rx_len = 0;
}

static void send_fc06(uint8_t addr, uint16_t reg, uint16_t value) {
    uint8_t req[8];
    req[0] = addr;
    req[1] = 0x06;
    req[2] = reg >> 8;
    req[3] = reg & 0xFF;
    req[4] = value >> 8;
    req[5] = value & 0xFF;
    uint16_t c = crc16(req, 6);
    req[6] = c & 0xFF;
    req[7] = c >> 8;
    RS485Serial.write(req, 8);
    drainSelfEcho();
    rx_len = 0;
}

static void send_fan_heartbeat_broadcast() {
    // Experiment (23.10.2026, siehe MARS_Hydro_RE_Notizen.md Abschnitt 5b): der Luefter
    // scheint einen "echten" Bus-Master nicht am blossen Polling zu erkennen, sondern an
    // diesem periodischen Broadcast der echten iControl-Steuerung (addr=0, FC10,
    // start=1001, count=26 Register, reg[8]=7 als Signatur). Ohne ihn bleibt er im
    // Standalone-Modus (eigenes Polling, ignoriert normale Leseanfragen auf seine Adresse).
    // Broadcast = keine Antwort erwartet, daher kein MB_WAIT_RESPONSE noetig.
    uint16_t regs[26] = {0};
    regs[8] = 7;

    uint8_t req[7 + 26 * 2 + 2];
    req[0] = 0;        // Broadcast-Adresse
    req[1] = 0x10;
    req[2] = 1001 >> 8;
    req[3] = 1001 & 0xFF;
    req[4] = 26 >> 8;
    req[5] = 26 & 0xFF;
    req[6] = 26 * 2;
    for (int i = 0; i < 26; i++) {
        req[7 + i * 2]     = regs[i] >> 8;
        req[7 + i * 2 + 1] = regs[i] & 0xFF;
    }
    uint16_t c = crc16(req, 7 + 26 * 2);
    req[7 + 26 * 2]     = c & 0xFF;
    req[7 + 26 * 2 + 1] = c >> 8;

    RS485Serial.write(req, sizeof(req));
    drainSelfEcho();
    rx_len = 0;
}

static void parse_ms2_response(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=28][14 × 2 bytes][crc_lo][crc_hi]
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_MS2 || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != RS485_MS2_REGS * 2) return;
    if (len < (uint8_t)(3 + byte_count + 2)) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler MS2");
        return;
    }

    const uint8_t* d = buf + 3;
    uint16_t regs[RS485_MS2_REGS];
    for (int i = 0; i < RS485_MS2_REGS; i++)
        regs[i] = (uint16_t)d[i*2] << 8 | d[i*2+1];

    ms2.co2_ppm       = regs[0];
    ms2.temp_c        = (int16_t)regs[1] / 10.0f;
    ms2.hum_pct       = regs[2] / 10.0f;
    ms2.ppfd          = regs[3] / 10.0f;
    for (int i = 0; i < 8; i++) ms2.channels[i] = (uint8_t)regs[4 + i];
    ms2.gain          = (uint8_t)regs[12];
    ms2.status        = (uint8_t)regs[13];
    ms2.online        = true;
    ms2.last_update_ms = millis();

    dbgPrintf("[RS485] MS2: CO2=%u ppm  T=%.1f°C  H=%.1f%%  PPFD=%.1f\n",
                  ms2.co2_ppm, ms2.temp_c, ms2.hum_pct, ms2.ppfd);
}

static void parse_licht_read(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=2][2 bytes][crc_lo][crc_hi]
    if (len < 7) return;
    if (buf[0] != RS485_ADDR_LICHT || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != 2) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Licht");
        return;
    }

    // Wie beim Lueftermodul: kam der Slave gerade aus einem eigenen Reset zurueck (Master lief
    // durch), hat er seinen letzten Zustand verloren -- der Master schreibt aber nur bei
    // Aenderung, sieht hier also keinen Grund zum Neuschreiben. Bei Wiederverbindung deshalb
    // einmalig die zuletzt kommandierte Maske erneut schreiben.
    if (!licht.online) {
        dbgPrintln("[RS485] Licht wieder erreichbar -- synchronisiere Maske neu");
        licht_write_pending = true;
    }

    licht.mask           = (uint8_t)((uint16_t)buf[3] << 8 | buf[4]);
    licht.online          = true;
    licht.last_update_ms = millis();
    dbgPrintf("[RS485] Licht: Maske=0x%02X\n", licht.mask);
}

static void parse_licht_write_ack(const uint8_t* buf, uint8_t len) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_LICHT) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        dbgPrintf("[RS485] Licht lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        licht.online = true;  // Geraet antwortet zumindest, nur der Befehl war ungueltig
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Licht-ACK");
        return;
    }
    licht.mask            = licht_write_value;
    licht.online          = true;
    licht.last_update_ms  = millis();
    dbgPrintf("[RS485] Licht-ACK: Maske=0x%02X bestaetigt\n", licht.mask);
}

static void parse_fan_read(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=34][17 x 2 bytes][crc_lo][crc_hi], Register 0-16 am Stueck
    // (siehe CLAUDE_CODE_SPEC_RS485_MARS_FAN.md) -- RPM/Prozent liegen an fester Position
    // innerhalb des Blocks (RS485_FAN_REG_RPM/_PERCENT, siehe rs485.h).
    if (len < 3 + RS485_FAN_READ_COUNT * 2 + 2) return;
    if (buf[0] != RS485_ADDR_FAN || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != RS485_FAN_READ_COUNT * 2) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Fan");
        return;
    }

    uint8_t rpmOff = 3 + RS485_FAN_REG_RPM * 2;
    uint8_t pctOff = 3 + RS485_FAN_REG_PERCENT * 2;
    fan.rpm            = (uint16_t)buf[rpmOff] << 8 | buf[rpmOff + 1];
    fan.percent         = (uint8_t)((uint16_t)buf[pctOff] << 8 | buf[pctOff + 1]);
    fan.online          = true;
    fan.last_update_ms  = millis();
    dbgPrintf("[RS485] Fan: %u%%  %u RPM\n", fan.percent, fan.rpm);
}

static void parse_fan_enable_ack(const uint8_t* buf, uint8_t len) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_FAN) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        dbgPrintf("[RS485] Fan lehnt Enable-Befehl ab (Code 0x%02X)\n", buf[2]);
        fan.online = true;
        return;
    }

    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Fan-Enable-ACK");
        return;
    }
    fan.online         = true;
    fan.last_update_ms = millis();
    dbgPrintln("[RS485] Fan-Enable-ACK bestaetigt");
}

static void parse_fan_write_ack(const uint8_t* buf, uint8_t len) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_FAN) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        dbgPrintf("[RS485] Fan lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        fan.online = true;
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Fan-ACK");
        return;
    }
    fan.percent         = fan_write_value;
    fan.online          = true;
    fan.last_update_ms  = millis();
    dbgPrintf("[RS485] Fan-ACK: Leistung=%u%% bestaetigt\n", fan.percent);
}

static void parse_analog_read(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=4][2 × 2 bytes][crc_lo][crc_hi]
    if (len < 9) return;
    if (buf[0] != RS485_ADDR_ANALOG || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != 4) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Analog");
        return;
    }

    // Wie bei Lueftermodul/Licht: Slave kann durch eigenen Reset seinen Zustand verloren haben,
    // ohne dass der Master eine Aenderung sieht -- bei Wiederverbindung beide Kanaele mit dem
    // zuletzt kommandierten Wert neu schreiben.
    if (!analog.online) {
        dbgPrintln("[RS485] Analog wieder erreichbar -- synchronisiere beide Kanaele neu");
        analog_ch1_write_pending = true;
        analog_ch2_write_pending = true;
    }

    analog.ch1_raw       = (uint16_t)buf[3] << 8 | buf[4];
    analog.ch2_raw       = (uint16_t)buf[5] << 8 | buf[6];
    analog.online         = true;
    analog.last_update_ms = millis();
    dbgPrintf("[RS485] Analog: Ch1=%u  Ch2=%u\n", analog.ch1_raw, analog.ch2_raw);
}

static void parse_analog_write_ack(const uint8_t* buf, uint8_t len, uint16_t written_value, bool is_ch1) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_ANALOG) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        dbgPrintf("[RS485] Analog lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        analog.online = true;
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Analog-ACK");
        return;
    }
    if (is_ch1) analog.ch1_raw = written_value;
    else        analog.ch2_raw = written_value;
    analog.online          = true;
    analog.last_update_ms  = millis();
    dbgPrintf("[RS485] Analog-ACK: %s=%u bestaetigt\n", is_ch1 ? "Ch1" : "Ch2", written_value);
}

static void parse_lueft_read(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=10][4 × 2 bytes RPM][2 bytes Boot-ID][crc_lo][crc_hi]
    // (RPM 4-7 + Boot-ID 8, siehe RS485_LUEFT_REG_BOOTID) -- ein Register mehr als frueher.
    if (len < 15) return;
    if (buf[0] != RS485_ADDR_LUEFT || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != (RS485_LUEFT_COUNT + 1) * 2) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Lueftermodul");
        return;
    }

    // Resync-Erkennung, zweifach abgesichert:
    //  1) Boot-ID (Reg 8) hat sich gegenueber dem letzten Poll geaendert -- erkennt JEDEN
    //     Neustart des Slaves zuverlaessig, auch einen zu kurzen, um als Bus-Unterbrechung
    //     aufzufallen (der Master saehe sonst gar keine Lücke).
    //  2) Fallback: Slave war zuvor als offline erkannt (z.B. laengerer Ausfall).
    // In beiden Faellen hat der Slave seine PWM-Register auf 0 verloren, der Master schreibt
    // Sollwerte aber nur bei Aenderung neu (siehe loopLueftmodul() in main.cpp) -- deshalb hier
    // einmalig alle 4 Kanaele mit dem zuletzt kommandierten Wert (lueft_write_value[], bleibt im
    // Master-RAM erhalten) neu schreiben.
    static int32_t lastBootId = -1;  // -1 = noch keinen gueltigen Wert gesehen (erzwingt Erstsync)
    uint16_t bootId = (uint16_t)buf[3 + RS485_LUEFT_COUNT * 2] << 8 | buf[4 + RS485_LUEFT_COUNT * 2];
    bool rebooted = (bootId != lastBootId);
    if (rebooted || !lueft.online) {
        dbgPrintf("[RS485] Lueftermodul %s -- synchronisiere PWM-Sollwerte neu (Boot-ID 0x%04X)\n",
                      rebooted ? "neu gestartet" : "wieder erreichbar", bootId);
        for (int i = 0; i < RS485_LUEFT_COUNT; i++) lueft_write_pending[i] = true;
    }
    // Analog (0x50, GP8403) sitzt physisch auf demselben Multimodul-Board wie das Lueftermodul
    // (0x51) -- ein echter Neustart (Boot-ID-Wechsel) betrifft also immer beide gemeinsam. Eigene
    // Boot-ID fuer Analog waere redundant, deshalb hier huckepack mit resynchronisieren (nur bei
    // echtem Neustart, nicht beim bloss ausgebliebenen Fallback, da der nicht zwingend das ganze
    // Board betrifft).
    if (rebooted) {
        analog_ch1_write_pending = true;
        analog_ch2_write_pending = true;
    }
    lastBootId = bootId;

    for (int i = 0; i < RS485_LUEFT_COUNT; i++)
        lueft.rpm[i] = (uint16_t)buf[3 + i * 2] << 8 | buf[4 + i * 2];
    lueft.online          = true;
    lueft.last_update_ms  = millis();
    dbgPrintf("[RS485] Lueftermodul: RPM %u/%u/%u/%u\n",
                  lueft.rpm[0], lueft.rpm[1], lueft.rpm[2], lueft.rpm[3]);
}

static void parse_lueft_write_ack(const uint8_t* buf, uint8_t len, uint8_t channel, uint8_t value) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_LUEFT) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        dbgPrintf("[RS485] Lueftermodul lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        lueft.online = true;
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        dbgPrintln("[RS485] CRC-Fehler Lueftermodul-ACK");
        return;
    }
    lueft.percent[channel] = value;
    lueft.online           = true;
    lueft.last_update_ms   = millis();
    dbgPrintf("[RS485] Lueftermodul-ACK: Kanal %u -> %u%% bestaetigt\n", channel + 1, value);
}

// ========== Public ==========

// ========== Firmware-Update eines Slaves (FC41) ==========
// Protokoll/Konstanten identisch zu Multimodul/src/fw_update.h. Laeuft als eigene kleine
// Zustandsmaschine, die den Bus komplett uebernimmt (siehe loopRS485()), damit sich Update-
// Frames und normale Polls nie ueberschneiden.
#define OTA_ADDR             RS485_ADDR_LUEFT
#define OTA_FC               0x41
#define OTA_SUB_INFO         0x00
#define OTA_SUB_BEGIN        0x01
#define OTA_SUB_DATA         0x02
#define OTA_SUB_END          0x03
#define OTA_SUB_ABORT        0x04
#define OTA_ST_OK            0
#define OTA_ST_OFFSET        2
#define OTA_ST_BAD_IMAGE     6
#define OTA_CHUNK            200
#define OTA_MAX_RETRIES      5
#define OTA_TIMEOUT_MS       1000UL   // DATA: Flash-Schreiben im Slave dauert pro 4-KB-Seite etwas
#define OTA_TIMEOUT_LONG_MS  15000UL  // BEGIN (LittleFS evtl. erst formatieren) / END (MD5 + Vormerken)
#define OTA_REBOOT_WAIT_MS   60000UL  // Slave-Neustart inkl. Kopieren durch den OTA-Bootloader
#define OTA_REBOOT_POLL_MS   1500UL
#define OTA_REBOOT_TRY_MS    500UL

static Rs485OtaStatus ota = {RS485_OTA_IDLE, 0, 0, false, 0, 0, ""};
static const uint8_t* ota_img = nullptr;
static uint8_t  ota_md5[16];
static bool     ota_requested  = false;  // wartet darauf, dass die laufende normale Abfrage fertig ist
static bool     ota_running    = false;  // hat den Bus uebernommen
static bool     ota_waiting    = false;  // Antwort auf die letzte Anfrage ausstehend
static bool     ota_abort_req  = false;
static uint8_t  ota_last_sub   = 0;
static uint8_t  ota_retries    = 0;
static uint32_t ota_send_ms    = 0;
static uint32_t ota_timeout_ms = OTA_TIMEOUT_MS;
static uint32_t ota_reboot_ms  = 0;

static void put_be32(uint8_t* p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static uint32_t get_be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void ota_finish(Rs485OtaState state) {
    ota.state   = state;
    ota_running = false;
    ota_waiting = false;
}

static void ota_fail(const char* msg) {
    snprintf(ota.error, sizeof(ota.error), "%s", msg);
    dbgPrintf("[RS485-OTA] Fehler: %s\n", msg);
    ota_finish(RS485_OTA_ERROR);
}

static void ota_send(uint8_t sub, const uint8_t* arg, uint8_t argLen, uint32_t timeout_ms) {
    uint8_t f[3 + 5 + OTA_CHUNK + 2];
    uint8_t n = argLen < 4 ? 4 : argLen;   // Frames immer >= 8 Bytes, kuerzere verwirft der Slave
    f[0] = OTA_ADDR;
    f[1] = OTA_FC;
    f[2] = sub;
    memset(f + 3, 0, n);
    if (argLen) memcpy(f + 3, arg, argLen);
    uint16_t c = crc16(f, 3 + n);
    f[3 + n] = c & 0xFF;
    f[4 + n] = c >> 8;
    RS485Serial.write(f, 5 + n);
    drainSelfEcho();
    rx_len         = 0;
    ota_last_sub   = sub;
    ota_send_ms    = millis();
    ota_timeout_ms = timeout_ms;
    ota_waiting    = true;
}

// (Re-)sendet die zum aktuellen Zustand passende Anfrage.
static void ota_send_current() {
    switch (ota.state) {
    case RS485_OTA_VERSION:
    case RS485_OTA_INFO:
        ota_send(OTA_SUB_INFO, nullptr, 0, OTA_TIMEOUT_MS);
        break;
    case RS485_OTA_REBOOT_WAIT:
        ota_send(OTA_SUB_INFO, nullptr, 0, OTA_REBOOT_TRY_MS);
        break;
    case RS485_OTA_BEGIN: {
        uint8_t a[20];
        put_be32(a, ota.total);
        memcpy(a + 4, ota_md5, 16);
        ota_send(OTA_SUB_BEGIN, a, sizeof(a), OTA_TIMEOUT_LONG_MS);
        break;
    }
    case RS485_OTA_DATA: {
        uint32_t n = ota.total - ota.sent;
        if (n > OTA_CHUNK) n = OTA_CHUNK;
        uint8_t a[5 + OTA_CHUNK];
        put_be32(a, ota.sent);
        a[4] = (uint8_t)n;
        memcpy(a + 5, ota_img + ota.sent, n);
        ota_send(OTA_SUB_DATA, a, 5 + n, OTA_TIMEOUT_MS);
        break;
    }
    case RS485_OTA_END:
        ota_send(OTA_SUB_END, nullptr, 0, OTA_TIMEOUT_LONG_MS);
        break;
    default:
        break;
    }
}

static void ota_handle_reply(uint8_t st, uint32_t val, uint32_t now) {
    char msg[80];
    switch (ota.state) {
    case RS485_OTA_VERSION:
        if (st == OTA_ST_OK) { ota.version = val; ota.version_valid = true; }
        ota_finish(RS485_OTA_IDLE);
        break;
    case RS485_OTA_INFO:
        if (st == OTA_ST_OK) { ota.version = ota.version_before = val; ota.version_valid = true; }
        dbgPrintf("[RS485-OTA] Start: %u Bytes an 0x%02X (bisherige Version %u.%u.%u)\n",
                  (unsigned)ota.total, OTA_ADDR, (unsigned)(val >> 16), (unsigned)((val >> 8) & 0xFF),
                  (unsigned)(val & 0xFF));
        ota.state = RS485_OTA_BEGIN;
        break;
    case RS485_OTA_BEGIN:
        if (st != OTA_ST_OK) {
            snprintf(msg, sizeof(msg), "Slave lehnt Update ab (Status %u, Updater-Fehler %u)",
                     st, (unsigned)val);
            ota_fail(msg);
            return;
        }
        ota.sent  = 0;
        ota.state = RS485_OTA_DATA;
        break;
    case RS485_OTA_DATA:
        if (st == OTA_ST_BAD_IMAGE) {
            ota_fail("Datei passt nicht zum Multimodul (keine RP2040-Firmware fuer dieses Board?)");
            return;
        }
        if ((st != OTA_ST_OK && st != OTA_ST_OFFSET) || val > ota.total) {
            snprintf(msg, sizeof(msg), "Schreibfehler im Slave (Status %u, Code %u)", st, (unsigned)val);
            ota_fail(msg);
            return;
        }
        ota.sent = val;   // bei OTA_ST_OFFSET: dort weitermachen, wo der Slave tatsaechlich steht
        if (ota.sent == ota.total) ota.state = RS485_OTA_END;
        break;
    case RS485_OTA_END:
        if (st != OTA_ST_OK) {
            snprintf(msg, sizeof(msg), "Abschluss fehlgeschlagen, Image verworfen (Status %u, Code %u)",
                     st, (unsigned)val);
            ota_fail(msg);
            return;
        }
        dbgPrintln("[RS485-OTA] Image im Slave vorgemerkt -- warte auf Neustart");
        ota.state     = RS485_OTA_REBOOT_WAIT;
        ota_reboot_ms = now;
        ota_send_ms   = now;   // erste Abfrage erst nach OTA_REBOOT_POLL_MS
        break;
    case RS485_OTA_REBOOT_WAIT:
        if (st != OTA_ST_OK) return;
        ota.version       = val;
        ota.version_valid = true;
        if (ota.version_before != 0 && val == ota.version_before)
            snprintf(ota.error, sizeof(ota.error),
                     "Hinweis: Versionsnummer unveraendert (FW_VERSION_* im Multimodul nicht erhoeht?)");
        dbgPrintf("[RS485-OTA] Fertig, Slave meldet Version %u.%u.%u\n", (unsigned)(val >> 16),
                  (unsigned)((val >> 8) & 0xFF), (unsigned)(val & 0xFF));
        ota_finish(RS485_OTA_DONE);
        break;
    default:
        break;
    }
}

static void ota_loop(uint32_t now) {
    if (ota_abort_req) {
        ota_abort_req = false;
        ota_send(OTA_SUB_ABORT, nullptr, 0, OTA_TIMEOUT_MS);   // best effort, Antwort egal
        ota_fail("Vom Benutzer abgebrochen");
        return;
    }

    if (!ota_waiting) {
        if (ota.state == RS485_OTA_REBOOT_WAIT) {
            if (now - ota_reboot_ms > OTA_REBOOT_WAIT_MS) {
                ota_fail("Slave nach dem Update nicht wieder erreichbar");
                return;
            }
            if (now - ota_send_ms < OTA_REBOOT_POLL_MS) return;
        }
        ota_send_current();
        return;
    }

    bool frame_done = (rx_len > 0) && ((micros() - last_byte_us) > FRAME_TIMEOUT_US);
    if (frame_done) {
        bool ok = rx_len >= 10 && rx_buf[0] == OTA_ADDR && rx_buf[1] == OTA_FC &&
                  rx_buf[2] == ota_last_sub &&
                  crc16(rx_buf, 8) == ((uint16_t)rx_buf[9] << 8 | rx_buf[8]);
        uint8_t  st  = rx_buf[3];
        uint32_t val = get_be32(rx_buf + 4);
        rx_len = 0;
        if (ok) {
            ota_waiting = false;
            ota_retries = 0;
            ota_handle_reply(st, val, now);
            return;
        }
        // kaputter Frame -> wie Timeout behandeln (unten)
    } else if (now - ota_send_ms <= ota_timeout_ms) {
        return;   // Antwort noch nicht da
    }

    ota_waiting = false;
    rx_len      = 0;
    if (ota.state == RS485_OTA_REBOOT_WAIT) return;   // Slave bootet noch -- spaeter erneut fragen
    if (++ota_retries > OTA_MAX_RETRIES) {
        ota_fail(ota.state == RS485_OTA_VERSION || ota.state == RS485_OTA_INFO
                     ? "Multimodul (0x51) antwortet nicht auf Update-Anfragen (Firmware ohne Update-Funktion?)"
                     : "Keine Antwort vom Multimodul (0x51) -- Update abgebrochen, alte Firmware bleibt aktiv");
        return;
    }
    dbgPrintf("[RS485-OTA] Keine/ungueltige Antwort (Sub %u), Wiederholung %u/%u\n",
              ota_last_sub, ota_retries, OTA_MAX_RETRIES);
}

bool rs485_ota_busy() { return ota_requested || ota_running; }

bool rs485_ota_start(const uint8_t* image, uint32_t size, const uint8_t md5[16]) {
    if (rs485_ota_busy() || !image || size == 0) return false;
    ota_img            = image;
    ota.total          = size;
    ota.sent           = 0;
    ota.version_before = 0;
    ota.error[0]       = '\0';
    memcpy(ota_md5, md5, 16);
    ota.state          = RS485_OTA_INFO;
    ota_retries        = 0;
    ota_requested      = true;
    return true;
}

void rs485_ota_query_version() {
    if (rs485_ota_busy()) return;
    ota.error[0]  = '\0';
    ota.state     = RS485_OTA_VERSION;
    ota_retries   = 0;
    ota_requested = true;
}

void rs485_ota_abort() {
    if (ota_running) {
        ota_abort_req = true;
    } else if (ota_requested) {
        ota_requested = false;
        ota_fail("Vom Benutzer abgebrochen");
    }
}

const Rs485OtaStatus& rs485_ota_status() { return ota; }

void setupRS485() {
    gpio_reset_pin((gpio_num_t)RS485_RX);
    gpio_reset_pin((gpio_num_t)RS485_TX);
    RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
    dbgPrintf("RS485 bereit (%d Baud, UART1)\n", RS485_BAUD);
}

void loopRS485() {
    uint32_t now = millis();

    // Bytes einlesen, Frame-Ende per micros() tracken
    while (RS485Serial.available()) {
        uint8_t b = (uint8_t)RS485Serial.read();
        if (rx_len < RX_BUF_SIZE) rx_buf[rx_len++] = b;
        else rx_len = 0;
        last_byte_us = micros();
    }

    // Firmware-Update: uebernimmt den Bus, sobald die gerade laufende normale Abfrage fertig ist
    // (bis dahin werden keine neuen gestartet, siehe MB_IDLE unten).
    if (ota_running) {
        ota_loop(now);
        return;
    }
    if (ota_requested && mb_state == MB_IDLE) {
        ota_requested = false;
        ota_running   = true;
        ota_waiting   = false;
        ota_loop(now);
        return;
    }

    if (mb_state == MB_WAIT_RESPONSE) {
        bool frame_done = (rx_len > 0) && ((micros() - last_byte_us) > FRAME_TIMEOUT_US);
        bool timed_out  = (now - send_ms) > RESPONSE_TIMEOUT_MS;

        if (frame_done) {
            // Kein Selbst-Echo mehr im Puffer -- das wird schon direkt beim Senden aktiv
            // weggeraeumt (siehe drainSelfEcho()), rx_buf/rx_len enthaelt hier also nur noch
            // die echte Antwort (falls eine kam).
            switch (active_target) {
                case TARGET_MS2_READ:    parse_ms2_response(rx_buf, rx_len);     break;
                case TARGET_LICHT_WRITE: parse_licht_write_ack(rx_buf, rx_len);  break;
                case TARGET_LICHT_READ:  parse_licht_read(rx_buf, rx_len);       break;
                case TARGET_FAN_ENABLE_WRITE: parse_fan_enable_ack(rx_buf, rx_len); break;
                case TARGET_FAN_WRITE:   parse_fan_write_ack(rx_buf, rx_len);    break;
                case TARGET_FAN_READ:    parse_fan_read(rx_buf, rx_len);         break;
                case TARGET_ANALOG_WRITE_CH1:
                    parse_analog_write_ack(rx_buf, rx_len, analog_ch1_write_value, true);
                    break;
                case TARGET_ANALOG_WRITE_CH2:
                    parse_analog_write_ack(rx_buf, rx_len, analog_ch2_write_value, false);
                    break;
                case TARGET_ANALOG_READ: parse_analog_read(rx_buf, rx_len);      break;
                case TARGET_LUEFT_WRITE:
                    parse_lueft_write_ack(rx_buf, rx_len, lueft_write_ch_active,
                                           lueft_write_value[lueft_write_ch_active]);
                    break;
                case TARGET_LUEFT_READ:  parse_lueft_read(rx_buf, rx_len);       break;
            }
            if (active_target == TARGET_LICHT_WRITE || active_target == TARGET_LICHT_READ) {
                LICHT_BAUD_END();  // ggf. zurueck auf die Standard-Baudrate
            }
            rx_len   = 0;
            mb_state = MB_IDLE;
        } else if (timed_out) {
            switch (active_target) {
                case TARGET_MS2_READ:
                    dbgPrintf("[RS485] MS2 Timeout (rx_len=%d)\n", rx_len);
                    ms2.online = false;
                    break;
                case TARGET_FAN_ENABLE_WRITE:
                case TARGET_FAN_WRITE:
                case TARGET_FAN_READ:
                    dbgPrintf("[RS485] Fan Timeout (rx_len=%d)\n", rx_len);
                    fan.online = false;
                    break;
                case TARGET_ANALOG_WRITE_CH1:
                case TARGET_ANALOG_WRITE_CH2:
                case TARGET_ANALOG_READ:
                    dbgPrintf("[RS485] Analog Timeout (rx_len=%d)\n", rx_len);
                    analog.online = false;
                    break;
                case TARGET_LUEFT_WRITE:
                case TARGET_LUEFT_READ:
                    dbgPrintf("[RS485] Lueftermodul Timeout (rx_len=%d)\n", rx_len);
                    lueft.online = false;
                    break;
                default:
                    dbgPrintf("[RS485] Licht Timeout (rx_len=%d)\n", rx_len);
                    licht.online = false;
                    LICHT_BAUD_END();  // ggf. zurueck auf die Standard-Baudrate
                    break;
            }
            rx_len   = 0;
            mb_state = MB_IDLE;
        }
    }

    if (mb_state == MB_IDLE && !ota_requested) {
        if (licht_write_pending) {
            licht_write_pending = false;
            send_ms             = now;
            dbgPrintf("[RS485] Licht-Maske schreiben: 0x%02X\n", licht_write_value);
            LICHT_BAUD_BEGIN();
            send_fc06(RS485_ADDR_LICHT, RS485_REG_MASK, licht_write_value);
            active_target = TARGET_LICHT_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (fan_enable_pending) {
            // Muss vor der Leistung geschrieben werden, sonst uebernimmt der Luefter den
            // Prozentwert zwar im Register, faehrt aber nicht physisch hoch (siehe
            // MARS_Hydro_RE_Notizen.md Abschnitt 5b, Fund 7/8).
            fan_enable_pending = false;
            send_ms            = now;
            dbgPrintln("[RS485] Fan-Enable schreiben (reg15=1) ...");
            send_fc06(RS485_ADDR_FAN, RS485_FAN_REG_ENABLE, 1);
            active_target = TARGET_FAN_ENABLE_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (fan_write_pending) {
            fan_write_pending = false;
            send_ms           = now;
            dbgPrintf("[RS485] Fan-Leistung schreiben: %u%%\n", fan_write_value);
            send_fc06(RS485_ADDR_FAN, RS485_FAN_REG_PERCENT, fan_write_value);
            active_target = TARGET_FAN_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (analog_ch1_write_pending) {
            analog_ch1_write_pending = false;
            send_ms                  = now;
            dbgPrintf("[RS485] Analog Ch1 schreiben: %u\n", analog_ch1_write_value);
            send_fc06(RS485_ADDR_ANALOG, RS485_REG_ANALOG_CH1, analog_ch1_write_value);
            active_target = TARGET_ANALOG_WRITE_CH1;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (analog_ch2_write_pending) {
            analog_ch2_write_pending = false;
            send_ms                  = now;
            dbgPrintf("[RS485] Analog Ch2 schreiben: %u\n", analog_ch2_write_value);
            send_fc06(RS485_ADDR_ANALOG, RS485_REG_ANALOG_CH2, analog_ch2_write_value);
            active_target = TARGET_ANALOG_WRITE_CH2;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (lueft_write_pending[0] || lueft_write_pending[1] ||
                   lueft_write_pending[2] || lueft_write_pending[3]) {
            uint8_t ch = 0;
            for (uint8_t i = 0; i < RS485_LUEFT_COUNT; i++) if (lueft_write_pending[i]) { ch = i; break; }
            lueft_write_pending[ch] = false;
            lueft_write_ch_active   = ch;
            send_ms                 = now;
            dbgPrintf("[RS485] Lueftermodul Kanal %u schreiben: %u%%\n", ch + 1, lueft_write_value[ch]);
            send_fc06(RS485_ADDR_LUEFT, RS485_LUEFT_REG_PWM_BASE + ch, lueft_write_value[ch]);
            active_target = TARGET_LUEFT_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (first_poll || (now - poll_ms) >= POLL_INTERVAL_MS) {
            first_poll = false;
            poll_ms    = now;
            send_ms    = now;
            dbgPrintln("[RS485] Poll MS2 (0x20) ...");
            send_fc03(RS485_ADDR_MS2, 0, RS485_MS2_REGS);
            active_target = TARGET_MS2_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (licht_polling_enabled && (now - licht_poll_ms >= LICHT_POLL_INTERVAL_MS)) {
            licht_poll_ms = now;
            send_ms       = now;
            dbgPrintln("[RS485] Poll Licht (0x40) ...");
            LICHT_BAUD_BEGIN();
            send_fc03(RS485_ADDR_LICHT, RS485_REG_MASK, 1);
            active_target = TARGET_LICHT_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (fan_polling_enabled && (now - fan_poll_ms >= FAN_POLL_INTERVAL_MS)) {
            fan_poll_ms = now;
            send_ms     = now;
            dbgPrintf("[RS485] Poll Fan (Adresse %d, Start 0, Count %d) ...\n",
                          RS485_ADDR_FAN, RS485_FAN_READ_COUNT);
            send_fc03(RS485_ADDR_FAN, 0, RS485_FAN_READ_COUNT);  // immer Start=0, siehe rs485.h
            active_target = TARGET_FAN_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (analog_polling_enabled && (now - analog_poll_ms >= LICHT_POLL_INTERVAL_MS)) {
            analog_poll_ms = now;
            send_ms        = now;
            dbgPrintln("[RS485] Poll Analog (0x50) ...");
            send_fc03(RS485_ADDR_ANALOG, RS485_REG_ANALOG_CH1, 2);
            active_target = TARGET_ANALOG_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (lueft_polling_enabled && (now - lueft_poll_ms >= FAN_POLL_INTERVAL_MS)) {
            lueft_poll_ms = now;
            send_ms       = now;
            dbgPrintf("[RS485] Poll Lueftermodul (Adresse %d) ...\n", RS485_ADDR_LUEFT);
            send_fc03(RS485_ADDR_LUEFT, RS485_LUEFT_REG_RPM_BASE, RS485_LUEFT_COUNT + 1);  // RPM×4 + Boot-ID
            active_target = TARGET_LUEFT_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (fan_polling_enabled && (now - fan_heartbeat_ms) >= FAN_HEARTBEAT_INTERVAL_MS) {
            fan_heartbeat_ms = now;
            dbgPrintln("[RS485] Fan-Heartbeat-Broadcast (Master-Erkennungsexperiment) ...");
            send_fan_heartbeat_broadcast();
            // bleibt MB_IDLE: Broadcast, keine Antwort erwartet
        }
    }
}

const Rs485SensorData& rs485_get_ms2() { return ms2; }

bool rs485_set_licht_mask(uint8_t mask) {
    licht_write_pending = true;
    licht_write_value   = mask & 0x0F;
    return true;
}

const Rs485LichtData& rs485_get_licht() { return licht; }

bool rs485_set_fan_percent(uint8_t percent) {
    fan_enable_pending = true;   // immer vor der Leistung setzen, siehe fan_enable_pending oben
    fan_write_pending  = true;
    fan_write_value    = percent > 100 ? 100 : percent;
    return true;
}

const Rs485FanData& rs485_get_fan() { return fan; }

bool rs485_set_analog_ch1(uint16_t value) {
    analog_ch1_write_pending = true;
    analog_ch1_write_value   = value > 4095 ? 4095 : value;
    return true;
}

bool rs485_set_analog_ch2(uint16_t value) {
    analog_ch2_write_pending = true;
    analog_ch2_write_value   = value > 4095 ? 4095 : value;
    return true;
}

const Rs485AnalogData& rs485_get_analog() { return analog; }

bool rs485_set_lueft_percent(uint8_t channel, uint8_t percent) {
    if (channel >= RS485_LUEFT_COUNT) return false;
    lueft_write_pending[channel] = true;
    lueft_write_value[channel]   = percent > 100 ? 100 : percent;
    return true;
}

const Rs485LueftData& rs485_get_lueft() { return lueft; }

void rs485_set_licht_polling(bool enabled)  { licht_polling_enabled  = enabled; }
void rs485_set_fan_polling(bool enabled)    { fan_polling_enabled    = enabled; }
void rs485_set_analog_polling(bool enabled) { analog_polling_enabled = enabled; }
void rs485_set_lueft_polling(bool enabled)  { lueft_polling_enabled  = enabled; }

uint8_t rs485_probe_fc03(uint8_t addr, uint16_t start, uint16_t count, uint8_t* out_buf, uint8_t max_len) {
    if (rs485_ota_busy()) return 0;   // Firmware-Update hat den Bus, nicht dazwischenfunken
    mb_state = MB_IDLE;  // evtl. laufende normale Abfrage nicht verwirren -- siehe loopRS485()

    // Bewusst OHNE drainSelfEcho(): die normale Funktion verwirft kurz nach dem Senden alles, was
    // auf dem Bus ankommt, um das eigene Sende-Echo (MAX13487) wegzuraeumen -- antwortet ein
    // Geraet aber sehr schnell, kann dabei der Anfang der echten Antwort mit verschluckt werden
    // (genau das war vermutlich die Ursache fuer abgeschnitten wirkende Scan-Treffer). Fuer die
    // Diagnose zeichnen wir deshalb wirklich ALLES auf dem Bus auf, inkl. der eigenen Anfrage --
    // laesst sich am Anfang des Puffers eindeutig erkennen (das sind die ersten 8 Bytes).
    uint8_t req[8];
    req[0] = addr;
    req[1] = 0x03;
    req[2] = start >> 8;
    req[3] = start & 0xFF;
    req[4] = count >> 8;
    req[5] = count & 0xFF;
    uint16_t c = crc16(req, 6);
    req[6] = c & 0xFF;
    req[7] = c >> 8;
    RS485Serial.write(req, 8);

    uint8_t len = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < 150) {
        while (RS485Serial.available() && len < max_len) {
            out_buf[len++] = RS485Serial.read();
        }
    }
    rx_len = 0;
    return len;
}
