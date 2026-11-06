// LichtLogger -- Lichtausfaelle im Zelt protokollieren (ESP32-C3 Super Mini + BH1750)
//
// Misst so schnell, wie der Sensor kann (BH1750 Low-Res-Dauermodus, ca. 16-24 ms pro Wert),
// damit auch Aussetzer unter einer Sekunde auffallen. Waehrend der Lichtzeit (04:00-24:00) wird
// jede Phase unter der Dunkel-Schwelle als ein Ereignis (Beginn/Ende auf die Millisekunde,
// Dauer, kleinster Wert, Anzahl Messungen) in /dunkel.csv im internen Flash gespeichert -- zum
// Abgleich mit den dunklen Zeitraffer-Fotos. Gespeichert wird nur pro Ereignis, die Messrate
// beeinflusst die Dateigroesse also nicht. Ausserhalb der Lichtzeit und solange die Uhrzeit
// (NTP) unbekannt ist, wird nichts geloggt.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BH1750.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <time.h>
#include "local_config.h"

#define FW_VERSION "1.1.0"
#define HOSTNAME   "lichtlogger"

#define I2C_SDA 1
#define I2C_SCL 0

#define LICHT_AN_STUNDE   4     // Lichtzeit 04:00 ...
#define LICHT_AUS_STUNDE  24    // ... bis 24:00 (Mitternacht)
#define SCHWELLE_DEFAULT  200.0f   // lx -- darunter gilt "kein Licht"
#define LOG_FILE          "/dunkel.csv"
#define LOG_KOPF          "Datum,Beginn,Ende,Dauer_ms,MinLux,Messungen,Hinweis"
#define LOG_MAX_BYTES     (512UL * 1024UL)   // danach aelteste Haelfte verwerfen

#define NTP_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

#define WIFI_CONNECT_TIMEOUT_MS 12000UL
#define WIFI_RETRY_PAUSE_MS     60000UL   // nach einer erfolglosen Runde ueber alle Stufen

static WebServer   server(80);
static BH1750      bh;
static Preferences prefs;

// ========== Zustand ==========

static bool     sensorOk      = false;
static float    luxNow        = -1;
static float    schwelle      = SCHWELLE_DEFAULT;
static bool     zeitOk        = false;
static bool     startGeloggt  = false;

static bool     dunkel        = false;   // laufendes Dunkel-Ereignis
static struct timeval dunkelBeginn = {};
static uint32_t dunkelBeginnMs = 0;      // millis() fuer die Dauer
static float    dunkelMinLux  = 0;
static uint32_t dunkelMessungen = 0;
static uint32_t ereignisseHeute = 0;

static uint32_t messungenSek  = 0, messrate = 0;   // Messungen pro Sekunde (Anzeige)

static int      tagHeute      = -1;      // fuer Tages-Min/Max (nur Lichtzeit)
static float    luxMinHeute   = -1, luxMaxHeute = -1;

// ========== WLAN (Sendeleistung stufenweise, siehe Fuellstand) ==========
// ESP32-C3 Super Mini: bei voller Sendeleistung scheitert die Anmeldung oft, zu wenig reicht
// bei Abstand nicht. Niedrig anfangen, pro gescheitertem Versuch eine Stufe hoeher, die
// erfolgreiche Stufe merken. loop() wird dabei nie blockiert.

static const wifi_power_t TX_LEVELS[] = {
    WIFI_POWER_8_5dBm, WIFI_POWER_11dBm, WIFI_POWER_13dBm,
    WIFI_POWER_15dBm,  WIFI_POWER_17dBm, WIFI_POWER_19_5dBm };
static const char* TX_NAMES[] = { "8,5", "11", "13", "15", "17", "19,5" };
#define TX_LEVEL_COUNT (sizeof(TX_LEVELS) / sizeof(TX_LEVELS[0]))

static uint8_t  txLevel = 0, txTried = 0;
static bool     wifiConnecting = false, wifiPause = false, ntpGestartet = false;
static uint32_t wifiStateMs = 0;

static void startConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(TX_LEVELS[txLevel]);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setTxPower(TX_LEVELS[txLevel]);   // manche Core-Versionen setzen sie in begin() zurueck
    Serial.printf("[WiFi] Verbinde mit %s (Sendeleistung %s dBm) ...\n", WIFI_SSID, TX_NAMES[txLevel]);
    wifiConnecting = true;
    wifiStateMs    = millis();
}

static void wifiBegin() {
    WiFi.setHostname(HOSTNAME);
    prefs.begin("ll", true);
    txLevel = prefs.getUChar("txlvl", 0);
    prefs.end();
    if (txLevel >= TX_LEVEL_COUNT) txLevel = 0;
    txTried = 0;
    startConnect();
}

static void wifiLoop() {
    uint32_t now = millis();
    bool connected = WiFi.status() == WL_CONNECTED;

    if (connected && !ntpGestartet) {
        configTzTime(NTP_TZ, "pool.ntp.org", "time.google.com");   // SNTP synchronisiert danach selbst weiter
        ntpGestartet = true;
    }

    if (wifiConnecting) {
        if (connected) {
            wifiConnecting = false;
            Serial.printf("[WiFi] Verbunden: http://%s (Empfang %d dBm, Sendeleistung %s dBm)\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI(), TX_NAMES[txLevel]);
            prefs.begin("ll", false);
            if (prefs.getUChar("txlvl", 0xFF) != txLevel) prefs.putUChar("txlvl", txLevel);
            prefs.end();
        } else if (now - wifiStateMs > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.printf("[WiFi] Kein Erfolg mit %s dBm\n", TX_NAMES[txLevel]);
            WiFi.disconnect(false, false);
            if (++txTried < TX_LEVEL_COUNT) {
                txLevel = (txLevel + 1) % TX_LEVEL_COUNT;
                startConnect();
            } else {
                Serial.println("[WiFi] Alle Stufen probiert -- neuer Versuch in 60 s (Messung laeuft weiter)");
                wifiConnecting = false;
                wifiPause      = true;
                wifiStateMs    = now;
            }
        }
    } else if (wifiPause) {
        if (now - wifiStateMs > WIFI_RETRY_PAUSE_MS) {
            wifiPause = false;
            txTried   = 0;
            startConnect();
        }
    }
    // Verbunden gewesen und Verbindung weg: Auto-Reconnect des Cores uebernimmt.
}

// ========== Log-Datei ==========

static String datumText(time_t t) {
    struct tm tm; localtime_r(&t, &tm);
    char b[12]; strftime(b, sizeof(b), "%Y-%m-%d", &tm);
    return String(b);
}

static String zeitText(time_t t) {
    struct tm tm; localtime_r(&t, &tm);
    char b[10]; strftime(b, sizeof(b), "%H:%M:%S", &tm);
    return String(b);
}

static String zeitTextMs(const struct timeval& tv) {
    char b[8]; snprintf(b, sizeof(b), ".%03ld", (long)(tv.tv_usec / 1000));
    return zeitText(tv.tv_sec) + b;
}

// Haelt die Datei unter LOG_MAX_BYTES: aelteste Haelfte (zeilenweise) verwerfen.
static void logKuerzen() {
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f || f.size() < LOG_MAX_BYTES) { if (f) f.close(); return; }
    String kopf = f.readStringUntil('\n');
    f.seek(f.size() / 2);
    f.readStringUntil('\n');   // angeschnittene Zeile verwerfen
    File neu = LittleFS.open("/dunkel.tmp", "w");
    neu.println(kopf);
    uint8_t buf[256];
    while (f.available()) { size_t n = f.read(buf, sizeof(buf)); neu.write(buf, n); }
    f.close(); neu.close();
    LittleFS.remove(LOG_FILE);
    LittleFS.rename("/dunkel.tmp", LOG_FILE);
}

static void logZeile(const String& zeile) {
    bool neu = !LittleFS.exists(LOG_FILE);
    File f = LittleFS.open(LOG_FILE, "a");
    if (!f) { Serial.println("[LOG] Datei nicht schreibbar"); return; }
    if (neu) f.println(LOG_KOPF);
    f.println(zeile);
    f.close();
    Serial.printf("[LOG] %s\n", zeile.c_str());
    logKuerzen();
}

static void dunkelEnde(const struct timeval& ende, const char* hinweis) {
    if (!dunkel) return;
    dunkel = false;
    uint32_t dauerMs = millis() - dunkelBeginnMs;
    logZeile(datumText(dunkelBeginn.tv_sec) + "," + zeitTextMs(dunkelBeginn) + "," + zeitTextMs(ende) + "," +
             String(dauerMs) + "," + String(dunkelMinLux, 1) + "," + String(dunkelMessungen) + "," + hinweis);
}

// ========== Messung ==========

static bool inLichtzeit(const struct tm& tm) {
    return tm.tm_hour >= LICHT_AN_STUNDE && tm.tm_hour < LICHT_AUS_STUNDE;
}

// Low-Res-Dauermodus: neuer Wert alle ca. 16 ms (max. 24 ms), Aufloesung ca. 4 lx -- fuer
// "Licht an/aus" mehr als genug und deutlich schneller als High-Res (120-180 ms).
static void sensorStart() {
    sensorOk = bh.begin(BH1750::CONTINUOUS_LOW_RES_MODE, 0x23, &Wire);
    Serial.println(sensorOk ? "[BH1750] gefunden" : "[BH1750] nicht gefunden (SDA=1, SCL=0, Adresse 0x23?)");
}

// Aus loop(): liest jeden neuen Sensorwert, sobald er fertig ist.
static void messen() {
    static uint32_t lastRetry = 0;
    if (!sensorOk) {
        if (millis() - lastRetry > 10000) { lastRetry = millis(); sensorStart(); }
        luxNow = -1;
        return;
    }
    if (!bh.measurementReady(true)) return;
    float lux = bh.readLightLevel();
    if (lux < 0) {   // I2C-Fehler
        Serial.println("[BH1750] Lesefehler");
        sensorOk = false;
        luxNow   = -1;
        return;
    }
    luxNow = lux;
    messungenSek++;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    struct tm tm; localtime_r(&now, &tm);
    zeitOk = tm.tm_year + 1900 >= 2025;
    if (!zeitOk) return;

    if (!startGeloggt) {
        startGeloggt = true;
        logZeile(datumText(now) + "," + zeitText(now) + ",,,,," + "Logger gestartet (FW " FW_VERSION ")");
    }

    if (tm.tm_mday != tagHeute) {
        tagHeute = tm.tm_mday;
        luxMinHeute = luxMaxHeute = -1;
        ereignisseHeute = 0;
    }

    if (!inLichtzeit(tm)) {
        dunkelEnde(tv, "Ende Lichtzeit");
        return;
    }

    if (luxMinHeute < 0 || lux < luxMinHeute) luxMinHeute = lux;
    if (lux > luxMaxHeute) luxMaxHeute = lux;

    if (lux < schwelle) {
        if (!dunkel) {
            dunkel          = true;
            dunkelBeginn    = tv;
            dunkelBeginnMs  = millis();
            dunkelMinLux    = lux;
            dunkelMessungen = 0;
            ereignisseHeute++;
            Serial.printf("[LICHT] Dunkel ab %s (%.1f lx)\n", zeitTextMs(tv).c_str(), lux);
        } else if (lux < dunkelMinLux) {
            dunkelMinLux = lux;
        }
        dunkelMessungen++;
    } else if (dunkel) {
        dunkelEnde(tv, "");
    }
}

// ========== Webseite ==========

static const char PAGE[] PROGMEM = R"html(<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Licht-Logger</title>
<style>
body{font-family:sans-serif;margin:0;padding:16px;background:#f4f6f8;color:#222}
.box{background:#fff;border-radius:8px;padding:14px;margin-bottom:14px;box-shadow:0 1px 3px #0002}
.big{font-size:2.4em;font-weight:bold}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:8px}
.lbl{font-size:.8em;color:#777}
table{border-collapse:collapse;width:100%;font-size:.9em}
td,th{border-bottom:1px solid #eee;padding:4px 6px;text-align:left}
button{padding:8px 16px;border:0;border-radius:5px;color:#fff;cursor:pointer;margin:2px}
input{padding:6px;width:90px}
</style></head><body>
<h2>💡 Licht-Logger</h2>
<div class="box">
 <div class="lbl">Aktuell</div>
 <div class="big"><span id="lux">--</span> lx</div>
 <div id="state" style="font-weight:bold;margin-top:4px"></div>
</div>
<div class="box grid">
 <div><div class="lbl">Uhrzeit</div><div id="zeit">--</div></div>
 <div><div class="lbl">Heute min / max (Lichtzeit)</div><div id="minmax">--</div></div>
 <div><div class="lbl">Dunkel-Ereignisse heute</div><div id="anz">--</div></div>
 <div><div class="lbl">WLAN</div><div id="wlan">--</div></div>
 <div><div class="lbl">Messungen / s</div><div id="rate">--</div></div>
 <div><div class="lbl">Laufzeit</div><div id="up">--</div></div>
 <div><div class="lbl">Firmware</div><div id="fw">--</div></div>
</div>
<div class="box">
 <div class="lbl">Dunkel-Schwelle (Lichtzeit 04:00–24:00, Dauermessung)</div>
 <input type="number" id="schwelle" min="1" step="1"> lx
 <button style="background:#3498db" onclick="saveSchwelle()">Speichern</button>
 <span id="schwSt" class="lbl"></span>
</div>
<div class="box">
 <button style="background:#27ae60" onclick="location.href='/download'">⬇ CSV herunterladen</button>
 <button style="background:#dc3545" onclick="clearLog()">🗑 Log löschen</button>
 <h3 style="margin:12px 0 6px">Letzte Einträge</h3>
 <table><thead><tr><th>Datum</th><th>Beginn</th><th>Ende</th><th>Dauer ms</th><th>Min lx</th><th>Messungen</th><th>Hinweis</th></tr></thead>
 <tbody id="log"></tbody></table>
</div>
<script>
let schwelleGeladen=false;
function upd(){fetch('/api/status').then(r=>r.json()).then(d=>{
 document.getElementById('lux').textContent=d.lux<0?'--':d.lux.toFixed(1);
 let st=!d.sensor?'⚠️ Sensor nicht erreichbar':!d.zeit_ok?'⏳ Uhrzeit noch unbekannt (WLAN/NTP) – es wird nicht geloggt'
   :!d.lichtzeit?'🌙 außerhalb der Lichtzeit – kein Logging':d.dunkel?'🔴 DUNKEL seit '+d.dunkel_seit:'🟢 Licht an';
 document.getElementById('state').textContent=st;
 document.getElementById('zeit').textContent=d.zeit||'--';
 document.getElementById('minmax').textContent=d.min<0?'--':d.min.toFixed(0)+' / '+d.max.toFixed(0)+' lx';
 document.getElementById('anz').textContent=d.ereignisse;
 document.getElementById('wlan').textContent=d.rssi?d.rssi+' dBm':'nicht verbunden';
 document.getElementById('rate').textContent=d.rate;
 document.getElementById('up').textContent=Math.floor(d.uptime/3600)+' h '+Math.floor(d.uptime%3600/60)+' min';
 document.getElementById('fw').textContent=d.fw;
 if(!schwelleGeladen){document.getElementById('schwelle').value=d.schwelle;schwelleGeladen=true;}
}).catch(()=>{});}
function loadLog(){fetch('/api/log').then(r=>r.text()).then(t=>{
 const rows=t.trim().split('\n').slice(1).reverse().slice(0,100);
 document.getElementById('log').innerHTML=rows.map(r=>'<tr>'+r.split(',').map(c=>'<td>'+c+'</td>').join('')+'</tr>').join('');
}).catch(()=>{});}
function saveSchwelle(){const v=parseFloat(document.getElementById('schwelle').value);
 fetch('/api/schwelle',{method:'POST',body:JSON.stringify({schwelle:v})}).then(r=>r.json())
 .then(d=>{document.getElementById('schwSt').textContent=d.ok?'gespeichert':'ungültig';});}
function clearLog(){if(!confirm('Log wirklich löschen?'))return;fetch('/api/clear',{method:'POST'}).then(loadLog);}
upd();loadLog();setInterval(upd,1000);setInterval(loadLog,10000);
</script></body></html>
)html";

static void handleStatus() {
    time_t now = time(nullptr);
    struct tm tm; localtime_r(&now, &tm);
    String j = "{";
    j += "\"lux\":" + String(luxNow, 1);
    j += ",\"sensor\":" + String(sensorOk ? "true" : "false");
    j += ",\"zeit_ok\":" + String(zeitOk ? "true" : "false");
    j += ",\"zeit\":\"" + (zeitOk ? datumText(now) + " " + zeitText(now) : String("")) + "\"";
    j += ",\"lichtzeit\":" + String(zeitOk && inLichtzeit(tm) ? "true" : "false");
    j += ",\"dunkel\":" + String(dunkel ? "true" : "false");
    j += ",\"dunkel_seit\":\"" + (dunkel ? zeitTextMs(dunkelBeginn) : String("")) + "\"";
    j += ",\"rate\":" + String(messrate);
    j += ",\"schwelle\":" + String(schwelle, 0);
    j += ",\"min\":" + String(luxMinHeute, 1) + ",\"max\":" + String(luxMaxHeute, 1);
    j += ",\"ereignisse\":" + String(ereignisseHeute);
    j += ",\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
    j += ",\"uptime\":" + String(millis() / 1000);
    j += ",\"fw\":\"" FW_VERSION "\"}";
    server.send(200, "application/json", j);
}

static void sendLogFile(bool alsDownload) {
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f) { server.send(200, "text/plain", LOG_KOPF "\n"); return; }
    if (alsDownload) server.sendHeader("Content-Disposition", "attachment; filename=\"lichtlogger_dunkel.csv\"");
    server.streamFile(f, alsDownload ? "text/csv" : "text/plain");
    f.close();
}

static void handleSchwelle() {
    String body = server.arg("plain");
    int i = body.indexOf(':');
    float v = i >= 0 ? body.substring(i + 1).toFloat() : 0;
    if (v < 1 || v > 100000) { server.send(200, "application/json", "{\"ok\":false}"); return; }
    schwelle = v;
    prefs.begin("ll", false);
    prefs.putFloat("schwelle", schwelle);
    prefs.end();
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleClear() {
    LittleFS.remove(LOG_FILE);
    startGeloggt = false;   // neue Datei beginnt wieder mit "Logger gestartet"
    server.send(200, "application/json", "{\"ok\":true}");
}

// ========== Setup / Loop ==========

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.printf("\n[MAIN] LichtLogger %s startet...\n", FW_VERSION);

    prefs.begin("ll", true);
    schwelle = prefs.getFloat("schwelle", SCHWELLE_DEFAULT);
    prefs.end();

    if (!LittleFS.begin(true)) Serial.println("[FS] LittleFS-Fehler");

    Wire.begin(I2C_SDA, I2C_SCL);
    sensorStart();

    wifiBegin();

    server.on("/",            [] { server.send_P(200, "text/html", PAGE); });
    server.on("/api/status",  handleStatus);
    server.on("/api/log",     [] { sendLogFile(false); });
    server.on("/download",    [] { sendLogFile(true); });
    server.on("/api/schwelle", HTTP_POST, handleSchwelle);
    server.on("/api/clear",    HTTP_POST, handleClear);
    server.begin();
}

void loop() {
    messen();

    static uint32_t lastRate = 0;
    if (millis() - lastRate >= 1000) {
        lastRate     = millis();
        messrate     = messungenSek;
        messungenSek = 0;
    }

    wifiLoop();
    server.handleClient();
    delay(1);   // WLAN-Task Luft lassen; ein Messwert dauert ohnehin >= 16 ms
}
