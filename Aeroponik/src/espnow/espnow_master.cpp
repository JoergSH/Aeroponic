#include "espnow_master.h"
#include "espnow_protocol.h"
#include "dbg.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <freertos/queue.h>
#include <Preferences.h>
#include <string.h>

// ========== Interner Zustand ==========

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

espnow_node_t espnow_nodes[ESPNOW_MAX_NODES];

static uint8_t            master_seq     = 0;
static QueueHandle_t      recv_queue     = nullptr;
static espnow_sensor_cb_t sensor_cb      = nullptr;

// Queue-Element: MAC + Paket
typedef struct {
    uint8_t         mac[6];
    espnow_packet_t pkt;
} recv_evt_t;

// ========== Hilfsfunktionen ==========

static espnow_node_t* find_by_mac(const uint8_t* mac) {
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (espnow_nodes[i].registered &&
            memcmp(espnow_nodes[i].mac, mac, 6) == 0)
            return &espnow_nodes[i];
    }
    return nullptr;
}

static espnow_node_t* find_by_id(uint8_t id) {
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (espnow_nodes[i].registered && espnow_nodes[i].node_id == id)
            return &espnow_nodes[i];
    }
    return nullptr;
}

static espnow_node_t* free_slot() {
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (!espnow_nodes[i].registered) return &espnow_nodes[i];
    }
    return nullptr;
}

static bool send_pkt(const uint8_t* mac, espnow_packet_t* pkt) {
    pkt->crc8 = crc8((uint8_t*)pkt, sizeof(espnow_packet_t) - 1);
    return esp_now_send(mac, (uint8_t*)pkt, sizeof(espnow_packet_t)) == ESP_OK;
}

// ========== Callbacks (WiFi-Task — kein Serial, kein Blocking) ==========

static void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#ifdef ESPNOW_DEBUG
    ets_printf("[RECV] von %02X:%02X:%02X:%02X:%02X:%02X  len=%d (erwartet=%d)\n",
               info->src_addr[0], info->src_addr[1], info->src_addr[2],
               info->src_addr[3], info->src_addr[4], info->src_addr[5],
               len, (int)sizeof(espnow_packet_t));
    if (len >= 4)
        ets_printf("[RECV] Bytes: %02X %02X %02X %02X ...\n",
                   data[0], data[1], data[2], data[3]);
#endif
    if (len != sizeof(espnow_packet_t) || !recv_queue) return;
    recv_evt_t evt;
    memcpy(evt.mac, info->src_addr, 6);
    memcpy(&evt.pkt, data, sizeof(espnow_packet_t));
    xQueueSendFromISR(recv_queue, &evt, nullptr);
}

static void on_sent(const esp_now_send_info_t* info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_FAIL) {
        espnow_node_t* node = find_by_mac(info->des_addr);
        if (node) {
            if (++node->retry_count >= 3)
                node->online = false;
        }
    }
}

// ========== Feste Kopplung (siehe MSG_UNPAIR in espnow_protocol.h) ==========
// Gekoppelte Nodes stehen dauerhaft im NVS und werden beim Start sofort in espnow_nodes[]
// eingetragen (offline, bis sie sich melden). Unbekannte Nodes bekommen keine Antwort,
// sondern landen nur in espnow_discovered[] und werden erst nach "Koppeln" im Webinterface
// angenommen -- es gibt bewusst kein automatisches Koppeln, auch nicht beim ersten Start.

#define PAIR_NVS_NS        "espnow_pair"
#define PAIR_NVS_KEY       "nodes"
#define DISCOVERED_MAX_AGE_MS (5UL * 60000UL)   // so lange ohne neue Meldung -> aus der Liste

typedef struct {
    uint8_t mac[6];
    uint8_t node_id;
    uint8_t node_type;
} __attribute__((packed)) paired_entry_t;

espnow_discovered_t espnow_discovered[ESPNOW_MAX_DISCOVERED];

static void add_peer(const uint8_t* mac) {
    if (esp_now_is_peer_exist(mac)) return;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;   // folgt dem aktuellen WiFi-Kanal
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}

static void save_paired() {
    paired_entry_t list[ESPNOW_MAX_NODES];
    size_t n = 0;
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (!espnow_nodes[i].registered) continue;
        memcpy(list[n].mac, espnow_nodes[i].mac, 6);
        list[n].node_id   = espnow_nodes[i].node_id;
        list[n].node_type = espnow_nodes[i].node_type;
        n++;
    }
    Preferences p;
    p.begin(PAIR_NVS_NS, false);
    if (n) p.putBytes(PAIR_NVS_KEY, list, n * sizeof(paired_entry_t));
    else   p.remove(PAIR_NVS_KEY);
    p.end();
}

static void load_paired() {
    Preferences p;
    p.begin(PAIR_NVS_NS, true);
    paired_entry_t list[ESPNOW_MAX_NODES];
    size_t len = p.isKey(PAIR_NVS_KEY) ? p.getBytes(PAIR_NVS_KEY, list, sizeof(list)) : 0;
    p.end();

    size_t n = len / sizeof(paired_entry_t);
    for (size_t i = 0; i < n && i < ESPNOW_MAX_NODES; i++) {
        espnow_node_t& node = espnow_nodes[i];
        memcpy(node.mac, list[i].mac, 6);
        node.node_id    = list[i].node_id;
        node.node_type  = list[i].node_type;
        node.registered = true;
        node.online     = false;
        add_peer(node.mac);
        dbgPrintf("[ESP-NOW] Gekoppelt: Node %d Typ=0x%02X MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
                  node.node_id, node.node_type, node.mac[0], node.mac[1], node.mac[2],
                  node.mac[3], node.mac[4], node.mac[5]);
    }
}

static espnow_node_t* pair_node(const uint8_t* mac, uint8_t node_id, uint8_t node_type) {
    espnow_node_t* node = free_slot();
    if (!node) return nullptr;
    memset(node, 0, sizeof(*node));
    memcpy(node->mac, mac, 6);
    node->node_id    = node_id;
    node->node_type  = node_type;
    node->registered = true;
    add_peer(mac);
    for (int i = 0; i < ESPNOW_MAX_DISCOVERED; i++)
        if (espnow_discovered[i].valid && memcmp(espnow_discovered[i].mac, mac, 6) == 0)
            espnow_discovered[i].valid = false;
    save_paired();
    dbgPrintf("[ESP-NOW] Node %d gekoppelt Typ=0x%02X MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
              node_id, node_type, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return node;
}

// Unbekannter Node hat sich gemeldet: nur merken (fuer "Neu gefunden" im Webinterface),
// nicht antworten.
static void note_discovered(const uint8_t* mac, uint8_t node_id, uint8_t node_type) {
    espnow_discovered_t* slot = nullptr;
    for (int i = 0; i < ESPNOW_MAX_DISCOVERED; i++)
        if (espnow_discovered[i].valid && memcmp(espnow_discovered[i].mac, mac, 6) == 0) { slot = &espnow_discovered[i]; break; }
    if (!slot) {
        for (int i = 0; i < ESPNOW_MAX_DISCOVERED; i++)
            if (!espnow_discovered[i].valid) { slot = &espnow_discovered[i]; break; }
        if (!slot) return;
        dbgPrintf("[ESP-NOW] Unbekannter Node %d (Typ=0x%02X MAC=%02X:%02X:%02X:%02X:%02X:%02X) -- nicht gekoppelt, ignoriert\n",
                  node_id, node_type, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    memcpy(slot->mac, mac, 6);
    slot->node_id      = node_id;
    slot->node_type    = node_type;
    slot->last_seen_ms = millis();
    slot->valid        = true;
}

// Liefert den gekoppelten Node zu dieser MAC, sonst nullptr (Node nur als "neu gefunden" vermerkt).
static espnow_node_t* paired_or_discover(const uint8_t* mac, uint8_t node_id, uint8_t node_type) {
    espnow_node_t* node = find_by_mac(mac);
    if (node) return node;
    note_discovered(mac, node_id, node_type);
    return nullptr;
}

// ========== Paketverarbeitung (loop-Kontext) ==========

static void handle_register(const uint8_t* mac, const espnow_packet_t& pkt) {
    espnow_node_t* node = paired_or_discover(mac, pkt.node_id, pkt.payload_len > 0 ? pkt.payload[0] : 0);
    if (!node) return;
    if (pkt.payload_len > 0 && pkt.payload[0] != node->node_type) {
        node->node_type = pkt.payload[0];   // z. B. Steckdose per NODECLEAR auf Licht-SSR umgestellt
        save_paired();
    }
    node->online = true;
    node->last_heartbeat_ms = millis();
    node->retry_count = 0;

    espnow_packet_t ack = {};
    ack.node_id     = 0;
    ack.msg_type    = MSG_REGISTER_ACK;
    ack.seq         = master_seq++;
    ack.payload_len = 1;
    ack.payload[0]  = pkt.node_id;
    send_pkt(mac, &ack);
}

static void handle_heartbeat(const uint8_t* mac, const espnow_packet_t& pkt) {
    espnow_node_t* node = paired_or_discover(mac, pkt.node_id, pkt.payload_len > 0 ? pkt.payload[0] : NODE_TYPE_SENSOR);
    if (!node) return;
    node->online = true;
    node->last_heartbeat_ms = millis();
    node->retry_count = 0;

    espnow_packet_t ack = {};
    ack.node_id     = 0;
    ack.msg_type    = MSG_HEARTBEAT_ACK;
    ack.seq         = pkt.seq;
    ack.payload_len = 0;
    send_pkt(mac, &ack);
}

static void handle_sensor_data(const uint8_t* mac, const espnow_packet_t& pkt) {
    // Typ aus Daten-Subtyp ableiten (nur fuer die Umstellung/"Neu gefunden" relevant)
    uint8_t inferred = NODE_TYPE_SENSOR;
    if (pkt.payload_len > 0) {
        if (pkt.payload[0] == SUBTYPE_RELAY_STATUS)    inferred = NODE_TYPE_SOCKET;
        else if (pkt.payload[0] == SUBTYPE_PWM_STATUS) inferred = NODE_TYPE_LIGHT;
    }
    espnow_node_t* node = paired_or_discover(mac, pkt.node_id, inferred);
    if (!node) return;

    espnow_packet_t ack = {};
    ack.node_id     = 0;
    ack.msg_type    = MSG_DATA_ACK;
    ack.seq         = pkt.seq;
    ack.payload_len = 0;
    send_pkt(mac, &ack);

#ifdef ESPNOW_DEBUG
    dbgPrintf("[ESP-NOW] Node %d Sensor (Typ=0x%02X len=%d)\n",
                  pkt.node_id, pkt.payload_len > 0 ? pkt.payload[0] : 0, pkt.payload_len);

    if (pkt.payload_len >= 1) {
        uint8_t sensor_type = pkt.payload[0];

        if ((sensor_type == 0x01 || sensor_type == 0x03) && pkt.payload_len >= 7) {
            int16_t  temp_raw = (int16_t)((pkt.payload[1] << 8) | pkt.payload[2]);
            uint16_t hum_raw  = (uint16_t)((pkt.payload[3] << 8) | pkt.payload[4]);
            uint16_t co2      = (uint16_t)((pkt.payload[5] << 8) | pkt.payload[6]);
            dbgPrintf("  Luft:  %.2f C  %.2f%% rH  %d ppm CO2\n",
                          temp_raw / 100.0f, hum_raw / 100.0f, co2);
        }
        if (sensor_type == 0x03 && pkt.payload_len >= 9) {
            uint16_t ppfd_raw = (uint16_t)((pkt.payload[7] << 8) | pkt.payload[8]);
            dbgPrintf("  Licht: %.1f umol/m2/s PPFD\n", ppfd_raw / 10.0f);
        } else if (sensor_type == 0x02 && pkt.payload_len >= 3) {
            uint16_t ppfd_raw = (uint16_t)((pkt.payload[1] << 8) | pkt.payload[2]);
            dbgPrintf("  Licht: %.1f umol/m2/s PPFD\n", ppfd_raw / 10.0f);
        }
        if (sensor_type < 0x01 || sensor_type > 0x03) {
            dbgPrint("  Raw: ");
            for (int i = 0; i < pkt.payload_len; i++)
                dbgPrintf("%02X ", pkt.payload[i]);
            dbgPrintln();
        }
    }
#endif

    if (sensor_cb)
        sensor_cb(pkt.node_id, node->node_type, pkt.payload, pkt.payload_len);
}

static void process(const recv_evt_t& evt) {
    const espnow_packet_t& pkt = evt.pkt;
    if (pkt.crc8 != crc8((uint8_t*)&pkt, sizeof(espnow_packet_t) - 1)) {
        dbgPrintf("[ESP-NOW] CRC-Fehler Node %d\n", pkt.node_id);
        return;
    }
    switch (pkt.msg_type) {
        case MSG_REGISTER:    handle_register(evt.mac, pkt);    break;
        case MSG_HEARTBEAT:   handle_heartbeat(evt.mac, pkt);   break;
        case MSG_SENSOR_DATA: handle_sensor_data(evt.mac, pkt); break;
        case MSG_REGISTER_ACK:
        case MSG_HEARTBEAT_ACK:
        case MSG_DATA_ACK:
        case MSG_UNPAIR:
            break;   // Antworten/Befehle einer anderen Steuerung in Funkreichweite
        case MSG_COMMAND_ACK:
        case MSG_CONFIG_ACK: {
            espnow_node_t* node = find_by_mac(evt.mac);
            if (node) node->retry_count = 0;
            break;
        }
        case MSG_ERROR:
            dbgPrintf("[ESP-NOW] Fehler von Node %d: 0x%02X\n",
                          pkt.node_id, pkt.payload_len ? pkt.payload[0] : 0xFF);
            break;
        default:
            break;
    }
}

// ========== Öffentliche API ==========

void setupEspNow() {
    memset(espnow_nodes, 0, sizeof(espnow_nodes));
    recv_queue = xQueueCreate(8, sizeof(recv_evt_t));

    esp_err_t r;

    r = esp_now_init();
    dbgPrintf("[ESP-NOW] esp_now_init: %s\n", esp_err_to_name(r));
    if (r != ESP_OK) return;

    // Power-Saving ausschalten — sonst schläft der Master während DTIM-Intervallen
    // und verpasst ESP-NOW Broadcasts von Slaves
    esp_wifi_set_ps(WIFI_PS_NONE);

    r = esp_now_register_recv_cb(on_recv);
    dbgPrintf("[ESP-NOW] register_recv_cb: %s\n", esp_err_to_name(r));

    r = esp_now_register_send_cb(on_sent);
    dbgPrintf("[ESP-NOW] register_send_cb: %s\n", esp_err_to_name(r));

    // Broadcast-Peer für Register-ACKs (channel=0 → folgt aktuellem WiFi-Kanal)
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    r = esp_now_add_peer(&peer);
    dbgPrintf("[ESP-NOW] add_peer(broadcast): %s\n", esp_err_to_name(r));

    memset(espnow_discovered, 0, sizeof(espnow_discovered));
    load_paired();

    uint8_t primary;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&primary, &second);
    dbgPrintf("[ESP-NOW] Bereit (WiFi-Kanal %d, max %d Nodes)\n", primary, ESPNOW_MAX_NODES);
    dbgPrintf("[ESP-NOW] sizeof(espnow_packet_t)=%d\n", (int)sizeof(espnow_packet_t));
}

void loopEspNow() {
    // Empfangs-Queue abarbeiten
    recv_evt_t evt;
    while (xQueueReceive(recv_queue, &evt, 0) == pdTRUE)
        process(evt);

    // Heartbeat-Watchdog alle 1 s
    static unsigned long lastWatchdog = 0;
    unsigned long now = millis();
    if (now - lastWatchdog >= 1000) {
        lastWatchdog = now;
        for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
            espnow_node_t& n = espnow_nodes[i];
            if (n.registered && n.online &&
                now - n.last_heartbeat_ms > ESPNOW_HEARTBEAT_TIMEOUT_MS) {
                n.online = false;
                dbgPrintf("[ESP-NOW] Node %d Timeout (offline)\n", n.node_id);
            }
        }
        for (int i = 0; i < ESPNOW_MAX_DISCOVERED; i++)
            if (espnow_discovered[i].valid && now - espnow_discovered[i].last_seen_ms > DISCOVERED_MAX_AGE_MS)
                espnow_discovered[i].valid = false;
    }
}

const char* espnow_pair(const uint8_t* mac) {
    espnow_discovered_t* d = nullptr;
    for (int i = 0; i < ESPNOW_MAX_DISCOVERED; i++)
        if (espnow_discovered[i].valid && memcmp(espnow_discovered[i].mac, mac, 6) == 0) { d = &espnow_discovered[i]; break; }
    if (!d)                    return "Node nicht (mehr) in der Liste";
    if (find_by_mac(mac))      return "Node ist bereits gekoppelt";
    if (find_by_id(d->node_id)) return "Node-ID ist schon an einen anderen Node vergeben (am Node per NODECLEAR aendern)";
    if (!pair_node(mac, d->node_id, d->node_type)) return "Knotenliste voll";
    return nullptr;   // Node meldet sich mit dem naechsten REGISTER (alle 3-30 s) und bekommt dann ein ACK
}

int espnow_unpair(uint8_t node_id) {
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        espnow_node_t& n = espnow_nodes[i];
        if (!n.registered || n.node_id != node_id) continue;
        espnow_packet_t pkt = {};
        pkt.node_id     = node_id;
        pkt.msg_type    = MSG_UNPAIR;
        pkt.seq         = master_seq++;
        pkt.payload_len = 0;
        send_pkt(n.mac, &pkt);   // erreicht ihn das nicht, ignoriert er uns trotzdem nicht -- dann am Node BOOT 3 s halten
        dbgPrintf("[ESP-NOW] Node %d entkoppelt\n", node_id);
        // Peer nicht sofort loeschen: das UNPAIR-Paket ist eventuell noch in der Sendewarteschlange
        memset(&n, 0, sizeof(n));
        save_paired();
        return i;
    }
    return -1;
}

void espnow_set_sensor_callback(espnow_sensor_cb_t cb) {
    sensor_cb = cb;
}

bool espnow_send_pwm(uint8_t node_id, uint16_t ch1_pm, uint16_t ch2_pm) {
    espnow_node_t* node = find_by_id(node_id);
    if (!node || !node->online) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = node_id;
    pkt.msg_type    = MSG_COMMAND;
    pkt.seq         = master_seq++;
    pkt.payload_len = 5;
    pkt.payload[0]  = CMD_SET_PWM;
    pkt.payload[1]  = (ch1_pm >> 8) & 0xFF;
    pkt.payload[2]  =  ch1_pm       & 0xFF;
    pkt.payload[3]  = (ch2_pm >> 8) & 0xFF;
    pkt.payload[4]  =  ch2_pm       & 0xFF;
    return send_pkt(node->mac, &pkt);
}

bool espnow_send_onoff(uint8_t node_id, bool on) {
    espnow_node_t* node = find_by_id(node_id);
    if (!node || !node->online) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = node_id;
    pkt.msg_type    = MSG_COMMAND;
    pkt.seq         = master_seq++;
    pkt.payload_len = 2;
    pkt.payload[0]  = CMD_SET_ONOFF;
    pkt.payload[1]  = on ? 1 : 0;
    return send_pkt(node->mac, &pkt);
}

bool espnow_send_relay_mask(uint8_t node_id, uint8_t mask) {
    espnow_node_t* node = find_by_id(node_id);
    if (!node || !node->online) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = node_id;
    pkt.msg_type    = MSG_COMMAND;
    pkt.seq         = master_seq++;
    pkt.payload_len = 2;
    pkt.payload[0]  = CMD_SET_ONOFF;
    pkt.payload[1]  = mask;
    return send_pkt(node->mac, &pkt);
}

bool espnow_send_config(uint8_t node_id, uint8_t key, uint32_t value) {
    espnow_node_t* node = find_by_id(node_id);
    if (!node || !node->online) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = node_id;
    pkt.msg_type    = MSG_CONFIG;
    pkt.seq         = master_seq++;
    pkt.payload_len = 5;
    pkt.payload[0]  = key;
    pkt.payload[1]  = (value >> 24) & 0xFF;
    pkt.payload[2]  = (value >> 16) & 0xFF;
    pkt.payload[3]  = (value >>  8) & 0xFF;
    pkt.payload[4]  =  value        & 0xFF;
    return send_pkt(node->mac, &pkt);
}

bool espnow_node_online(uint8_t node_id) {
    espnow_node_t* node = find_by_id(node_id);
    return node && node->online;
}

bool espnow_trigger_ota(uint8_t node_id, const char* ap_pass) {
    espnow_node_t* node = find_by_id(node_id);
    if (!node) return false;
    espnow_packet_t pkt = {};
    pkt.node_id     = node_id;
    pkt.msg_type    = MSG_OTA_START;
    pkt.seq         = master_seq++;
    pkt.payload_len = 8;
    memcpy(pkt.payload, ap_pass, 8);
    dbgPrintf("[ESP-NOW] OTA-Befehl an Node %d\n", node_id);
    return send_pkt(node->mac, &pkt);
}
