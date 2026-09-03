#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>
#include <stdbool.h>

// Dieses Board beantwortet auf DEMSELBEN RS485-Bus bis zu drei Modbus-Adressen, je
// nachdem welche I2C-Peripherie beim Boot erkannt wurde. Der Master (Aeroponik) kennt
// alle drei Adressen schon unabhaengig voneinander (Lueftermodul 0x51, Multisensor2
// 0x20, Analog-Ausgang 0x50) und toleriert laengst, dass eines der Geraete fehlt/offline
// ist (Timeout -> online=false) -- dadurch sind am Master keine Code-Aenderungen noetig.
#define RS485_ADDR_LUEFT   0x51   // immer aktiv (Luefter-PWM/Tacho, kein I2C noetig)
#define RS485_ADDR_MS2     0x20   // aktiv, wenn SCD41 beim Boot gefunden wurde
#define RS485_ADDR_ANALOG  0x50   // aktiv, wenn GP8403 beim Boot gefunden wurde

#define RS485_LUEFT_NUM_REGS    8   // 0-3 PWM% (FC06 schreiben/FC03 lesen), 4-7 RPM (read-only)
#define RS485_MS2_NUM_REGS      14  // CO2,Temp,Humi,PPFD,8xKanal,Gain,Status — read-only (FC03)
#define RS485_ANALOG_NUM_REGS   2   // Ch1/Ch2 0-4095 — FC06 schreiben / FC03 lesen

// Register-Layout MS2 — identisch zum Multisensor (RS485_ADDR_MS2), damit die vom
// Master schon vorhandene Parse-Logik unveraendert funktioniert.
#define RS485_MS2_REG_CO2    0
#define RS485_MS2_REG_TEMP   1   // °C × 10
#define RS485_MS2_REG_HUMI   2   // % × 10
#define RS485_MS2_REG_PPFD   3   // µmol/m²/s × 10
#define RS485_MS2_REG_CH0    4   // 415nm .. Reg 11 = 680nm
#define RS485_MS2_REG_GAIN   12
#define RS485_MS2_REG_STATUS 13  // bit0=SCD41 ok, bit1=AS7341 ok

typedef void (*rs485_slave_write_cb_t)(uint16_t reg, uint16_t value);

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();

// Lueftermodul-Register (0x51): Schreib-Callback (PWM-Sollwert) + Ist-Wert spiegeln (RPM)
void rs485_slave_set_lueft_write_cb(rs485_slave_write_cb_t cb);
void rs485_slave_update_lueft(uint16_t reg, uint16_t value);

// Analog-Register (0x50): Schreib-Callback (GP8403-Kanal setzen) + Ist-Wert spiegeln.
// Nur aktiv (antwortet ueberhaupt), wenn rs485_slave_set_analog_active(true) gesetzt wurde.
void rs485_slave_set_analog_write_cb(rs485_slave_write_cb_t cb);
void rs485_slave_update_analog(uint16_t reg, uint16_t value);
void rs485_slave_set_analog_active(bool active);

// MS2-Register (0x20): nur lesbar, Firmware schreibt die aktuellen Sensordaten hinein.
// Nur aktiv (antwortet ueberhaupt), wenn rs485_slave_set_ms2_active(true) gesetzt wurde.
void rs485_slave_update_ms2(uint16_t reg, uint16_t value);
void rs485_slave_set_ms2_active(bool active);

#endif
