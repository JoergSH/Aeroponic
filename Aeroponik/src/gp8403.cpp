#include "gp8403.h"
#include "pinout.h"
#include <Wire.h>

// Registerkarte (DFRobot GP8403):
//   0x01: Ausgangsbereich (0x00 = 0-5V, 0x11 = 0-10V, beide Kanaele)
//   0x02: DAC Kanal 0 — 12-bit-Wert linksbuendig (value << 4), 2 Bytes LSB-first
//   0x04: DAC Kanal 1 — dito
#define GP8403_REG_RANGE   0x01
#define GP8403_RANGE_10V   0x11
#define GP8403_REG_CH0     0x02
#define GP8403_REG_CH1     0x04

static bool gp8403_ok = false;

static bool write_reg(uint8_t reg, const uint8_t* data, uint8_t len) {
    Wire.beginTransmission(GP8403_ADDR);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++) Wire.write(data[i]);
    gp8403_ok = (Wire.endTransmission() == 0);
    return gp8403_ok;
}

void gp8403_init() {
    uint8_t range = GP8403_RANGE_10V;
    write_reg(GP8403_REG_RANGE, &range, 1);  // 0-10V-Bereich fuer beide Kanaele
}

bool gp8403_set_channel(uint8_t channel, uint16_t value_0_4095) {
    if (value_0_4095 > 4095) value_0_4095 = 4095;
    uint16_t d = (uint16_t)(value_0_4095 << 4);   // linksbuendig ins 16-bit-Feld
    uint8_t  buf[2] = { (uint8_t)(d & 0xFF), (uint8_t)(d >> 8) };
    return write_reg(channel == 0 ? GP8403_REG_CH0 : GP8403_REG_CH1, buf, 2);
}

bool gp8403_online() { return gp8403_ok; }
