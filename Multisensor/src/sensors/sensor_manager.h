#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Subtypen für MSG_SENSOR_DATA payload[0]
#define SUBTYPE_AIR    0x01
#define SUBTYPE_LIGHT  0x02

// Befehls-Subtypen für MSG_COMMAND payload[0] (Multisensor-spezifisch)
#define CMD_SET_INTERVAL    0x10  // payload[1..2] = Intervall in Sekunden
#define CMD_REQUEST_DATA    0x11  // sofortige Messung
#define CMD_REQUEST_RAWSPEC 0x12  // nächste Messung sendet Rohkanäle

typedef struct {
    int16_t  temperature_cdeg;  // °C * 100 (SCD41)
    uint16_t humidity_cpct;     // % * 100  (SCD41)
    uint16_t co2_ppm;           // CO2 ppm  (SCD41, echter NDIR-Wert)
    uint16_t ppfd_dppfd;        // PPFD * 10
    uint16_t ch_415nm;
    uint16_t ch_445nm;
    uint16_t ch_480nm;
    uint16_t ch_515nm;
    uint16_t ch_555nm;
    uint16_t ch_590nm;
    uint16_t ch_630nm;
    uint16_t ch_680nm;
    uint8_t  as7341_gain;
} sensor_data_t;

typedef enum {
    SM_INIT_FAILED = -1,
    SM_OK          = 0,
} sm_init_result_t;

sm_init_result_t sensor_manager_init(uint8_t sda_pin, uint8_t scl_pin);
void             sensor_manager_loop();
void             sensor_manager_on_command(const uint8_t* payload, uint8_t len);
bool             sensor_manager_data_ready();
void             sensor_manager_get_data(sensor_data_t* out);
bool             sensor_manager_send_raw_next();

#endif
