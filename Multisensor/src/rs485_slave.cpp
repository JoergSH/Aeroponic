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

static uint16_t crc16(const uint8_t* d, size_t n) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        crc ^= d[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

static void send_exception(uint8_t func, uint8_t code) {
    uint8_t r[5] = {RS485_SLAVE_ADDR, (uint8_t)(func | 0x80), code, 0, 0};
    uint16_t c = crc16(r, 3);
    r[3] = c & 0xFF;
    r[4] = c >> 8;
    RS485.write(r, 5);
}

static void handle_frame() {
    if (rx_len < 8) return;

    uint16_t calc = crc16(rx_buf, rx_len - 2);
    uint16_t recv = (uint16_t)rx_buf[rx_len - 1] << 8 | rx_buf[rx_len - 2];
    if (calc != recv) return;

    if (rx_buf[0] != RS485_SLAVE_ADDR) return;

    request_received = true;
    uint8_t  func  = rx_buf[1];
    uint16_t start = (uint16_t)rx_buf[2] << 8 | rx_buf[3];
    uint16_t count = (uint16_t)rx_buf[4] << 8 | rx_buf[5];

    if (func != 0x03) {
        send_exception(func, 0x01);  // illegal function
        return;
    }
    if (start + count > RS485_NUM_REGS || count == 0) {
        send_exception(func, 0x02);  // illegal data address
        return;
    }

    // Response: [addr][0x03][byte_count][data × 2][crc_lo][crc_hi]
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
    RS485.write(resp, rlen + 2);
}

bool rs485_slave_got_request() { return request_received; }

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin) {
    RS485.begin(RS485_BAUD, SERIAL_8N1, rx_pin, tx_pin);
    Serial.printf("[RS485] Slave 0x%02X bereit (%d Baud, TX=%d RX=%d)\n",
                  RS485_SLAVE_ADDR, RS485_BAUD, tx_pin, rx_pin);
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

void rs485_slave_update(uint16_t co2_ppm, int16_t temp_cdeg, uint16_t hum_cpct,
                        uint16_t ppfd_dppfd, const uint8_t channels[8], uint8_t gain,
                        bool scd41_ok, bool as7341_ok) {
    regs[RS485_REG_CO2]    = co2_ppm;
    regs[RS485_REG_TEMP]   = (uint16_t)((temp_cdeg < 0 ? temp_cdeg - 5 : temp_cdeg + 5) / 10);
    regs[RS485_REG_HUMI]   = (hum_cpct + 5) / 10;
    regs[RS485_REG_PPFD]   = ppfd_dppfd;
    for (int i = 0; i < 8; i++) regs[RS485_REG_CH_415 + i] = channels[i];
    regs[RS485_REG_GAIN]   = gain;
    regs[RS485_REG_STATUS] = (scd41_ok ? 0x01 : 0) | (as7341_ok ? 0x02 : 0);
}
