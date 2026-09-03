#ifndef SCD41_HANDLER_H
#define SCD41_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

bool scd41_init();
bool scd41_data_ready();
bool scd41_read(int16_t* temp_cdeg, uint16_t* hum_cpct, uint16_t* co2_ppm);
bool scd41_has_error();
void scd41_reinit();

#endif
