#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stdbool.h>

// Adress-Raum RS485 Bus
// 0x01-0x0E  Mars Hydro Original (MH-AE=2, iHub4=5, iHub10=13, ...)
// 0x20-0x3F  Eigene Sensoren
// 0x40-0x5F  Eigene Aktoren (reserviert)

#define RS485_ADDR_MS2      0x20   // Multisensor2
#define RS485_MS2_REGS      14     // CO2, Temp, Humi, PPFD, 8×Kanal, Gain, Status

#define RS485_ADDR_LICHT    0x40   // Lichtx4 (4× Relais)
#define RS485_REG_MASK      0      // Relaismaske, Bit0-3 = Relais 1-4

#define RS485_ADDR_FAN       6     // MARS Hydro iControl Abluftluefter (fremdes Geraet, feste Adresse)
#define RS485_FAN_REG_RPM    11    // 0-basiert = Register 12 (1-basiert), Ist-Drehzahl
#define RS485_FAN_REG_PERCENT 16   // 0-basiert = Register 17 (1-basiert), Leistung 0-100%
#define RS485_FAN_READ_COUNT 6     // deckt Register 12-17 (0-basiert 11-16) in einem Read ab

struct Rs485SensorData {
    bool     online;
    uint32_t last_update_ms;
    uint16_t co2_ppm;
    float    temp_c;
    float    hum_pct;
    float    ppfd;
    uint8_t  channels[8];
    uint8_t  gain;
    uint8_t  status;
};

struct Rs485LichtData {
    bool     online;
    uint8_t  mask;
    uint32_t last_update_ms;
};

struct Rs485FanData {
    bool     online;
    uint16_t rpm;
    uint8_t  percent;
    uint32_t last_update_ms;
};

void setupRS485();
void loopRS485();

const Rs485SensorData& rs485_get_ms2();

// Setzt die Relaismaske am Lichtx4 (nicht-blockierend, wird beim naechsten
// freien Bus-Slot per Modbus FC06 gesendet).
bool rs485_set_licht_mask(uint8_t mask);
const Rs485LichtData& rs485_get_licht();

// Setzt die Luefterleistung (0-100%) am MARS-Hydro-Luefter (Adresse 6, FC06
// Register 0x10). Nicht-blockierend, siehe rs485_set_licht_mask().
bool rs485_set_fan_percent(uint8_t percent);
const Rs485FanData& rs485_get_fan();

#endif
