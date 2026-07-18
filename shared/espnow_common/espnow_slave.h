#ifndef ESPNOW_SLAVE_H
#define ESPNOW_SLAVE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SLAVE_STATE_REGISTERING = 0,
    SLAVE_STATE_ONLINE,
    SLAVE_STATE_ERROR
} slave_state_t;

typedef void (*slave_command_cb_t)(const uint8_t* payload, uint8_t len);
typedef void (*slave_config_cb_t)(const uint8_t* payload, uint8_t len);

// Muss nach WiFi.mode(WIFI_STA) und esp_wifi_set_ps(WIFI_PS_NONE) aufgerufen werden
void          espnow_slave_init(uint8_t node_id, uint8_t node_type, uint8_t channel);
void          espnow_slave_loop();
slave_state_t espnow_slave_get_state();
bool          espnow_slave_is_online();

bool espnow_slave_send_data(const uint8_t* payload, uint8_t len);
bool espnow_slave_send_error(uint8_t error_code);

void espnow_slave_set_command_cb(slave_command_cb_t cb);
void espnow_slave_set_config_cb(slave_config_cb_t cb);

#endif
