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

// Muss nach WiFi.mode(WIFI_STA) und esp_wifi_set_ps(WIFI_PS_NONE) aufgerufen werden.
//
// Empfohlen: espnow_slave_begin() -- ohne serielles Setup:
//   * Node-ID, Funkkanal und Sendestufe stehen im NVS (Namespace "nodecfg": id/chan/txlvl).
//     Alte Werte (wificfg/channel) werden beim ersten Start uebernommen.
//   * Funkkanal wird automatisch gesucht: antwortet der Master auf dem gemerkten Kanal nicht,
//     reihum 1..13; der gefundene Kanal wird gemerkt. Wechselt der Router (und damit der
//     Master) den Kanal, findet der Node ihn so von selbst wieder.
//   * Die Node-ID vergibt der Master beim Koppeln (siehe NODE_CAP_ID_ASSIGN). fallback_id gilt,
//     solange keine gespeichert ist (NODE_ID_UNASSIGNED = Master schlaegt eine freie vor).
// espnow_slave_set_tx_levels() ggf. vorher aufrufen.
void          espnow_slave_begin(uint8_t node_type, uint8_t fallback_id);
// Alt: feste ID und fester Kanal, keine Kanalsuche.
void          espnow_slave_init(uint8_t node_id, uint8_t node_type, uint8_t channel);
uint8_t       espnow_slave_get_node_id();
void          espnow_slave_loop();
slave_state_t espnow_slave_get_state();
bool          espnow_slave_is_online();

// Feste Kopplung (siehe MSG_UNPAIR in espnow_protocol.h): true, wenn ein Master gespeichert ist.
bool espnow_slave_is_paired();
// Vergisst den gespeicherten Master (NVS) und meldet sich neu an -- angenommen wird danach
// wieder der erste Master, der antwortet (also eine Steuerung, bei der der Node gekoppelt ist).
void espnow_slave_unpair();

bool espnow_slave_send_data(const uint8_t* payload, uint8_t len);
bool espnow_slave_send_error(uint8_t error_code);

void espnow_slave_set_command_cb(slave_command_cb_t cb);
void espnow_slave_set_config_cb(slave_config_cb_t cb);

// Optional (EC-Meter): Bestaetigung des Masters auf send_data. seq = Sequenznummer des
// bestaetigten Pakets; die eigene liefert espnow_slave_last_seq() direkt nach dem Senden.
typedef void (*slave_data_ack_cb_t)(uint8_t seq);
void    espnow_slave_set_data_ack_cb(slave_data_ack_cb_t cb);
uint8_t espnow_slave_last_seq();

// Optional (Handgeraet ohne WLAN-Konfiguration): Funkkanal im Betrieb wechseln, z. B. um den
// Kanal des Masters zu suchen. Passt auch die gespeicherten Peers an.
void    espnow_slave_set_channel(uint8_t channel);
uint8_t espnow_slave_get_channel();

// Optional (nur mit espnow_slave_begin, vor dem Aufruf): Sendeleistungsstufen in 0,25 dBm
// (esp_wifi_set_max_tx_power). Bleibt ein ganzer Durchlauf ueber alle Kanaele ohne Antwort,
// wird die naechste Stufe probiert; die funktionierende wird gemerkt. Fuer ESP32-C3 Super
// Mini, die bei voller Leistung teils gar nicht und bei kleiner nicht weit genug funken.
// Ohne Aufruf bleibt die Sendeleistung unveraendert (Standard des Chips).
void  espnow_slave_set_tx_levels(const int8_t* levels_qdbm, uint8_t count, uint8_t default_idx);
float espnow_slave_get_tx_dbm();       // aktuelle Stufe, 0 = nicht verwaltet
bool  espnow_slave_is_hunting();       // sucht gerade den Kanal des Masters

#endif
