#ifndef AS7341_HANDLER_H
#define AS7341_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t ppfd_dppfd;  // PPFD * 10 (z.B. 4523 = 452.3 µmol/m²/s)
    uint16_t ch_415nm;
    uint16_t ch_445nm;
    uint16_t ch_480nm;
    uint16_t ch_515nm;
    uint16_t ch_555nm;
    uint16_t ch_590nm;
    uint16_t ch_630nm;
    uint16_t ch_680nm;
    uint8_t  gain;        // Gain-Index (0=0.5x … 10=512x)
} as7341_data_t;

bool as7341_init();
void as7341_start();
bool as7341_update();          // aufrufen bis true zurückkommt
bool as7341_get_data(as7341_data_t* out);

#endif
