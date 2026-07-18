#include "espnow_slave.h"
#include "espnow_protocol.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <string.h>

#define REGISTER_RETRY_MS    3000
#define HEARTBEAT_INTERVAL_MS 5000
#define REGISTER_MAX_RETRIES 20

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t       my_node_id;
static uint8_t       my_node_type;
static uint8_t       my_channel;
static uint8_t       master_mac[6];
static bool          master_known    = false;
static slave_state_t state           = SLAVE_STATE_REGISTERING;
static uint8_t       slave_seq       = 0;
static uint8_t       reg_retries     = 0;
static uint32_t      last_register_ms  = 0;
static uint32_t      last_heartbeat_ms = 0;

// ========== recv_queue — einfaches Flag statt FreeRTOS-Queue ==========
// Wir speichern nur das letzte empfangene Paket; reicht für Register-ACK

static volatile bool recv_pending = false;
static uint8_t       recv_mac[6];
static espnow_packet_t recv_pkt;

static slave_command_cb_t command_cb  = nullptr;
static slave_config_cb_t  config_cb   = nullptr;
static volatile bool      ota_pending = false;
static char               ota_ap_pass[9] = {};

// ========== OTA ==========

static void do_ota_update() {
    Serial.printf("[OTA] Verbinde mit '%s'...\n", OTA_AP_SSID);
    esp_now_deinit();
    WiFi.begin(OTA_AP_SSID, ota_ap_pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi-Verbindung fehlgeschlagen");
        delay(1000); ESP.restart(); return;
    }
    Serial.printf("[OTA] Verbunden: %s\n", WiFi.localIP().toString().c_str());
    WiFiClient client;
    t_httpUpdate_return ret = httpUpdate.update(client, "http://" OTA_MASTER_IP "/ota.bin");
    switch (ret) {
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Erfolgreich"); delay(500); ESP.restart(); break;
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Fehler %d: %s\n",
                          httpUpdate.getLastError(),
                          httpUpdate.getLastErrorString().c_str());
            delay(2000); ESP.restart(); break;
        default:
            Serial.println("[OTA] Keine Updates"); delay(1000); ESP.restart();
    }
}

// ========== Helpers ==========

static bool send_pkt_to(const uint8_t* mac, espnow_packet_t* pkt) {
    pkt->crc8 = crc8((uint8_t*)pkt, sizeof(espnow_packet_t) - 1);
#ifdef ESPNOW_DEBUG
    ets_printf("[TX] node_id=%02X msg=%02X seq=%d payload_len=%d crc=%02X\n",
               pkt->node_id, pkt->msg_type, pkt->seq, pkt->payload_len, pkt->crc8);
    ets_printf("[TX] Roh: %02X %02X %02X %02X %02X ...\n",
               ((uint8_t*)pkt)[0], ((uint8_t*)pkt)[1], ((uint8_t*)pkt)[2],
               ((uint8_t*)pkt)[3], ((uint8_t*)pkt)[4]);
    ets_printf("[TX] Ziel: %02X:%02X:%02X:%02X:%02X:%02X  len=%d\n",
               mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
               (int)sizeof(espnow_packet_t));
#endif
    esp_err_t r = esp_now_send(mac, (uint8_t*)pkt, sizeof(espnow_packet_t));
#ifdef ESPNOW_DEBUG
    ets_printf("[TX] esp_now_send -> %s\n", esp_err_to_name(r));
#endif
    return r == ESP_OK;
}

// ========== Callbacks (WiFi-Task) ==========

static void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#ifdef ESPNOW_DEBUG
    ets_printf("[RX] von %02X:%02X:%02X:%02X:%02X:%02X len=%d\n",
               info->src_addr[0], info->src_addr[1], info->src_addr[2],
               info->src_addr[3], info->src_addr[4], info->src_addr[5], len);
    if (len >= 4)
        ets_printf("[RX] Bytes: %02X %02X %02X %02X\n",
                   data[0], data[1], data[2], data[3]);
#endif
    if (len != sizeof(espnow_packet_t)) return;
    if (recv_pending) return;  // vorheriges Paket noch nicht verarbeitet

    memcpy(recv_mac, info->src_addr, 6);
    memcpy(&recv_pkt, data, sizeof(espnow_packet_t));
    recv_pending = true;
}

static void on_sent(const esp_now_send_info_t* info, esp_now_send_status_t status) {
    (void)info;
#ifdef ESPNOW_DEBUG
    ets_printf("[SENT] %s\n",
               status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
#endif
}

// ========== Paketverarbeitung (loop-Kontext) ==========

static void handle_register_ack(const espnow_packet_t& pkt) {
    if (pkt.payload_len < 1) {
        Serial.println("[SLAVE] REGISTER_ACK: payload_len=0, ignoriert");
        return;
    }
    if (pkt.payload[0] != my_node_id) {
        Serial.printf("[SLAVE] REGISTER_ACK: fuer node_id=0x%02X (meins=0x%02X), ignoriert\n",
                      pkt.payload[0], my_node_id);
        return;
    }

    if (!master_known) {
        memcpy(master_mac, recv_mac, 6);
        master_known = true;

        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, recv_mac, 6);
        peer.channel = my_channel;
        peer.encrypt = false;
        esp_now_add_peer(&peer);

        Serial.printf("[SLAVE] *** Registriert bei Master %02X:%02X:%02X:%02X:%02X:%02X ***\n",
                      recv_mac[0], recv_mac[1], recv_mac[2],
                      recv_mac[3], recv_mac[4], recv_mac[5]);
    }
    state       = SLAVE_STATE_ONLINE;
    reg_retries = 0;
}

static void process_recv() {
    const espnow_packet_t& pkt = recv_pkt;

    uint8_t expected_crc = crc8((uint8_t*)&pkt, sizeof(espnow_packet_t) - 1);
    if (pkt.crc8 != expected_crc) {
        Serial.printf("[SLAVE] CRC-Fehler: empfangen=%02X erwartet=%02X\n",
                      pkt.crc8, expected_crc);
        return;
    }

#ifdef ESPNOW_DEBUG
    Serial.printf("[SLAVE] Empfangen: msg=%02X seq=%d payload_len=%d\n",
                  pkt.msg_type, pkt.seq, pkt.payload_len);
#endif

    switch (pkt.msg_type) {
        case MSG_REGISTER_ACK:
            handle_register_ack(pkt);
            break;
        case MSG_HEARTBEAT_ACK:
            break;
        case MSG_DATA_ACK:
            break;  // Master-ACK auf send_data
        case MSG_REGISTER:
            break;  // Broadcasts anderer Slaves
        case MSG_COMMAND:
            if (command_cb) command_cb(pkt.payload, pkt.payload_len);
            break;
        case MSG_CONFIG:
            if (config_cb) config_cb(pkt.payload, pkt.payload_len);
            break;
        case MSG_OTA_START:
            if (pkt.payload_len >= 8) {
                memcpy(ota_ap_pass, pkt.payload, 8);
                ota_ap_pass[8] = '\0';
                ota_pending = true;
                Serial.println("[OTA] Befehl empfangen");
            }
            break;
        default:
            Serial.printf("[SLAVE] Unbekannter msg_type=0x%02X\n", pkt.msg_type);
            break;
    }
}

// ========== Oeffentliche API ==========

void espnow_slave_init(uint8_t node_id, uint8_t node_type, uint8_t channel) {
    my_node_id   = node_id;
    my_node_type = node_type;
    my_channel   = channel;
    state        = SLAVE_STATE_REGISTERING;
    reg_retries  = 0;
    master_known = false;
    slave_seq    = 0;
    recv_pending = false;

    Serial.printf("[SLAVE] Init: node_id=0x%02X type=0x%02X channel=%d\n",
                  node_id, node_type, channel);
    Serial.printf("[SLAVE] sizeof(espnow_packet_t)=%d\n", (int)sizeof(espnow_packet_t));

    esp_err_t r;

    r = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    Serial.printf("[SLAVE] esp_wifi_set_channel(%d): %s\n", channel, esp_err_to_name(r));

    uint8_t actual_ch = 0;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&actual_ch, &second);
    Serial.printf("[SLAVE] Kanal nach set_channel: %d\n", actual_ch);

    r = esp_now_init();
    Serial.printf("[SLAVE] esp_now_init: %s\n", esp_err_to_name(r));
    if (r != ESP_OK) {
        Serial.println("[SLAVE] FEHLER: esp_now_init fehlgeschlagen!");
        state = SLAVE_STATE_ERROR;
        return;
    }

    r = esp_now_register_recv_cb(on_recv);
    Serial.printf("[SLAVE] register_recv_cb: %s\n", esp_err_to_name(r));

    r = esp_now_register_send_cb(on_sent);
    Serial.printf("[SLAVE] register_send_cb: %s\n", esp_err_to_name(r));

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = channel;
    peer.encrypt = false;
    r = esp_now_add_peer(&peer);
    Serial.printf("[SLAVE] add_peer(broadcast ch=%d): %s\n", channel, esp_err_to_name(r));

    esp_wifi_get_channel(&actual_ch, &second);
    Serial.printf("[SLAVE] Kanal nach esp_now_init: %d\n", actual_ch);

    Serial.println("[SLAVE] Init abgeschlossen, sende ersten REGISTER...");
}

void espnow_slave_loop() {
    if (recv_pending) {
        process_recv();
        recv_pending = false;
    }

    if (ota_pending) {
        ota_pending = false;
        do_ota_update();
        return;
    }

    uint32_t now = millis();

    if (state == SLAVE_STATE_REGISTERING) {
        if (now - last_register_ms >= REGISTER_RETRY_MS) {
            last_register_ms = now;

            if (reg_retries >= REGISTER_MAX_RETRIES) {
                Serial.println("[SLAVE] Max Versuche erreicht — gebe auf");
                state = SLAVE_STATE_ERROR;
                return;
            }

            Serial.printf("[SLAVE] REGISTER Versuch %d/%d\n",
                          reg_retries + 1, REGISTER_MAX_RETRIES);

#ifdef ESPNOW_DEBUG
            uint8_t my_mac[6];
            esp_wifi_get_mac(WIFI_IF_STA, my_mac);
            uint8_t actual_ch = 0;
            wifi_second_chan_t second;
            esp_wifi_get_channel(&actual_ch, &second);
            Serial.printf("[SLAVE] Kanal aktiv: %d  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          actual_ch,
                          my_mac[0],my_mac[1],my_mac[2],
                          my_mac[3],my_mac[4],my_mac[5]);
#endif

            espnow_packet_t pkt = {};
            pkt.node_id     = my_node_id;
            pkt.msg_type    = MSG_REGISTER;
            pkt.seq         = slave_seq++;
            pkt.payload_len = 1;
            pkt.payload[0]  = my_node_type;
            send_pkt_to(BROADCAST_MAC, &pkt);

            reg_retries++;
        }
    } else if (state == SLAVE_STATE_ONLINE) {
        if (now - last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS) {
            last_heartbeat_ms = now;

            espnow_packet_t pkt = {};
            pkt.node_id     = my_node_id;
            pkt.msg_type    = MSG_HEARTBEAT;
            pkt.seq         = slave_seq++;
            pkt.payload_len = 1;
            pkt.payload[0]  = my_node_type;
            send_pkt_to(master_mac, &pkt);
        }
    }
}

slave_state_t espnow_slave_get_state() { return state; }
bool          espnow_slave_is_online()  { return state == SLAVE_STATE_ONLINE; }

bool espnow_slave_send_data(const uint8_t* payload, uint8_t len) {
    if (!master_known || state != SLAVE_STATE_ONLINE) return false;
    if (len > 16) len = 16;
    espnow_packet_t pkt = {};
    pkt.node_id     = my_node_id;
    pkt.msg_type    = MSG_SENSOR_DATA;
    pkt.seq         = slave_seq++;
    pkt.payload_len = len;
    memcpy(pkt.payload, payload, len);
    return send_pkt_to(master_mac, &pkt);
}

bool espnow_slave_send_error(uint8_t error_code) {
    if (!master_known) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = my_node_id;
    pkt.msg_type    = MSG_ERROR;
    pkt.seq         = slave_seq++;
    pkt.payload_len = 1;
    pkt.payload[0]  = error_code;
    return send_pkt_to(master_mac, &pkt);
}

void espnow_slave_set_command_cb(slave_command_cb_t cb) { command_cb = cb; }
void espnow_slave_set_config_cb(slave_config_cb_t cb)   { config_cb  = cb; }
