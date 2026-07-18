#include "rs485_slave.h"
#include <HardwareSerial.h>
#include <Arduino.h>

#define RS485_BAUD         9600
#define FRAME_TIMEOUT_US   4000   // >3.5ms Pause = Frame-Ende (bei 9600 Baud)
#define RX_BUF_SIZE        64

static HardwareSerial RS485(1);

static uint16_t regs[RS485_NUM_REGS] = {0};
static bool     request_received     = false;
static uint8_t  rx_buf[RX_BUF_SIZE];
static uint8_t  rx_len = 0;
static uint32_t last_byte_us = 0;
static rs485_slave_write_cb_t write_cb = nullptr;

static uint16_t crc16(const uint8_t* d, size_t n) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        crc ^= d[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

// Sendet und wartet danach aktiv, bis der Auto-Direction-Transceiver zurueck
// auf Empfang geschaltet hat — verwirft dabei das eigene TX-Echo, das sonst
// als Datenmuell im naechsten Frame landet.
static void send_response(const uint8_t* buf, uint8_t len) {
    RS485.write(buf, len);
    RS485.flush();
    delay(1);  // Turnaround-Reserve fuer den Transceiver
    while (RS485.available()) RS485.read();
}

static void send_exception(uint8_t func, uint8_t code) {
    Serial.printf("[RS485]   -> Exception func=0x%02X code=0x%02X\n", func, code);
    uint8_t r[5] = {RS485_SLAVE_ADDR, (uint8_t)(func | 0x80), code, 0, 0};
    uint16_t c = crc16(r, 3);
    r[3] = c & 0xFF;
    r[4] = c >> 8;
    send_response(r, 5);
}

// FC03: Read Holding Registers
static void handle_read(uint16_t start, uint16_t count) {
    if (start + count > RS485_NUM_REGS || count == 0) {
        send_exception(0x03, 0x02);  // illegal data address
        return;
    }

    uint8_t resp[3 + RS485_NUM_REGS * 2 + 2];
    resp[0] = RS485_SLAVE_ADDR;
    resp[1] = 0x03;
    resp[2] = (uint8_t)(count * 2);
    for (uint16_t i = 0; i < count; i++) {
        resp[3 + i * 2]     = regs[start + i] >> 8;
        resp[3 + i * 2 + 1] = regs[start + i] & 0xFF;
    }
    uint8_t  rlen = 3 + count * 2;
    uint16_t c    = crc16(resp, rlen);
    resp[rlen]     = c & 0xFF;
    resp[rlen + 1] = c >> 8;
    send_response(resp, rlen + 2);
}

// FC06: Write Single Register — Antwort ist der Echo der Anfrage (Modbus-Standard)
static void handle_write(uint16_t reg, uint16_t value, const uint8_t* req_frame) {
    if (reg >= RS485_NUM_REGS) {
        send_exception(0x06, 0x02);  // illegal data address
        return;
    }
    if (write_cb) write_cb(reg, value);
    send_response(req_frame, 8);
}

static void log_hex(const uint8_t* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) Serial.printf("%02X ", buf[i]);
    Serial.println();
}

static void handle_frame() {
    Serial.printf("[RS485] Rx %d Byte: ", rx_len);
    log_hex(rx_buf, rx_len);

    if (rx_len < 8) {
        Serial.println("[RS485]   -> verworfen: Frame zu kurz (< 8 Byte)");
        return;
    }

    uint16_t calc = crc16(rx_buf, rx_len - 2);
    uint16_t recv = (uint16_t)rx_buf[rx_len - 1] << 8 | rx_buf[rx_len - 2];
    if (calc != recv) {
        Serial.printf("[RS485]   -> verworfen: CRC-Fehler (berechnet=0x%04X empfangen=0x%04X)\n", calc, recv);
        return;
    }

    if (rx_buf[0] != RS485_SLAVE_ADDR) {
        Serial.printf("[RS485]   -> verworfen: fremde Adresse 0x%02X (erwartet 0x%02X)\n",
                      rx_buf[0], RS485_SLAVE_ADDR);
        return;
    }

    request_received = true;
    uint8_t  func = rx_buf[1];
    Serial.printf("[RS485]   -> gueltig: addr=0x%02X func=0x%02X\n", rx_buf[0], func);

    if (func == 0x03) {
        uint16_t start = (uint16_t)rx_buf[2] << 8 | rx_buf[3];
        uint16_t count = (uint16_t)rx_buf[4] << 8 | rx_buf[5];
        Serial.printf("[RS485]   FC03 Read: start=%u count=%u\n", start, count);
        handle_read(start, count);
    } else if (func == 0x06) {
        uint16_t reg   = (uint16_t)rx_buf[2] << 8 | rx_buf[3];
        uint16_t value = (uint16_t)rx_buf[4] << 8 | rx_buf[5];
        Serial.printf("[RS485]   FC06 Write: reg=%u value=%u\n", reg, value);
        handle_write(reg, value, rx_buf);
    } else {
        send_exception(func, 0x01);  // illegal function
    }
}

bool rs485_slave_got_request() { return request_received; }

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin) {
    RS485.begin(RS485_BAUD, SERIAL_8N1, rx_pin, tx_pin);
    Serial.printf("[RS485] Slave 0x%02X bereit (%d Baud, TX=%d RX=%d)\n",
                  RS485_SLAVE_ADDR, RS485_BAUD, tx_pin, rx_pin);
}

void rs485_slave_set_write_cb(rs485_slave_write_cb_t cb) {
    write_cb = cb;
}

void rs485_slave_loop() {
    while (RS485.available()) {
        uint8_t b = (uint8_t)RS485.read();
        if (rx_len < RX_BUF_SIZE)
            rx_buf[rx_len++] = b;
        else
            rx_len = 0;
        last_byte_us = micros();
    }

    if (rx_len > 0 && (micros() - last_byte_us) > FRAME_TIMEOUT_US) {
        handle_frame();
        rx_len = 0;
    }
}

void rs485_slave_update(uint8_t mask) {
    regs[RS485_REG_MASK] = mask;
}
