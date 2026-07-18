#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include <stdint.h>

// Adress-Raum: 0x01-0x0E = Mars Hydro Original, 0x20+ = eigene Geraete
#define RS485_SLAVE_ADDR   0x20  // Multisensor2

#define RS485_REG_CO2      0
#define RS485_REG_TEMP     1   // °C × 10
#define RS485_REG_HUMI     2   // % × 10
#define RS485_REG_PPFD     3   // µmol/m²/s × 10
#define RS485_REG_CH_415   4
#define RS485_REG_CH_445   5
#define RS485_REG_CH_480   6
#define RS485_REG_CH_515   7
#define RS485_REG_CH_555   8
#define RS485_REG_CH_590   9
#define RS485_REG_CH_630   10
#define RS485_REG_CH_680   11
#define RS485_REG_GAIN     12
#define RS485_REG_STATUS   13  // bit0=SCD41 ok, bit1=AS7341 ok
#define RS485_NUM_REGS     14

void rs485_slave_init(uint8_t tx_pin, uint8_t rx_pin);
void rs485_slave_loop();
void rs485_slave_update(uint16_t co2_ppm, int16_t temp_cdeg, uint16_t hum_cpct,
                        uint16_t ppfd_dppfd, const uint8_t channels[8], uint8_t gain,
                        bool scd41_ok, bool as7341_ok);
bool rs485_slave_got_request();

#endif
