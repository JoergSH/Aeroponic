#include "rs485.h"
#include "pinout.h"
#include <HardwareSerial.h>
#include <Arduino.h>
#include "driver/gpio.h"

#define RS485_BAUD           9600
#define POLL_INTERVAL_MS     15000UL
#define LICHT_POLL_INTERVAL_MS 15000UL
#define FAN_POLL_INTERVAL_MS  3000UL  // Luefter-Leistung aendert sich schneller als Zeitplan/Ventile
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

static Rs485SensorData ms2   = {false, 0, 0, 0, 0, 0, {}, 0, 0};
static Rs485LichtData  licht = {false, 0, 0};
static Rs485FanData    fan   = {false, 0, 0, 0};

// ========== Zustandsmaschine ==========
// Ein Bus, ein Master, immer nur eine Anfrage gleichzeitig unterwegs.
// Reihenfolge bei freiem Bus: anstehende Schreibbefehle (Licht, dann Fan) > MS2-Poll > Licht-Poll > Fan-Poll.
// Der Luefter (Adresse 6) ist ein fremdes MARS-Hydro-Geraet, auf dessen Bus auch
// der iControl-Hub als eigener Master pollt/schreibt — siehe CLAUDE_CODE_SPEC_RS485_MARS_FAN.md.

typedef enum { MB_IDLE, MB_WAIT_RESPONSE } mb_state_t;
typedef enum { TARGET_MS2_READ, TARGET_LICHT_WRITE, TARGET_LICHT_READ,
               TARGET_FAN_WRITE, TARGET_FAN_READ } mb_target_t;

static mb_state_t  mb_state      = MB_IDLE;
static mb_target_t active_target = TARGET_MS2_READ;
static uint32_t     poll_ms       = 0;
static uint32_t     licht_poll_ms = 0;
static uint32_t     fan_poll_ms   = 0;
static uint32_t     send_ms       = 0;
static bool         first_poll    = true;

static bool    licht_write_pending = false;
static uint8_t licht_write_value   = 0;

static bool    fan_write_pending = false;
static uint8_t fan_write_value   = 0;

static uint8_t    rx_buf[RX_BUF_SIZE];
static uint8_t    rx_len       = 0;
static uint32_t   last_byte_us = 0;

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
        Serial.println("[RS485] CRC-Fehler MS2");
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

    Serial.printf("[RS485] MS2: CO2=%u ppm  T=%.1f°C  H=%.1f%%  PPFD=%.1f\n",
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
        Serial.println("[RS485] CRC-Fehler Licht");
        return;
    }

    licht.mask           = (uint8_t)((uint16_t)buf[3] << 8 | buf[4]);
    licht.online          = true;
    licht.last_update_ms = millis();
    Serial.printf("[RS485] Licht: Maske=0x%02X\n", licht.mask);
}

static void parse_licht_write_ack(const uint8_t* buf, uint8_t len) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_LICHT) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        Serial.printf("[RS485] Licht lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        licht.online = true;  // Geraet antwortet zumindest, nur der Befehl war ungueltig
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        Serial.println("[RS485] CRC-Fehler Licht-ACK");
        return;
    }
    licht.mask            = licht_write_value;
    licht.online          = true;
    licht.last_update_ms  = millis();
    Serial.printf("[RS485] Licht-ACK: Maske=0x%02X bestaetigt\n", licht.mask);
}

static void parse_fan_read(const uint8_t* buf, uint8_t len) {
    // Erwartet: [addr][0x03][byte_count=12][6 × 2 bytes][crc_lo][crc_hi]
    // Register-Reihenfolge: [0]=Reg12 (RPM) ... [5]=Reg17 (Leistung %)
    if (len < 17) return;
    if (buf[0] != RS485_ADDR_FAN || buf[1] != 0x03) return;
    uint8_t byte_count = buf[2];
    if (byte_count != RS485_FAN_READ_COUNT * 2) return;

    uint16_t calc = crc16(buf, 3 + byte_count);
    uint16_t recv = (uint16_t)buf[3 + byte_count + 1] << 8 | buf[3 + byte_count];
    if (calc != recv) {
        Serial.println("[RS485] CRC-Fehler Fan");
        return;
    }

    fan.rpm            = (uint16_t)buf[3] << 8 | buf[4];               // erstes Register = Reg12
    fan.percent         = (uint8_t)((uint16_t)buf[13] << 8 | buf[14]); // letztes Register = Reg17
    fan.online          = true;
    fan.last_update_ms  = millis();
    Serial.printf("[RS485] Fan: %u%%  %u RPM\n", fan.percent, fan.rpm);
}

static void parse_fan_write_ack(const uint8_t* buf, uint8_t len) {
    if (len < 5) return;
    if (buf[0] != RS485_ADDR_FAN) return;

    if (buf[1] == 0x86) {  // Exception: FC06 abgelehnt
        uint16_t calc = crc16(buf, 3);
        uint16_t recv = (uint16_t)buf[4] << 8 | buf[3];
        if (calc != recv) return;
        Serial.printf("[RS485] Fan lehnt Schreibbefehl ab (Code 0x%02X)\n", buf[2]);
        fan.online = true;
        return;
    }

    // Normale Antwort: Echo der Anfrage [addr][0x06][reg][value][crc]
    if (len < 8 || buf[1] != 0x06) return;
    uint16_t calc = crc16(buf, 6);
    uint16_t recv = (uint16_t)buf[7] << 8 | buf[6];
    if (calc != recv) {
        Serial.println("[RS485] CRC-Fehler Fan-ACK");
        return;
    }
    fan.percent         = fan_write_value;
    fan.online          = true;
    fan.last_update_ms  = millis();
    Serial.printf("[RS485] Fan-ACK: Leistung=%u%% bestaetigt\n", fan.percent);
}

// ========== Public ==========

void setupRS485() {
    gpio_reset_pin((gpio_num_t)RS485_RO);
    gpio_reset_pin((gpio_num_t)RS485_DI);
    RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RO, RS485_DI);
    Serial.printf("RS485 bereit (%d Baud, UART1)\n", RS485_BAUD);
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

    if (mb_state == MB_WAIT_RESPONSE) {
        bool frame_done = (rx_len > 0) && ((micros() - last_byte_us) > FRAME_TIMEOUT_US);
        bool timed_out  = (now - send_ms) > RESPONSE_TIMEOUT_MS;

        if (frame_done) {
            switch (active_target) {
                case TARGET_MS2_READ:    parse_ms2_response(rx_buf, rx_len);     break;
                case TARGET_LICHT_WRITE: parse_licht_write_ack(rx_buf, rx_len);  break;
                case TARGET_LICHT_READ:  parse_licht_read(rx_buf, rx_len);       break;
                case TARGET_FAN_WRITE:   parse_fan_write_ack(rx_buf, rx_len);    break;
                case TARGET_FAN_READ:    parse_fan_read(rx_buf, rx_len);         break;
            }
            rx_len   = 0;
            mb_state = MB_IDLE;
        } else if (timed_out) {
            switch (active_target) {
                case TARGET_MS2_READ:
                    Serial.printf("[RS485] MS2 Timeout (rx_len=%d)\n", rx_len);
                    ms2.online = false;
                    break;
                case TARGET_FAN_WRITE:
                case TARGET_FAN_READ:
                    Serial.printf("[RS485] Fan Timeout (rx_len=%d)\n", rx_len);
                    fan.online = false;
                    break;
                default:
                    Serial.printf("[RS485] Licht Timeout (rx_len=%d)\n", rx_len);
                    licht.online = false;
                    break;
            }
            rx_len   = 0;
            mb_state = MB_IDLE;
        }
    }

    if (mb_state == MB_IDLE) {
        if (licht_write_pending) {
            licht_write_pending = false;
            send_ms             = now;
            Serial.printf("[RS485] Licht-Maske schreiben: 0x%02X\n", licht_write_value);
            send_fc06(RS485_ADDR_LICHT, RS485_REG_MASK, licht_write_value);
            active_target = TARGET_LICHT_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (fan_write_pending) {
            fan_write_pending = false;
            send_ms           = now;
            Serial.printf("[RS485] Fan-Leistung schreiben: %u%%\n", fan_write_value);
            send_fc06(RS485_ADDR_FAN, RS485_FAN_REG_PERCENT, fan_write_value);
            active_target = TARGET_FAN_WRITE;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (first_poll || (now - poll_ms) >= POLL_INTERVAL_MS) {
            first_poll = false;
            poll_ms    = now;
            send_ms    = now;
            Serial.println("[RS485] Poll MS2 (0x20) ...");
            send_fc03(RS485_ADDR_MS2, 0, RS485_MS2_REGS);
            active_target = TARGET_MS2_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (now - licht_poll_ms >= LICHT_POLL_INTERVAL_MS) {
            licht_poll_ms = now;
            send_ms       = now;
            Serial.println("[RS485] Poll Licht (0x40) ...");
            send_fc03(RS485_ADDR_LICHT, RS485_REG_MASK, 1);
            active_target = TARGET_LICHT_READ;
            mb_state      = MB_WAIT_RESPONSE;
        } else if (now - fan_poll_ms >= FAN_POLL_INTERVAL_MS) {
            fan_poll_ms = now;
            send_ms     = now;
            send_fc03(RS485_ADDR_FAN, RS485_FAN_REG_RPM, RS485_FAN_READ_COUNT);
            active_target = TARGET_FAN_READ;
            mb_state      = MB_WAIT_RESPONSE;
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
    fan_write_pending = true;
    fan_write_value   = percent > 100 ? 100 : percent;
    return true;
}

const Rs485FanData& rs485_get_fan() { return fan; }
