#ifndef ESPNOW_MASTER_H
#define ESPNOW_MASTER_H

// Einkommentieren oder -D ESPNOW_DEBUG in platformio.ini setzen für Debug-Ausgaben
// #define ESPNOW_DEBUG

#include <stdint.h>
#include <stdbool.h>
#include "espnow_protocol.h"

#define ESPNOW_MAX_NODES            16
#define ESPNOW_HEARTBEAT_TIMEOUT_MS 15000   // 3× Heartbeat-Intervall

typedef struct {
    uint8_t  node_id;
    uint8_t  node_type;
    uint8_t  mac[6];
    bool     registered;
    bool     online;
    uint32_t last_heartbeat_ms;
    uint8_t  last_seq;
    uint8_t  retry_count;
} espnow_node_t;

extern espnow_node_t espnow_nodes[ESPNOW_MAX_NODES];

// Callback für eingehende Sensordaten (in loop()-Kontext, Serial erlaubt)
typedef void (*espnow_sensor_cb_t)(uint8_t node_id, uint8_t node_type,
                                   const uint8_t* payload, uint8_t len);

void setupEspNow();
void loopEspNow();

void espnow_set_sensor_callback(espnow_sensor_cb_t cb);

bool espnow_send_pwm(uint8_t node_id, uint16_t ch1_pm, uint16_t ch2_pm);
bool espnow_send_onoff(uint8_t node_id, bool on);
bool espnow_send_relay_mask(uint8_t node_id, uint8_t mask);
bool espnow_send_config(uint8_t node_id, uint8_t key, uint32_t value);
bool espnow_node_online(uint8_t node_id);

// OTA: sendet MSG_OTA_START mit AP-Passwort an den Node (8 Zeichen, null-terminiert)
bool espnow_trigger_ota(uint8_t node_id, const char* ap_pass);

#endif
