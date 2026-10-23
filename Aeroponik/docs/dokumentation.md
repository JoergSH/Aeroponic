# Aeroponik Steuerung – Systemdokumentation

**[Deutsch](#deutsch) | [English](#english)**

---

## Deutsch

Systemdokumentation — Master (ESP32-S3) mit RS485-Geräten und drahtlosen Sensor-/Aktor-Nodes via ESP-NOW.

### Inhaltsverzeichnis

**System**
- [Übersicht](#übersicht)
- [ESP-NOW-Protokoll](#esp-now-protokoll)

**Hauptsteuerung**
- [Hardware & Pinout](#hardware--pinout)
- [Ventile & Pumpen](#ventile--pumpen)
- [Funktionen](#funktionen)
- [Beleuchtungs-Scheduler](#beleuchtungs-scheduler)
- [RS485-Bus](#rs485-bus)
- [RS485-Multisensor Zelt](#rs485-multisensor-zelt)
- [Lichtx4](#lichtx4)
- [Abluftlüfter](#abluftlüfter)
- [Lueftermodul](#lueftermodul)
- [Multimodul](#multimodul)
- [Analog-Ausgangsmodul](#analog-ausgangsmodul)
- [CO2-Steuerung](#co2-steuerung)
- [Luftbefeuchtung-Steuerung](#luftbefeuchtung-steuerung)
- [Raumlicht & DWC-Timer](#raumlicht--dwc-timer)
- [Durchflussmesser-Kalibrierung & Düsen-Überwachung](#durchflussmesser-kalibrierung--düsen-überwachung)
- [Raumklima & VPD](#raumklima--vpd)
- [Daten-Log & Download](#daten-log--download)

**Nodes (ESP-NOW)**
- [Multisensor](#multisensor)
- [Steckdosen](#steckdosen)
- [Erststart & Konfiguration](#erststart--konfiguration)

**Referenz**
- [Serial-Kommandos](#serial-kommandos)

---

### Übersicht

```
Aeroponik Master (ESP32-S3 N16R8)
 │  W5500 Ethernet (LAN + WiFi parallel) · Magnetventile MV1–MV4 (direkte GPIO) · DS3231 RTC (I2C)
 │  MicroSD Datenlogging · RS485 UART1 (MAX13487) · DS18B20 (Vorrat/Pflanze/Innen) · Drucksensor 0,5–4,5V
 │  Tank-Füllstand: Ultraschall ODER HX710B-Drucksensor (umschaltbar) · Durchflussmesser (Düsen-Überwachung)
 │  Netzspannungsüberwachung (USV) · Webinterface Port 80
 │
 ├── RS485 (115200 Baud) ⇄
 │    ├─ Multisensor              0x20 · FC03           Zelt-Klima (CO₂/Temp/Feuchte) + PPFD/Spektrum (AS7341)
 │    ├─ Lichtx4                  0x40 · FC03/FC06      4× Relais-Lichtsteuerung
 │    ├─ Abluftlüfter (MARS Hydro) Adr. 6 · FC03/FC06   Drehzahl/Leistung, manuell oder automatisch
 │    ├─ Analog-Ausgangsmodul     0x50 · FC03/FC06      2× 0–10V (Licht-/Lüfter-Alternative)
 │    └─ Lueftermodul / Multimodul 0x51 (+ optional 0x20/0x50) · FC03/FC06   4× Zeltlüfter PWM/Tacho
 │
 └── ESP-NOW (Kanal 6) ⇄
      └─ Steckdosen              NODE_TYPE_SOCKET · 0x03   4× Relais, manuell/API (u. a. CO2-Ausgang)
```

Die RS485-Geräte sind fest verdrahtet und werden zyklisch abgefragt — siehe [RS485-Bus](#rs485-bus). ESP-NOW-Nodes registrieren sich dagegen beim Start beim Master und senden periodische Heartbeats; der Master erkennt ihren Online-/Offline-Status automatisch.

LAN (W5500 via SPI2) und WiFi laufen gleichzeitig. Das Webinterface auf Port 80 ist auf beiden Interfaces ohne Codeänderungen erreichbar.

### ESP-NOW-Protokoll

#### Paketformat

```c
typedef struct {
    uint8_t  node_id;      // 1–254  |  0 = Master  |  255 = Broadcast
    uint8_t  msg_type;     // Nachrichtentyp
    uint8_t  seq;          // Sequenznummer
    uint8_t  payload_len;  // 0–16
    uint8_t  payload[16];
    uint8_t  crc8;         // Polynom 0x07
} espnow_packet_t;         // 21 Byte
```

#### Node-Typen

| Konstante | Wert | Beschreibung |
|---|---|---|
| `NODE_TYPE_LIGHT` | `0x01` | LED-Lichtsteuerung (PWM, Licht-Meter) |
| `NODE_TYPE_SENSOR` | `0x02` | Multisensor — Luft + Spektrum |
| `NODE_TYPE_SOCKET` | `0x03` | Steckdose — allgemeine manuelle Steuerung |
| `NODE_TYPE_LIGHT_SSR` | `0x05` | Licht-SSR — automatisch per Beleuchtungs-Scheduler (Legacy, siehe [Steckdosen](#steckdosen)) |

> Der frühere separate ESP-NOW-Spektrum-Node (`NODE_TYPE_SPEKTRUM 0x04`, Subtyp `SUBTYPE_LICHT_SUMMARY 0x20`) wurde entfernt — PPFD/Spektrum kommen jetzt über RS485 (`0x20`, Multisensor bzw. Multimodul).

#### Nachrichtentypen

| Typ | Wert | Richtung | Bedeutung |
|---|---|---|---|
| `MSG_REGISTER` | `0x01` | Node → Master | Node meldet sich an |
| `MSG_REGISTER_ACK` | `0x02` | Master → Node | Anmeldung bestätigt |
| `MSG_HEARTBEAT` | `0x03` | Node → Master | Keep-Alive / Online-Check |
| `MSG_HEARTBEAT_ACK` | `0x04` | Master → Node | Heartbeat bestätigt |
| `MSG_SENSOR_DATA` | `0x10` | Node → Master | Messdaten (Subtyp in payload[0]) |
| `MSG_COMMAND` | `0x20` | Master → Node | Steuerbefehl |
| `MSG_OTA_START` | `0x40` | Master → Node | OTA-Update (AP-Passwort in payload[0..7]) |
| `MSG_OTA_ACK` | `0x41` | Node → Master | OTA wird gestartet |

#### Befehls-Subtypen (MSG_COMMAND · payload[0])

| Subtyp | Wert | payload[1+] |
|---|---|---|
| `CMD_SET_PWM` | `0x01` | uint16 BE — PWM-Wert 0–1000 (= 0–100,0 %) |
| `CMD_SET_ONOFF` | `0x02` | Bitmaske: Bit0=R1 Bit1=R2 Bit2=R3 Bit3=R4 |
| `CMD_SET_INTERVAL` | `0x10` | uint16 — Messintervall in Sekunden |
| `CMD_REQUEST_DATA` | `0x11` | – (löst sofortige Messung aus) |
| `CMD_REQUEST_RAWSPEC` | `0x12` | – (nächste Messung sendet Rohkanäle) |

#### Sensor-Daten-Subtypen (MSG_SENSOR_DATA · payload[0])

| Subtyp | Wert | payload-Inhalt |
|---|---|---|
| `SUBTYPE_AIR` | `0x01` | [1..2] Temp×100 int16BE · [3..4] RH×100 uint16BE · [5..6] eCO₂ ppm · [7..8] TVOC ppb |
| `SUBTYPE_LIGHT` | `0x02` | [1..2] PPFD×10 uint16BE · [3..16] Spektralkanäle 415–680 nm |
| `SUBTYPE_RELAY_STATUS` | `0x10` | [1] Relais-Bitmaske (Bit0–3 = R1–R4) |

---

### Hardware & Pinout

**MCU:** ESP32-S3 N16R8 (16 MB Flash, 8 MB OPI-PSRAM)

> **Gesperrte GPIOs:** 0, 45, 46 (Strapping, boot-kritisch) · 19, 20 (USB-OTG) · 26–32 (Flash) · 33–37 (OPI-PSRAM) · 38–42 (JTAG) · 43, 44 (UART0). GPIO3 ist zwar ebenfalls ein Strapping-Pin, dessen Rolle (JTAG-Signalquelle) aber nur greift, wenn ein bestimmtes eFuse-Bit gesetzt ist (Standard: nein) — wird deshalb bewusst für den Durchflussmesser genutzt (siehe unten), statt des bootkritischeren GPIO46.

> **Hardware-Revision:** Ab der SMD-Platine (siehe [`SmdVersion_Pinbelegung.pdf`](../../DokuEtc/SmdVersion_Pinbelegung.pdf)) gibt es keinen I2C-GPIO-Expander (PCF8574AP) mehr — Ventile, Pumpe und Umwälzpumpe hängen an direkten GPIOs, und AHT21B teilt sich den Hauptbus mit der RTC statt eines eigenen zweiten I2C-Busses.

| GPIO | Konstante | Peripherie |
|---|---|---|
| `1` | `RS485_RX` | UART1 RX ← MAX13487 Receive Out |
| `2` | `RS485_TX` | UART1 TX → MAX13487 Data In |
| `3` | `FLOW_SENSOR_PIN` | Durchflussmesser (YF-S401-ähnlich, Hall-Impulsgeber, push-pull) — siehe [Durchflussmesser-Kalibrierung & Düsen-Überwachung](#durchflussmesser-kalibrierung--düsen-überwachung) |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (Vorrat, Pflanze, optional Innentemperatur), Zuordnung über ROM-Adresse (siehe [Funktionen](#funktionen)) |
| `5` | `PRESSURE_PIN` | Analog — Drucksensor 0,5–4,5 V ratiometrisch (per Web-UI abschaltbar, siehe [Drucksensor](#drucksensor-pumpendruck)) |
| `6` | `GEHAEUSE_FAN_PIN` | Gehäuselüfter, An/Aus (kein PWM), temperaturgeregelt über Innen-DS18B20 |
| `7` | `VENTIL2_PIN` | Magnetventil Behälter 2 (direkte GPIO) |
| `8` | `VENTIL1_PIN` | Magnetventil Behälter 1 |
| `9` | `VENTIL4_PIN` | Rücklaufventil |
| `10` | `VENTIL3_PIN` | Magnetventil Behälter 3 |
| `11` | `SPI_MOSI` | SPI2 MOSI — W5500 + MicroSD |
| `12` | `SPI_SCK` | SPI2 SCK |
| `13` | `SPI_MISO` | SPI2 MISO |
| `14` | `SPI_CS_W5500` | W5500 Chip Select |
| `15` | `ULTRASONIC_TRIG` | HC-SR04 Trigger |
| `16` | `ULTRASONIC_ECHO` | HC-SR04 Echo (3,3 V direkt anschließbar) |
| `17` | `I2C_SCL` | I2C-Bus: DS3231, AHT21B, optional ENS160/GP8403 |
| `18` | `I2C_SDA` | I2C-Bus: DS3231, AHT21B, optional ENS160/GP8403 |
| `21` | `WSINT` | W5500 Interrupt |
| `38` | `NETZ_OK_PIN` | Netzspannungsüberwachung — HIGH = Netz vorhanden, LOW = USV-Betrieb (2000 ms entprellt) |
| `39` | `SPI_CS_SD` | MicroSD Chip Select |
| `47` | `UMWAELZPUMPE_GPIO` | Umwälzpumpe (Vorratsbehälter-Zirkulation) |
| `48` | `PUMPE_GPIO` | Hauptpumpe |

Die Treiber-Polarität aller direkten Ausgänge (Ventile 1–4, Pumpe, Umwälzpumpe) ist über `OUTPUT_ACTIVE_LOW` in `pinout.h` umschaltbar (0 = High-aktiv, Default; 1 = Low-aktiv für invertierte MOSFET-Treiber).

#### I2C-Geräte (SDA=18, SCL=17)

| Adresse | Baustein | Funktion |
|---|---|---|
| `0x38` | AHT21B | Raumklima-Referenzsensor (Standardadresse der Adafruit_AHTX0-Bibliothek) |
| `0x52` | ENS160 | Optionaler eCO₂-Sensor für die Raumluft (z. B. DFRobot SEN0515, Kombimodul mit AHT21), siehe [Raumklima & VPD](#raumklima--vpd) |
| `0x58` | GP8403 | Optionaler 2-Kanal-0–10V-DAC, siehe [Analog-Ausgangsmodul](#analog-ausgangsmodul) |
| `0x68` | DS3231 | Echtzeituhr + integriertes EEPROM (AT24C32) |

### Ventile & Pumpen

Die vier Magnetventile, die Hauptpumpe und die Umwälzpumpe sind direkte GPIO-Ausgänge (siehe [Hardware & Pinout](#hardware--pinout)) — kein I2C-Expander.

| Konstante | Ventil | Funktion |
|---|---|---|
| `VENTIL1_PIN` | MV1 | Zulauf Behälter 1 |
| `VENTIL2_PIN` | MV2 | Zulauf Behälter 2 |
| `VENTIL3_PIN` | MV3 | Zulauf Behälter 3 |
| `VENTIL4_PIN` | MV4 | Rücklauf / Ablauf |

#### Hauptpumpe & Umwälzpumpe

Die Hauptpumpe (`PUMPE_GPIO`) schaltet automatisch mit, sobald irgendein Behälter gerade aktiv wässert (Ventil offen) — unabhängig davon, ob im normalen oder im Batch-Zeitmodus (siehe [Ventilsteuerung](#funktionen)).

Die Umwälzpumpe (`UMWAELZPUMPE_GPIO`, Vorratsbehälter-Zirkulation) läuft in einem konfigurierbaren Lauf-/Pausenzyklus (`laufzeit_s` / `pausenzeit_min`), sobald irgendein Behälter überhaupt aktiviert ist — unabhängig davon, ob dieser Behälter gerade tatsächlich wässert oder nur zwischen zwei Zyklen pausiert. Ist kein Behälter aktiviert, bleibt sie aus.

### Funktionen

#### Netzwerk

W5500 (Ethernet) und WiFi laufen gleichzeitig über das lwIP-Stack. Bei Ethernet-Verbindungsaufbau wird automatisch **NTP** abgerufen und die RTC gestellt (Schwellwert: ±60 s Abweichung, sync alle 60 min). Das Webinterface ist auf beiden IPs ohne Codeänderungen verfügbar.

Ohne Router: Master startet als **AP auf Kanal 6** (`ESPNOW_DEFAULT_CHANNEL`). Nodes im "Kein-WLAN"-Modus nutzen denselben Kanal.

#### Ventilsteuerung

Drei unabhängige Bewässerungskreise mit konfigurierbarer Öffnungszeit (Sek.) und Pausenintervall (Min., jeweils getrennt für Licht- und Dunkelphase). Optional vorgeschalteter Rücklauf-Vorlauf via MV4 (konfigurierbare Vorlaufzeit). Im Modus "Gleiche Zeiten für alle" läuft statt drei unabhängiger Timer ein gemeinsamer Batch-Durchlauf (Vorlauf einmal vor dem ersten aktiven Behälter, dann alle aktiven Behälter direkt nacheinander, eine gemeinsame Pause).

Nach einem Neustart wird **sofort** gewässert, statt erst die volle Pausenzeit (bis zu 60 min) abzuwarten.

**Manueller Einzelzyklus:** Auf der Aeroponik-Seite lässt sich pro Behälter ein einzelner Gießzyklus von Hand auslösen — das Ventil öffnet einmal für die aktuell gültige Öffnungszeit (Licht- bzw. Dunkelphase) und schließt danach automatisch (`/api/ventile/dose/start`, `/status`, `/cancel`). Die normale Automatik pausiert währenddessen.

**Ausgangstest:** Der Direkttest der Ausgänge (Verkabelungsprüfung, Durchflussmesser-Kalibrierung) unterbricht keinen laufenden Gießzyklus mehr — ist gerade ein Ventil/eine Pumpe automatisch aktiv, wird der Testmodus erst aktiv, sobald der Zyklus von selbst beendet ist. Die UI zeigt den Wartezustand an (`GET /api/output/test`).

#### Tanküberwachung (Kegelstumpf)

Füllstand wahlweise per **Ultraschall** (HC-SR04/RCWL-1670/JSN-SR04T) oder per **Drucksensor** (HX710B-basierte 0–40kPa-Module, 2-Draht-Protokoll ähnlich HX711, kein I2C) — umschaltbar per Konfiguration, beide Sensortypen nutzen dieselben zwei GPIOs (`ULTRASONIC_TRIG`/`ULTRASONIC_ECHO` bzw. `HX710_SCK`/`HX710_DOUT`), keine Platinenänderung beim Wechsel nötig. Umrechnung Distanz/Druck-Rohwert → Wasserhöhe → Volumen unter Berücksichtigung der konischen Tankgeometrie (einstellbare Radien oben/unten und Höhe). Beim Ultraschall glättet ein Median-Filter über die letzten 5 Messungen einzelne Ausreißer weg.

#### DS18B20 — Rollenzuordnung über ROM-Adresse

Die DS18B20-Sensoren am gemeinsamen 1-Wire-Bus (Vorrat, Pflanze, optional Innentemperatur für den Gehäuselüfter) werden über ihre eindeutige 64-Bit-ROM-Adresse einer Rolle zugeordnet, statt sich auf die Bus-Scan-Reihenfolge zu verlassen (die sich bei vertauschten oder unterschiedlich langen Kabeln ändert). Solange eine Rolle nicht zugewiesen ist, fällt die Firmware auf die alte Index-Reihenfolge zurück (0=Vorrat, 1=Pflanze, 2=Innentemperatur).

#### Drucksensor (Pumpendruck)

Der analoge Drucksensor an `PRESSURE_PIN` (GPIO5) lässt sich auf der Aeroponik-Seite ein-/ausschalten (eigener EEPROM-Block, Magic `0x19`) — für Aufbauten ohne bzw. mit defektem Sensor. Ausgeschaltet entfällt die Druck-Karte auf der Startseite und es wird kein Druck gemessen (`/api/drucksensor`, `/api/drucksensor/save`).

#### Gehäuselüfter

Einfacher An/Aus-Schalter (kein PWM) am Steuerungsgehäuse, geregelt über den Innentemperatur-DS18B20. Hysterese: an ab `temp_max`, aus ab/unter `temp_min` — dazwischen bleibt der zuletzt gesetzte Zustand erhalten, damit der Lüfter nicht dauernd taktet.

#### Netzspannungsüberwachung

`NETZ_OK_PIN` erkennt einen Netzausfall (Betrieb an der USV) — HIGH = Netz vorhanden, LOW = Netzausfall, 2000 ms entprellt gegen kurze Spannungsschwankungen. Der Status ist in der Weboberfläche sichtbar und löst optional eine [Benachrichtigung](#benachrichtigungen) aus.

#### Benachrichtigungen

Drei unabhängig voneinander aktivierbare Kanäle — CallMeBot ist erfahrungsgemäß öfter mal instabil, Telegram und/oder E-Mail laufen deshalb parallel weiter, falls ein Kanal gerade ausfällt. Ein globaler Schalter (`global_enabled`) übersteuert alle Kanäle und Alarme; jeder Alarm ist zusätzlich einzeln aktivierbar:

| Kanal | Einrichtung |
|---|---|
| WhatsApp | Über [CallMeBot](https://www.callmebot.com/blog/free-api-whatsapp-messages/) — Telefonnummer + API-Key, einmalig per WhatsApp-Aktivierungsnachricht erhalten |
| Telegram | Bot bei @BotFather anlegen (Token), eigene Chat-ID z. B. über @userinfobot ermitteln — Versand per Telegram-Bot-API (HTTPS) |
| E-Mail | Echtes SMTP (implizites TLS, Port 465 üblich) über ein eigenes Postfach, z. B. Gmail mit App-Passwort (nicht das normale Kontopasswort) — kein Drittanbieter-Dienst nötig |

| Alarm | Auslöser |
|---|---|
| Netzausfall | Siehe [Netzspannungsüberwachung](#netzspannungsüberwachung) |
| Sensor-Ausfall | AHT21 / RTC / DS18B20 nicht erreichbar |
| Luftfeuchte | Zelt-Luftfeuchte (RS485-Multisensor) außerhalb `feuchte_min`/`feuchte_max` |
| Temperatur | Zelt-Temperatur (RS485-Multisensor) außerhalb `temperatur_min`/`temperatur_max` |
| Behälter fast leer | Vorratsbehälter-Füllstand unter `tank_min_prozent` |
| Zeltlüfter-Drehzahl | Ist-Drehzahl weicht vom Erwartungswert ab, siehe [Lueftermodul](#lueftermodul) |
| Düsen-Durchfluss | Soll/Ist-Abweichung je Behälter, siehe [Düsen-Überwachung](#durchflussmesser-kalibrierung--düsen-überwachung) |

Jeder Kanal hat ein eigenes Rate-Limit (30 s) gegen versehentliches Zuspammen; ein eigener Test-Button pro Kanal in der Konfiguration verifiziert die Zugangsdaten unabhängig von den anderen.

#### SD-Logging

Alle 5 Minuten werden sämtliche Mess- und Steuerwerte in CSV-Dateien nach dem Schema `/JJJJ-MM-TT.csv` geschrieben, inkl. Raum-/Zeltklima, Lüfter- und Lichtstatus. Details, vollständige Spaltenliste sowie Download/Löschen über das Webinterface: siehe [Daten-Log & Download](#daten-log--download).

#### Node-OTA

Über die API kann für jeden registrierten Node ein OTA-Update ausgelöst werden. Der Master öffnet kurzzeitig einen Hilfs-AP (`Aeroponik-OTA`, zeitlich begrenztes Zufalls-Passwort); der Node verbindet sich und lädt die Firmware per HTTP.

### Beleuchtungs-Scheduler

Simuliert Sonnenauf- und -untergang über bis zu 4 SSRs. Sendet alle 30 Sekunden — bei Maskenänderung — `CMD_SET_ONOFF` an alle online gemeldeten Nodes mit Typ `NODE_TYPE_LIGHT_SSR`. Erfordert funktionierende RTC.

#### Parameter (EEPROM, Magic 0xD5)

| Feld | Standard | Einheit | Beschreibung |
|---|---|---|---|
| `enabled` | false | bool | Scheduler aktiv |
| `dawn_start` | 360 | min seit 0:00 | Beginn Sonnenaufgang (6:00) |
| `dawn_end` | 480 | min seit 0:00 | Ende Aufgang / volle Helligkeit (8:00) |
| `dusk_start` | 1080 | min seit 0:00 | Beginn Sonnenuntergang (18:00) |
| `dusk_end` | 1200 | min seit 0:00 | Ende Untergang / Dunkel (20:00) |
| `num_ssr` | 4 | 1–4 | Anzahl genutzter SSRs |

#### Relay-Maske — Berechnungslogik

```c
// t = linearer Fortschritt 0.0→1.0 im Auf-/Untergang
active = ceilf(t × num_ssr);        // gleichmäßige Stufen, erste/letzte Stufe exakt am Fensterrand
mask   = (active == 0) ? 0x00 : (uint8_t)((1 << active) - 1);
```

> **Hinweis:** `round()` wurde durch `ceil()` ersetzt: Mit `round()` waren die erste und letzte Stufe nur halb so breit wie die mittleren (Bin-Ränder bei 0 und n decken nur `[0, 0.5/n)` bzw. `[1−0.5/n, 1]` ab) — dadurch schaltete die erste SSR zu spät ein und die letzte zu früh aus.

Beispiel `num_ssr=4`, Aufgang 6:00–8:00 (120 min):

| Zeitraum | t | Aktive SSRs | Maske |
|---|---|---|---|
| vor 6:00 | — | 0 | `0x00` |
| 6:00 – 6:30 | 0,0 – 0,25 | 1 | `0x01` (SSR1) |
| 6:30 – 7:00 | 0,25 – 0,5 | 2 | `0x03` (SSR1+2) |
| 7:00 – 7:30 | 0,5 – 0,75 | 3 | `0x07` (SSR1+2+3) |
| 7:30 – 8:00 → 18:00 | 0,75–1,0 / voll | 4 | `0x0F` (alle) |
| 18:00 – 20:00 | 1,0 → 0,0 | 4 → 0 | spiegelverkehrt |
| nach 20:00 | — | 0 | `0x00` |

### RS485-Bus

Zweiter, vom ESP-NOW-Netz unabhängiger Kommunikationsweg für fest verdrahtete Geräte im Zelt. Physikalisch ein gemeinsam genutzter Zweidraht-Bus über einen **MAX13487** (automatische Sende-/Empfangsrichtung, kein manuelles DE/RE-Umschalten nötig) an `RS485_TX` (GPIO1) / `RS485_RX` (GPIO2), **115200 Baud, 8N1** — Master und alle eigenen Slaves (Lichtx4, Multisensor, Multimodul, ProtoRS485).

> **Übergang für ein noch nicht neu geflashtes Lichtx4:** Ein bereits eingebautes Lichtx4 mit alter Firmware läuft noch auf 9600 Baud. Für diesen Fall gibt es das Define `LICHTX4_LEGACY_9600` — es wird in einer optionalen, per `.gitignore` ausgeschlossenen Datei `Aeroponik/src/local_config.h` gesetzt (`#define LICHTX4_LEGACY_9600`). Ist es aktiv, schaltet der Master nur für die Lichtx4-Transaktionen kurz auf 9600 Baud um und danach zurück. Ohne die Datei (Standard im Repo) läuft der ganze Bus mit 115200 Baud.

> **Historisch:** Auf demselben Bus lief bis vor Kurzem zusätzlich ein MARS-Hydro-*iControl*-Hub als weiterer Modbus-Master (Fremdsteuerung des Abluftlüfters per Werks-App). Dieses Gerät wurde vollständig entfernt — der Aeroponik-Master ist jetzt alleiniger Bus-Master. Damit der Lüfter ihn als Master akzeptiert, sendet der Master den periodischen Broadcast des iControl-Hubs nach, siehe [Abluftlüfter](#abluftlüfter).

#### Protokoll

Vereinfachtes Modbus-RTU: **FC03** (Read Holding Registers) zum Auslesen, **FC06** (Write Single Register) zum Schreiben. CRC16 nach Modbus-Standard (Polynom `0xA001`). Ein schlankes Zustandsautomat-Modell (`mb_state_t`) fragt die Busteilnehmer nacheinander ab; nur eine Anfrage gleichzeitig offen, Antwort-Timeout **2000 ms**.

Nach jedem Senden wird das eigene Sende-Echo aktiv weggeräumt (`flush()` + kurze Pause + Restpuffer leeren), bevor auf die Antwort gewartet wird — nötig, weil der MAX13487 auf allen eigenen Platinen mit fest auf Empfang liegendem RE die eigene Sendung immer mit auf RX zurückspiegelt.

> **Resync nach Slave-Reboot:** Lueftermodul/Multimodul (`0x51`) haben kein Flash-Gedächtnis für PWM-/Analogwerte — ein eigenständiger Reset des Slaves (Stromwackler, o. ä.) setzt sie auf 0 zurück, ohne dass der Master zwangsläufig eine Bus-Unterbrechung bemerkt (zu kurzer Ausfall). Register 8 der Lüfter-Registerkarte enthält deshalb eine bei jedem Boot zufällig neu gewürfelte **Boot-ID** (`rp2040.hwrand32()`); ändert sie sich gegenüber dem letzten Poll, erkennt der Master zuverlässig „Slave neu gestartet“ und schreibt die zuletzt kommandierten Lüfter-PWM-Werte **und** beide GP8403-Analogkanäle automatisch neu (gleiches physisches Board, siehe [Multimodul](#multimodul)). Licht (Lichtx4) braucht das nicht — die Relaismaske wird dort bereits Node-seitig per `Preferences` persistiert und beim eigenen Boot selbst wiederhergestellt.

#### Bus-Teilnehmer

| Adresse | Gerät | Zugriff | Poll-Intervall |
|---|---|---|---|
| `0x20` | RS485-Multisensor (Zelt-Klima + PPFD) | FC03 read | 15 s |
| `0x40` | Lichtx4 (4× Relais-Lichtsteuerung) | FC03 read + FC06 write | 15 s (+ sofort bei Maskenänderung) |
| `6` | Abluftlüfter — MARS Hydro | FC03 read + FC06 write | 3 s |
| `0x50` | Analog-Ausgangsmodul (2× 0-10V) | FC03 read + FC06 write | 15 s |
| `0x51` | Lueftermodul / Multimodul (4× Zeltlüfter PWM/Tacho) | FC03 read + FC06 write | 3 s |

Lichtx4, MARS-Lüfter und Analogmodul werden nur gepollt, wenn sie laut [Geräte-Konfiguration](#analog-ausgangsmodul) tatsächlich als Ausgang ausgewählt sind. Das RS485-Multisensor wird immer abgefragt; das Lueftermodul/Multimodul ebenfalls immer, sofern in der Konfiguration aktiviert (siehe [Lueftermodul](#lueftermodul)).

#### Priorität bei gleichzeitigem Bedarf

Ausstehende Schreibbefehle gehen vor den zyklischen Lesevorgängen. Reihenfolge: Licht-Write → Lüfter(MARS)-Write → Analog-Write (Kanal 1, dann 2) → Lueftermodul-Write → Multisensor-Read → Licht-Read → Lüfter(MARS)-Read → Analog-Read → Lueftermodul-Read.

### RS485-Multisensor Zelt

Fest im Zelt verbautes Sensormodul an RS485-Adresse `0x20`, liefert Klimadaten und den **PPFD-Lichtsensor inkl. 8-Kanal-Spektrum in einem Gerät** — daher entfällt für dieses Deployment der separate ESP-NOW-Spektrum-Node.

#### Gelesene Register (FC03, 14 Register ab Register 0)

| Feld | Einheit | Beschreibung |
|---|---|---|
| `co2_ppm` | ppm | CO₂-Konzentration im Zelt |
| `temp_c` | °C | Zelt-Lufttemperatur |
| `hum_pct` | %rH | Zelt-Luftfeuchte |
| `ppfd` | µmol/m²/s | Photonenflussdichte |
| `channels[8]` | — | Spektralkanäle |
| `gain`, `status` | — | Sensor-Diagnose |

**PPFD-Berechnung:** physikalisches Modell aus den 8 Spektralkanälen (Kanal-Responsivität laut AS7341-Datenblatt, Photonenenergie je Mittenwellenlänge, normiert auf GAIN_1X) mit einem empirischen Kalibrierfaktor, der mit dem Licht-Meter-Handgerät gegen ein Referenzmessgerät ermittelt wurde. Ersetzt die frühere MLR-Regression, die bei Spektren abseits der Trainingslampen deutlich danebenlag. Gleiche Berechnung in Multisensor und Multimodul.

Zelt-Temperatur, -Feuchte und -CO₂ dieses Sensors sind die Grundlage für [Abluftlüfter-Automodi](#abluftlüfter), [CO2-Steuerung](#co2-steuerung) und die [VPD-Berechnung](#raumklima--vpd).

### Lichtx4

RS485-Lichtsteuerung. Eigenständige Firmware (`Lichtx4`) mit 4 Relais-/SSR-Ausgängen, ausschließlich über RS485 angebunden — **kein ESP-NOW**. Hat den früher vorgesehenen ESP-NOW-Licht-SSR-[Steckdosen-Node](#steckdosen) als Lichtsteuerung vollständig abgelöst.

#### Register

| Adresse | Register | Zugriff | Inhalt |
|---|---|---|---|
| `0x40` | 0 | FC03 read | Aktuelle Relais-Bitmaske (Bit0–3 = Ausgang 1–4) |
| `0x40` | 0 | FC06 write | Neue Relais-Bitmaske setzen |

Wird vom bestehenden [Beleuchtungs-Scheduler](#beleuchtungs-scheduler) angesteuert — statt eines ESP-NOW `CMD_SET_ONOFF`-Pakets schreibt der Master bei Maskenänderung direkt per FC06 auf Register 0.

### Abluftlüfter

MARS Hydro (RS485). Werksseitig fertige Lüfterplatine, RS485-Adresse `6`, ursprünglich per MARS-Hydro-*iControl*-App gesteuert. Der Master kann die Steuerung optional vollständig übernehmen.

> **Sicherheits-Default:** `enabled=false` — ohne explizite Aktivierung sendet der Master keine Schreibbefehle, der Lüfter bleibt unangetastet. Erst nach Aktivierung in der Konfiguration übernimmt der Master die Regelung. (Die iControl-Hardware wurde inzwischen vollständig entfernt.)

#### Register

| Register | Zugriff | Inhalt |
|---|---|---|
| 0–16 | FC03 read | Immer das volle Set: Start 0, 17 Register (wie der Original-Controller) |
| 11 | (im Block) | Drehzahl (RPM) |
| 15 | FC06 write | Enable-Bit — wird **vor** jeder Leistungsänderung auf 1 gesetzt, sonst übernimmt der Lüfter den Wert nur ins Register, fährt aber nicht physisch hoch |
| 16 | FC03 read (im Block) / FC06 write | Aktuelle bzw. Ziel-Leistung 0–100 % |

> **Master-Erkennung (Experiment):** Der Lüfter scheint einen Bus-Master nicht am bloßen Polling zu erkennen, sondern am periodischen Broadcast der echten iControl-Steuerung. Der Master sendet deshalb alle 6 s denselben Broadcast nach (Adresse 0, FC10, Start 1001, 26 Register, `reg[8]=7` als Signatur; keine Antwort erwartet). Für den Verdacht auf eine verschobene Geräteadresse gibt es einen Diagnose-Scan (`POST /api/rs485/scanfan`, JSON `{"from":1,"to":20}`), der jede Adresse im Bereich per FC03 anfragt und die Antworten roh zurückliefert — Treffer auf den eigenen Adressen (`0x20`, `0x40`, `0x50`, `0x51`) sind dabei normal.

#### Steuerungsmodi

| Modus | Quelle | Verhalten |
|---|---|---|
| Manuell | — | Feste Leistung (%) |
| Auto — Luftfeuchte | Zelt-Feuchte (RS485-Multisensor) | Linearer Ramp zwischen `hum_min` (Mindestdrehzahl) und `hum_max` (100 %) |
| Auto — Temperatur, absolut | Zelt-Temperatur | Ramp zwischen `temp_min`/`temp_max`; optionale **Differenz-Bremse**: liegt Zelt−Raum-Differenz unter `temp_cap_diff`, wird trotz Rampe nur die Mindestdrehzahl gefahren (verhindert Volllast, wenn der Raum fast so warm wie das Zelt ist) |
| Auto — Temperatur, Differenz | Zelt-Temperatur − Raumtemperatur | Ramp direkt über die Temperaturdifferenz zwischen `temp_min`/`temp_max` |

Eine gemeinsame Mindestdrehzahl (`min_speed`) gilt für beide Automodi. Die Differenz-Modi benötigen den [Raumklimasensor](#raumklima--vpd); ist dieser nicht verfügbar, bleibt die letzte Zielleistung unverändert.

#### Konfiguration (EEPROM, Magic 0x92)

`mode` · `manual_percent` · `min_speed` · `hum_min/max` · `temp_min/max` · `temp_mode` (absolut/Differenz) · `temp_cap_diff` · `enabled`

### Lueftermodul

> **Eigenständiges Lueftermodul-Projekt entfernt:** Die frühere separate Firmware `Lueftermodul` (nur Lüfter, keine optionale I2C-Peripherie) wurde gelöscht — [Multimodul](#multimodul) deckt den reinen Lüfter-Anwendungsfall bereits vollständig ab (MS2/Analog bleiben einfach inaktiv, wenn die zugehörige I2C-Peripherie fehlt). Auch bei einem Board, das nur Lüfter ansteuert, wird deshalb die Multimodul-Firmware geflasht — das Register-/Pinout-Layout unten gilt unverändert fort.

Firmware auf einem **RP2040-Zero** (Waveshare), steuert bis zu 4 direkt am Zelt montierte PC-Lüfter (Arctic, 600–3000 RPM, PWM-gesteuert, 0 RPM unter 5 % PWM) mit individueller oder gemeinsamer Drehzahl in 10-%-Schritten. Nur über RS485 angebunden, Adresse `0x51`.

#### Pinout

| Funktion | GPIO |
|---|---|
| RS485 TX (DI) / RX (RO) | 28 / 29 (UART0) |
| PWM Lüfter 1–4 | 0, 2, 4, 6 (25 kHz) |
| Tacho Lüfter 1–4 | 1, 3, 5, 7 (Flanken-Interrupt) |
| WS2812 Status-LED | 16 |

PWM- und Tacho-Pin eines Lüfters liegen bewusst nebeneinander (vereinfacht das Platinenlayout — beide Signale eines Lüfter-Steckers liegen so nebeneinander).

#### Register

| Register | Zugriff | Inhalt |
|---|---|---|
| 0–3 | FC06 write / FC03 read | PWM-Sollwert Lüfter 1–4, 0–100 % |
| 4–7 | FC03 read (read-only) | Ist-Drehzahl Lüfter 1–4, U/min |
| 8 | FC03 read (read-only) | Boot-ID — zufällig bei jedem Start neu (`rp2040.hwrand32()`), siehe [RS485-Bus](#rs485-bus) |

#### Master-seitige Überwachung

Der Master vergleicht die gemeldete Ist-Drehzahl gegen eine aus dem PWM-Sollwert erwartete Drehzahl (lineare Interpolation zwischen 600 RPM bei 5 % und 3000 RPM bei 100 %, 0 RPM unter 5 %) mit einstellbarer Toleranz und meldet auffällige Abweichungen über die [Benachrichtigungen](#funktionen).

### Multimodul

Eigenständige Firmware (`Multimodul`) auf derselben RP2040-Zero-Basis wie das [Lueftermodul](#lueftermodul) — identisches Pinout, plus ein zweiter I2C-Bus (I2C1, SDA=GPIO10/SCL=GPIO11) für optionale Zusatz-Hardware:

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — dieselben Sensoren wie im [Multisensor](#multisensor)-Node. Wenn angeschlossen, beantwortet das Board zusätzlich RS485-Adresse `0x20` mit demselben 14-Register-Layout wie das [RS485-Multisensor](#rs485-multisensor-zelt).
- **GP8403** (I2C `0x58`, 2-Kanal-0–10V-DAC) — wenn angeschlossen, beantwortet das Board zusätzlich RS485-Adresse `0x50` mit demselben 2-Register-Layout wie das [Analog-Ausgangsmodul](#analog-ausgangsmodul).

Beim Boot scannt die Firmware den I2C-Bus; nur tatsächlich gefundene Geräte werden initialisiert und ihre RS485-Adresse aktiviert. Fehlt ein Gerät, bleibt die zugehörige Adresse auf dem Bus unbeantwortet — für den Master nicht von einem schlicht nicht verbauten separaten Multisensor/Analogmodul zu unterscheiden. Dadurch ist am Master **keine Codeänderung** nötig: er pollt `0x20`, `0x50` und `0x51` bereits unabhängig voneinander und behandelt Ausbleiben der Antwort ohnehin als „Gerät offline“.

Ein Multimodul kann also je nach Bestückung ein reines Lüftermodul, ein Lüftermodul mit Zelt-Klimasensorik, ein Lüftermodul mit 0–10V-Ausgang oder alles zusammen sein — auf demselben RS485-Bus, ohne separate Firmware-Varianten.

> **Boot-Reihenfolge:** `rs485_slave_init()` läuft bewusst ganz am Anfang von `setup()`, **vor** dem I2C-Scan/Sensor-Init — sonst bliebe die DI-Leitung (GP28) unkonfiguriert/hochohmig, solange der I2C-Scan oder `scd41_init()`/`gp8403_init()` laufen (können bei einer hängenden I2C-Transaktion beliebig lange dauern), ohne definierten Idle-Pegel auf dem Bus. Zusätzlich loggt die Firmware beim Start die Reset-Ursache (`rp2040.getResetReason()`: Power-On, Watchdog, Brownout, Glitch, …) — nützlich zur Diagnose unerklärter Neustarts, da das Projekt selbst keinen Software-Watchdog nutzt.

> **SCD41 und AS7341 unabhängig:** CO₂/Temperatur/Feuchte (SCD41) werden sofort nach jeder Messung in die `0x20`-Register geschrieben, unabhängig vom AS7341. Bleibt eine AS7341-Messung länger als 6 s hängen, wird nur der AS7341 neu initialisiert (kein I2C-Bus-Reset, der den SCD41 mitreißen würde); nach 3 erfolglosen Versuchen wird er für 10 min deaktiviert (Lichtwerte = 0, Gain = `0xFF`, Status-Bit 1 = 0) und danach automatisch neu versucht — Board, SCD41, Lüfter und Analogausgang bleiben die ganze Zeit online.

### Analog-Ausgangsmodul

0-10V. 2-Kanal RS485-zu-0-10V-Wandler (ATO), Standard-Modbus-RTU. Wahlweise Alternative zu Lichtx4 (Kanal 2, stufenlose Helligkeit statt Relais-Stufen) und/oder zum MARS-Hydro-Lüfter (Kanal 1) — z. B. für generische Lüfter oder Dimmer mit 0-10V-Steuereingang. Dasselbe Register-Layout wird auch vom optionalen GP8403-Ausgang des [Multimodul](#multimodul) bedient.

> **Einmaliger Hardware-Setup:** Das Modul kommt ab Werk auf Adresse `0x01` / 4800 Baud. Vor dem Einbau muss es einmalig **einzeln** (nicht am gemeinsamen Bus) per USB-RS485-Adapter und der ATO-Konfigurationssoftware umgestellt werden: Adresse → `0x50`, Ausgangstyp beider Kanäle → 0-10V (Register 0x0055 sowie die physischen Lötbrücken auf "V"), Baudrate → die des Busses.
>
> **Achtung Baudrate:** Der Bus läuft inzwischen mit **115200 Baud** (früher 9600). Ein ATO-Modul, das noch auf 9600 eingestellt ist, antwortet dann nicht mehr — es muss auf 115200 umgestellt werden, sofern die ATO-Software das anbietet. Andernfalls übernimmt der GP8403-Ausgang des [Multimodul](#multimodul) die Adresse `0x50`.

#### Register

| Register | Zugriff | Inhalt |
|---|---|---|
| 0x0000 | FC03 read + FC06 write | Kanal 1 (Lüfter), 0–4095 (12-bit) = 0–10V |
| 0x0001 | FC03 read + FC06 write | Kanal 2 (Licht), 0–4095 (12-bit) = 0–10V |

Ziel-Prozentwerte (0–100 %) aus Lüfter-Rampe bzw. Zeitplan werden linear auf 0–4095 umgerechnet (`round(percent / 100 × 4095)`).

Zum Testen ohne die echte Hardware simuliert das separate Projekt `ProtoRS485` (ESP32-C3 + WS2812-Streifen) dieses Modul auf dem Bus — siehe [ProtoRS485/README.md](../../ProtoRS485/README.md).

#### Geräte-Konfiguration (Setup statt Laufzeit-Umschaltung)

Unter Konfiguration → Geräte legt man fest, welches Gerät Licht bzw. Lüfter tatsächlich ansteuert (Lichtx4/MARS Hydro oder Analogmodul) — bewusst als Setup-Entscheidung, nicht als Umschalter im laufenden Betrieb. Das hat zwei Effekte: Nur das gewählte Gerät bekommt Steuerbefehle, und nur tatsächlich vorhandene Geräte werden überhaupt auf dem RS485-Bus gepollt (das Multisensor bleibt davon unberührt, es wird immer abgefragt).

Für den Lichtausgang ändert die Auswahl auch die Regelcharakteristik: Lichtx4 nutzt weiterhin die gestufte Relais-Rampe ([Beleuchtungs-Scheduler](#beleuchtungs-scheduler)), das Analogmodul eine stufenlose Helligkeitsrampe über dieselben Zeitfenster.

### CO2-Steuerung

Hysterese-Regelung eines CO₂-Ausgangs auf einem beliebigen **ESP-NOW-Steckdosen-Node**, gesteuert über die CO₂-Messung des [RS485-Multisensors](#rs485-multisensor-zelt).

#### Logik

- CO₂ ≤ `co2_min` → Ziel-Relais **EIN**
- CO₂ ≥ `co2_max` → Ziel-Relais **AUS**
- dazwischen: letzter Zustand bleibt erhalten (klassische Hysterese, kein Takten)

Nur das konfigurierte Relais-Bit (`target_relay_bit`, 0–3) des gewählten Nodes (`target_node_id`) wird geschaltet — die übrigen Bits der Relaismaske dieses Nodes bleiben unangetastet.

#### Konfiguration (EEPROM, Magic 0x7C)

`enabled` · `target_node_id` · `target_relay_bit` · `co2_min` (Standard 800 ppm) · `co2_max` (Standard 1200 ppm)

Auf der Startseite erscheint eine Status-Karte nur, wenn die Funktion aktiviert ist.

### Luftbefeuchtung-Steuerung

1:1 dieselbe Hysterese-Logik wie die [CO2-Steuerung](#co2-steuerung), nur mit der Zelt-Luftfeuchte (RS485-Multisensor) als Regelgröße statt CO₂ — ebenfalls auf einem beliebigen ESP-NOW-Steckdosen-Node.

#### Logik

- Luftfeuchte ≤ `humid_min` → Ziel-Relais **EIN** (befeuchten)
- Luftfeuchte ≥ `humid_max` → Ziel-Relais **AUS**
- dazwischen: letzter Zustand bleibt erhalten

#### Konfiguration (eigener EEPROM-Block, Magic 0x44)

`enabled` · `target_node_id` · `target_relay_bit` · `humid_min` (Standard 50,0 %) · `humid_max` (Standard 65,0 %) — intern als %×10 ohne Float im EEPROM abgelegt, im UI als normale Prozentzahl mit einer Nachkommastelle.

Eigene Status-Karte auf der Startseite, nur sichtbar wenn aktiviert.

### Raumlicht & DWC-Timer

Zwei einfache Schalt-Funktionen, die — wie [CO2-](#co2-steuerung) und [Luftbefeuchtung-Steuerung](#luftbefeuchtung-steuerung) — genau einen Ausgang (`target_relay_bit`, 0–3) eines **ESP-NOW-Steckdosen-Nodes** (`target_node_id`) schalten; die übrigen Relais-Bits des Nodes bleiben unangetastet.

#### Raumlicht (EEPROM, Magic 0x3F)

Rein **manueller** Ein/Aus-Schalter, ohne Automatik, Hysterese oder Zeitplan. Der Ausgang wird im Klima-Tab gewählt; sobald einer gewählt ist, erscheint auf der Startseite ein Schalter. Der Ein/Aus-Zustand wird nicht separat gespeichert, sondern live aus der gemeldeten Relaismaske des Ziel-Nodes gelesen — UI und echter Relaiszustand können so nicht auseinanderlaufen. Endpunkte: `GET /api/raumlicht`, `POST /api/raumlicht/save`, `POST /api/raumlicht/toggle`.

#### DWC-Timer (EEPROM, Magic 0x3A)

Einfacher Ein/Aus-Beleuchtungstimer für ein separates DWC-System, wie eine haushaltsübliche Zeitschaltuhr (kein Sonnenauf-/-untergang wie beim [Beleuchtungs-Scheduler](#beleuchtungs-scheduler)): `enabled` · `target_node_id` · `target_relay_bit` · `on_min` / `off_min` (Minuten seit Mitternacht; `off_min < on_min` = über Mitternacht).

### Durchflussmesser-Kalibrierung & Düsen-Überwachung

Durchflussmesser (YF-S401-ähnlich, Hall-Impulsgeber mit Push-Pull-Ausgang, kein Pull-up nötig) in der gemeinsamen Zulaufleitung **vor** dem Ventil-Verteiler — sieht damit den Durchfluss aller 4 Ventile nacheinander, da laut [Ventilsteuerung](#funktionen) ohnehin immer nur eines gleichzeitig offen ist. Interrupt an `FLOW_SENSOR_PIN` (GPIO3) zählt nur die Pulse; die Umrechnung in ml läuft über eine einmalige Kalibrierung.

#### Kalibrierung (Konfiguration → Ventile)

1. **„Kalibrierung starten“** — Pumpe an (GPIO48), Druck wird live angezeigt. Nutzt denselben Ausgangs-Direkttest wie die Verkabelungsprüfung (pausiert damit automatisch die normale Ventil-/Pumpensteuerung).
2. **„Entlüften“** — Ventil auf/zu, beliebig oft, bis keine Luft mehr im System ist (Luft würde die Impulszahl verfälschen). „Start“ ist erst nach mindestens einem abgeschlossenen Entlüftungszyklus möglich.
3. **„Start“** — öffnet Rücklaufventil (MV4), zählt ab da die Impulse (live sichtbar).
4. **„Stop“** — Ventil zu, Impulszahl eingefroren.
5. Gemessene Menge in ml eintragen (z. B. mit einem Messbecher aufgefangen) → **„Berechnen“** ermittelt Pulse/Liter und speichert dauerhaft (eigener EEPROM-Block).

#### Düsen-Überwachung (`flowmonitor.cpp`)

Beobachtet den Ventilzustand (`behaelterZustand[]`) von außen: Wechselt ein Behälter in den Zustand „Aktiv“, wird der Impulszähler geleert (verwirft Reste aus der Vorlaufphase über MV4); beim Verlassen des Zustands wird die Menge berechnet und zu laufenden Summen addiert (je Behälter + Gesamt, nur im RAM seit letztem Neustart — eine echte Lebenszeit-Summe lässt sich bei Bedarf aus dem SD-Log rekonstruieren).

Pro Behälter wird ein **Sollwert** (ml/s) geführt:

- Ist noch keiner hinterlegt (0 bzw. per „Sollwerte löschen“ zurückgesetzt), übernimmt das System automatisch den **nächsten abgeschlossenen Gießzyklus** als neue Referenz — keine manuelle Berechnung nötig.
- Ist einer hinterlegt und der Soll/Ist-Vergleich aktiviert, wird jeder Zyklus mit konfigurierbarer Toleranz (%) verglichen; bei Abweichung nach unten (verstopfte Düse) oder oben (Leck) geht eine [Benachrichtigung](#benachrichtigungen) über alle aktiven Kanäle raus.
- „Sollwerte löschen“ setzt alle drei Werte auf 0 zurück — praktisch nach dem Düsenreinigen, damit nicht die alten (jetzt wieder gültigen) Werte fälschlich als Abweichung gewertet werden.

SD-Log-Spalten: siehe [Daten-Log & Download](#daten-log--download).

### Raumklima & VPD

#### AHT21B

Hängt am Haupt-I2C-Bus (`I2C_SDA`/`I2C_SCL`, GPIO18/17) zusammen mit der RTC und dem optionalen GP8403 — siehe [I2C-Geräte](#i2c-geräte-sda18-scl17). Der Sensor misst die **Raumluft außerhalb des Zelts** als Referenz.

> **Bekannter Library-Bug:** Adafruit_AHTX0 sendet standardmäßig den AHT10-Kalibrierbefehl `0xE1`. AHT20/AHT21(B)-Chips erwarten stattdessen `0xBE` — sonst liefert der Feuchtekanal dauerhaft ~100 %, während die Temperatur bereits korrekt ist. Die Firmware sendet den korrekten Befehl (`0xBE, 0x08, 0x00`) direkt nach `aht21.begin()` manuell nach.

#### ENS160 (optional)

Optionaler eCO₂-Sensor für die Raumluft am selben I2C-Bus (Adresse `0x52`, z. B. DFRobot Gravity SEN0515 — Kombimodul ENS160 + AHT21). Wird beim Boot automatisch erkannt; da der ENS160 aktuelle Temperatur/Feuchte zur Kompensation braucht, bekommt er laufend die AHT21-Werte (`setTempAndHum()`). Der eCO₂-Wert (ppm) erscheint in der Status-API (`ens160Ok`, `ens160Co2`) und in der Weboberfläche. eCO₂ ist ein Schätzwert aus der VOC-Messung, kein echter NDIR-CO₂-Wert wie beim SCD41 im Zelt.

#### VPD (Vapor Pressure Deficit)

Berechnet aus **Zelt**-Temperatur und -Feuchte (RS485-Multisensor) nach der Tetens-Formel, serverseitig identisch zur UI-Berechnung:

```c
SVP = 0.6108 × exp(17.27 × T / (T + 237.3))   // kPa
VPD = SVP × (1 − RH / 100)
```

Anzeige zwischen Luftfeuchte und CO₂ in der Messwerte-Karte; Raum- und Zelttemperatur zusammen sind zusätzlich Grundlage der [Lüfter-Differenzmodi](#abluftlüfter).

### Daten-Log & Download

Eine CSV-Datei pro Tag (`/JJJJ-MM-TT.csv`) auf der MicroSD-Karte, Schreibintervall **5 Minuten**. Werte einer gerade nicht erreichbaren Quelle (Raumsensor, RS485-Multisensor, Lüfter offline) werden als **leere Zelle** statt eines irreführenden Nullwerts geschrieben.

#### Spalten

| Spalte | Quelle |
|---|---|
| `Zeit` | RTC |
| `TempVorrat_C`, `TempPflanze_C` | 2× DS18B20 |
| `Druck_bar` | Drucksensor |
| `Fuellstand_%`, `Volumen_L` | Ultraschall + Tankgeometrie |
| `RaumTemp_C`, `RaumFeuchte_%` | AHT21B (Raumklima) |
| `ZeltTemp_C`, `ZeltFeuchte_%`, `ZeltCO2_ppm` | RS485-Multisensor |
| `ZeltVPD_kPa` | berechnet |
| `LichtMaske` | Lichtx4 Relais-Bitmaske |
| `LuefterLeistung_%`, `LuefterRPM` | Abluftlüfter |
| `CO2Ausgang` | CO2-Steuerung Relaiszustand (0/1) |
| `HumidAusgang` | Luftbefeuchtung-Steuerung Relaiszustand (0/1) |
| `FlowBehaelter1_ml` … `FlowBehaelter3_ml` | Düsen-Überwachung — Summe je Behälter seit Neustart |
| `FlowGesamt_ml` | Düsen-Überwachung — Gesamtsumme aller Behälter seit Neustart |

> **Automatische Migration:** Trifft die Firmware beim Schreiben auf eine bereits vorhandene Tagesdatei mit dem alten, kürzeren Spaltenformat (vor dieser Erweiterung), wird diese zu `_alt.csv` umbenannt (Daten bleiben erhalten) und eine neue Datei mit vollständigem Header begonnen — kein Vermischen unterschiedlicher Spaltenzahlen in einer Datei.

#### Download & Löschen (Konfiguration → Daten-Log)

| Endpunkt | Methode | Funktion |
|---|---|---|
| `/api/logs` | GET | Liste aller `*.csv`- und `*.txt`-Dateien mit Dateigröße |
| `/api/logs/download?file=NAME` | GET | Streamt die Datei mit `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Löscht die Datei (UI fragt vorher per Bestätigungsdialog nach) |
| `/api/logs/delete-all` | POST | Löscht alle `*.csv`/`*.txt`-Dateien auf der SD-Karte, Antwort enthält die Anzahl (UI fragt vorher nach) |

Dateinamen werden serverseitig gegen Pfad-Traversal geprüft (kein `/` oder `..` zulässig).

---

### Multisensor

**MCU:** ESP32-C3 · Node-ID: `0x02` (compile-time) · wahlweise `ESP-NOW · NODE_TYPE_SENSOR 0x02` oder `RS485 · Adr. 0x20`

Dieselbe Firmware unterstützt beide Übertragungswege; welcher aktiv ist, wird beim Start festgelegt. Im aktuellen Zelt-Deployment läuft der Node im **RS485-Modus** — Registerbelegung und Poll-Verhalten dazu: [RS485-Multisensor](#rs485-multisensor-zelt).

#### Sensoren (I2C: SDA=1, SCL=0)

| Sensor | I2C-Adresse | Messgrößen |
|---|---|---|
| SCD41 | `0x62` | CO₂ (ppm, NDIR) · Temperatur · Luftfeuchte |
| AS7341 | `0x39` | PPFD (µmol/m²/s × 10) · 8 Spektralkanäle 415–680 nm |

> **Bestückung geändert:** Frühere Revisionen nutzten AHT21 (Temp/Feuchte) + ENS160 (eCO₂/TVOC). Beide wurden durch den SCD41 ersetzt (echter NDIR-CO₂-Sensor statt eCO₂-Schätzwert, plus Temperatur/Feuchte in einem Bauteil).

#### GPIO-Pinout

| GPIO | Funktion | Hinweis |
|---|---|---|
| `0` | I2C_SCL | Sicher, kein Strapping-Pin |
| `1` | I2C_SDA | Sicher, kein Strapping-Pin |
| `3` | CAN_TX | Reserviert für TWAI/CAN-Transceiver |
| `5` | CAN_RX | JTAG MTDI → `gpio_reset_pin(GPIO_NUM_5)` in setup() |

> **ESP32-C3 Strapping-Pins:** GPIO9 zieht ESP32-C3 in Download-Mode wenn LOW beim Booten — nicht für I2C verwenden. GPIO4, 5, 6, 7 sind JTAG-Pins und müssen vor Verwendung mit `gpio_reset_pin()` freigegeben werden.

#### Im ESP-NOW-Modus gesendete Datenpakete

- `SUBTYPE_AIR (0x01)`: Temperatur, Luftfeuchte, CO₂ vom SCD41.
- `SUBTYPE_LIGHT (0x02)`: PPFD + 8 Spektralkanäle vom AS7341.

Im RS485-Modus entfällt der ESP-NOW-Versand — dieselben Werte liegen stattdessen in den Modbus-Registern, siehe [RS485-Multisensor](#rs485-multisensor-zelt).

### Steckdosen

**MCU:** ESP32 · 4× Relais-Ausgänge · `NODE_TYPE_SOCKET · 0x03`

Allgemeine, manuell bzw. per API gesteuerte Relaisknoten — Verwendung u. a. als Ziel-Node der [CO2-Steuerung](#co2-steuerung), für Pumpen, Lüfter und sonstige Verbraucher. Befehle kommen vom Master auf Anfrage (nicht zyklisch).

> **Licht-SSR-Modus ungenutzt:** Die Firmware bietet beim Erststart weiterhin einen zweiten Node-Typ `NODE_TYPE_LIGHT_SSR (0x05)` für automatische Beleuchtungssteuerung an. Diese Funktion wurde durch den dedizierten RS485-Knoten [Lichtx4](#lichtx4) abgelöst — im aktuellen Deployment laufen alle Steckdosen-Nodes als reguläre Steckdose.

#### Relay-Befehl

```c
// Alle 4 Relais einschalten:
MSG_COMMAND + CMD_SET_ONOFF (0x02) + payload[1] = 0x0F

// Nur R1 + R3:
payload[1] = 0b00000101  // = 0x05
```

### Erststart & Konfiguration

#### Steckdosen — Node-Typ und ID

Beim allerersten Start (NVS-Namespace `"nodecfg"` leer) erscheint im Serial-Monitor:

```
=== ERSTSTART: Node-Konfiguration ===
Typ waehlen (60s Timeout):
  1 = Steckdose  (allgemeine Steuerung)
  2 = Licht-SSR  (Sonnenauf/-untergang via Scheduler)
> 1
Node-ID eingeben (1-254) + Enter:
> 10
[NODE] Gespeichert: Typ=Steckdose  ID=0x0A
```

Gespeichert in NVS `"nodecfg"`: Keys `type` (uint8) und `id` (uint8). Mit `NODECLEAR` löschen → Dialog erscheint beim nächsten Boot. Option 2 (Licht-SSR) ist ein Legacy-Modus — siehe Hinweis unter [Node: Steckdosen](#steckdosen).

#### WiFi-Kanal / Kein-WLAN-Modus (alle Nodes)

Beim ersten Start ohne gespeicherten Kanal (NVS `"wificfg"` leer):

```
[WIFI] 3 Netzwerke gefunden
  [ 1] MeinRouter           Kanal  6  -62 dBm
  [ 2] Nachbar-AP           Kanal 11  -78 dBm
  [ 0] Kein WLAN (fester Kanal 6)
Nummer (0-3) + Enter (60s Timeout):
> 0
[WIFI] Kein WLAN, Kanal 6 gespeichert
```

> **Kanal-Konsistenz:** Alle Nodes und der Master-AP müssen auf demselben Kanal laufen. Der feste Kanal ist als `ESPNOW_DEFAULT_CHANNEL = 6` in allen `espnow_protocol.h`-Dateien definiert. Der Master-AP verwendet diesen Kanal automatisch wenn `USE_ACCESS_POINT = true`.

> **Selbstheilung bei Router-Kanalwechsel:** Ändert der Router nachträglich seinen WLAN-Kanal (Auto-Channel, Neustart nach Stromausfall, …), würde ein Node mit nur einmalig gespeichertem Kanal dauerhaft auf dem alten (jetzt falschen) Kanal hängen bleiben und den Master nie wiederfinden — der Master selbst merkt das automatisch, da er sich bei jedem Boot per `WiFi.begin(ssid, password)` frisch verbindet und so immer den aktuellen Kanal übernimmt. Steckdosen und Multisensor prüfen deshalb bei **jedem Boot** die bekannte SSID per Scan still im Hintergrund nach (kein erneuter Serial-Dialog nötig) und aktualisieren den gespeicherten Kanal bei Abweichung. Zusätzlich läuft alle 12 Stunden im laufenden Betrieb derselbe Scan erneut; ändert sich der Kanal während des Betriebs (ohne Node-Neustart), startet sich der Node selbst neu, um den neuen Kanal zu übernehmen.

#### 60-Sekunden-Timeout

Wenn bei allen Setup-Dialogen (Typ-Auswahl, Node-ID, Kanal) innerhalb von 60 Sekunden keine Eingabe erfolgt, wird ein Standardwert verwendet: Steckdose / ID=3 / Kanal 6.

---

### Serial-Kommandos

Baudrate **115200** · Zeilenende **LF** oder **CR+LF**

#### Steckdosen

| Kommando | Wirkung |
|---|---|
| `STATUS` | Aktuelle Relais-Bitmaske und Einzelstatus R1–R4 |
| `MASK <0–15>` | Alle Relais per Dezimalwert setzen (z. B. `MASK 5` → R1+R3 ein) |
| `R<1–4> <0\|1>` | Einzelrelais schalten (z. B. `R2 1`) |
| `NODEINFO` | Gespeicherten Node-Typ und Node-ID anzeigen |
| `NODECLEAR` | Node-Typ und ID löschen → Erststart-Dialog beim nächsten Neustart |
| `WIFIINFO` | Gespeicherte SSID und Kanal anzeigen |
| `WIFICLEAR` | WiFi-Konfiguration löschen → Kanal-Scan beim nächsten Neustart |

#### Multisensor

| Kommando | Wirkung |
|---|---|
| `WIFIINFO` | Gespeicherte SSID und Kanal anzeigen |
| `WIFICLEAR` | WiFi-Konfiguration löschen → Kanal-Scan beim nächsten Neustart |

---

## English

System documentation — master (ESP32-S3) with RS485 devices and wireless sensor/actuator nodes via ESP-NOW.

### Table of Contents

**System**
- [Overview](#overview)
- [ESP-NOW Protocol](#esp-now-protocol)

**Main Controller**
- [Hardware & Pinout](#hardware--pinout-1)
- [Valves & Pumps](#valves--pumps)
- [Features](#features)
- [Lighting Scheduler](#lighting-scheduler)
- [RS485 Bus](#rs485-bus-1)
- [RS485 Multisensor Tent](#rs485-multisensor-tent)
- [Lichtx4](#lichtx4-1)
- [Exhaust Fan](#exhaust-fan)
- [Lueftermodul](#lueftermodul-1)
- [Multimodul](#multimodul-1)
- [Analog Output Module](#analog-output-module)
- [CO2 Control](#co2-control)
- [Humidity Control](#humidity-control)
- [Room Light & DWC Timer](#room-light--dwc-timer)
- [Flow Sensor Calibration & Nozzle Monitoring](#flow-sensor-calibration--nozzle-monitoring)
- [Room Climate & VPD](#room-climate--vpd)
- [Data Log & Download](#data-log--download)

**Nodes (ESP-NOW)**
- [Multisensor](#multisensor-2)
- [Sockets](#sockets)
- [First Boot & Configuration](#first-boot--configuration)

**Reference**
- [Serial Commands](#serial-commands)

---

### Overview

```
Aeroponik Master (ESP32-S3 N16R8)
 │  W5500 Ethernet (LAN + WiFi in parallel) · solenoid valves MV1–MV4 (direct GPIO) · DS3231 RTC (I2C)
 │  MicroSD data logging · RS485 UART1 (MAX13487) · 2× DS18B20 · pressure sensor 0.5–4.5V
 │  Tank fill level: ultrasonic OR HX710B pressure sensor (switchable) · flow meter (nozzle monitoring)
 │  Mains power monitoring (UPS) · web interface, port 80
 │
 ├── RS485 (115200 baud) ⇄
 │    ├─ Multisensor              0x20 · FC03           Tent climate (CO₂/temp/humidity) + PPFD/spectrum (AS7341)
 │    ├─ Lichtx4                  0x40 · FC03/FC06      4× relay light control
 │    ├─ Exhaust fan (MARS Hydro) Addr. 6 · FC03/FC06   RPM/power, manual or automatic
 │    ├─ Analog output module     0x50 · FC03/FC06      2× 0–10V (light/fan alternative)
 │    └─ Lueftermodul / Multimodul 0x51 (+ optional 0x20/0x50) · FC03/FC06   4× tent fan PWM/tacho
 │
 └── ESP-NOW (channel 6) ⇄
      └─ Sockets                 NODE_TYPE_SOCKET · 0x03   4× relay, manual/API (incl. CO2 output)
```

The RS485 devices are hard-wired and polled cyclically — see [RS485 Bus](#rs485-bus-1). ESP-NOW nodes, by contrast, register with the master on startup and send periodic heartbeats; the master detects their online/offline status automatically.

LAN (W5500 via SPI2) and WiFi run simultaneously. The web interface on port 80 is reachable on both interfaces with no code changes.

### ESP-NOW Protocol

#### Packet Format

```c
typedef struct {
    uint8_t  node_id;      // 1–254  |  0 = master  |  255 = broadcast
    uint8_t  msg_type;     // message type
    uint8_t  seq;          // sequence number
    uint8_t  payload_len;  // 0–16
    uint8_t  payload[16];
    uint8_t  crc8;         // polynomial 0x07
} espnow_packet_t;         // 21 bytes
```

#### Node Types

| Constant | Value | Description |
|---|---|---|
| `NODE_TYPE_LIGHT` | `0x01` | LED light control (PWM, light meter) |
| `NODE_TYPE_SENSOR` | `0x02` | Multisensor — air + spectrum |
| `NODE_TYPE_SOCKET` | `0x03` | Socket — general manual control |
| `NODE_TYPE_LIGHT_SSR` | `0x05` | Light SSR — automatic via lighting scheduler (legacy, see [Sockets](#sockets)) |

> The former separate ESP-NOW spectrum node (`NODE_TYPE_SPEKTRUM 0x04`, subtype `SUBTYPE_LICHT_SUMMARY 0x20`) has been removed — PPFD/spectrum now come via RS485 (`0x20`, Multisensor or Multimodul).

#### Message Types

| Type | Value | Direction | Meaning |
|---|---|---|---|
| `MSG_REGISTER` | `0x01` | Node → master | Node registers |
| `MSG_REGISTER_ACK` | `0x02` | Master → node | Registration confirmed |
| `MSG_HEARTBEAT` | `0x03` | Node → master | Keep-alive / online check |
| `MSG_HEARTBEAT_ACK` | `0x04` | Master → node | Heartbeat confirmed |
| `MSG_SENSOR_DATA` | `0x10` | Node → master | Measurement data (subtype in payload[0]) |
| `MSG_COMMAND` | `0x20` | Master → node | Control command |
| `MSG_OTA_START` | `0x40` | Master → node | OTA update (AP password in payload[0..7]) |
| `MSG_OTA_ACK` | `0x41` | Node → master | OTA is starting |

#### Command Subtypes (MSG_COMMAND · payload[0])

| Subtype | Value | payload[1+] |
|---|---|---|
| `CMD_SET_PWM` | `0x01` | uint16 BE — PWM value 0–1000 (= 0–100.0 %) |
| `CMD_SET_ONOFF` | `0x02` | Bitmask: bit0=R1 bit1=R2 bit2=R3 bit3=R4 |
| `CMD_SET_INTERVAL` | `0x10` | uint16 — measurement interval in seconds |
| `CMD_REQUEST_DATA` | `0x11` | – (triggers an immediate measurement) |
| `CMD_REQUEST_RAWSPEC` | `0x12` | – (next measurement sends raw channels) |

#### Sensor Data Subtypes (MSG_SENSOR_DATA · payload[0])

| Subtype | Value | payload content |
|---|---|---|
| `SUBTYPE_AIR` | `0x01` | [1..2] temp×100 int16BE · [3..4] RH×100 uint16BE · [5..6] eCO₂ ppm · [7..8] TVOC ppb |
| `SUBTYPE_LIGHT` | `0x02` | [1..2] PPFD×10 uint16BE · [3..16] spectral channels 415–680 nm |
| `SUBTYPE_RELAY_STATUS` | `0x10` | [1] relay bitmask (bit0–3 = R1–R4) |

---

### Hardware & Pinout

**MCU:** ESP32-S3 N16R8 (16 MB flash, 8 MB OPI-PSRAM)

> **Reserved GPIOs:** 0, 45, 46 (strapping, boot-critical) · 19, 20 (USB-OTG) · 26–32 (flash) · 33–37 (OPI-PSRAM) · 38–42 (JTAG) · 43, 44 (UART0). GPIO3 is also a strapping pin, but its role (JTAG signal source) only applies if a specific eFuse bit is set (default: no) — deliberately used for the flow sensor below instead of the more boot-critical GPIO46.

> **Hardware revision:** As of the SMD board (see [`SmdVersion_Pinbelegung.pdf`](../../DokuEtc/SmdVersion_Pinbelegung.pdf)), there is no longer an I2C GPIO expander (PCF8574AP) — valves, main pump, and circulation pump are direct GPIOs, and the AHT21B shares the main bus with the RTC instead of having its own second I2C bus.

| GPIO | Constant | Peripheral |
|---|---|---|
| `1` | `RS485_RX` | UART1 RX ← MAX13487 Receive Out |
| `2` | `RS485_TX` | UART1 TX → MAX13487 Data In |
| `3` | `FLOW_SENSOR_PIN` | Flow meter (YF-S401-like, Hall pulse output, push-pull) — see [Flow Sensor Calibration & Nozzle Monitoring](#flow-sensor-calibration--nozzle-monitoring) |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (reservoir, plant, optional internal temperature), assigned by ROM address (see [Features](#features)) |
| `5` | `PRESSURE_PIN` | Analog — pressure sensor 0.5–4.5 V ratiometric (can be disabled in the web UI, see [Pressure Sensor](#pressure-sensor-pump-pressure)) |
| `6` | `GEHAEUSE_FAN_PIN` | Enclosure fan, on/off (no PWM), temperature-controlled via the internal DS18B20 |
| `7` | `VENTIL2_PIN` | Solenoid valve reservoir 2 (direct GPIO) |
| `8` | `VENTIL1_PIN` | Solenoid valve reservoir 1 |
| `9` | `VENTIL4_PIN` | Return valve |
| `10` | `VENTIL3_PIN` | Solenoid valve reservoir 3 |
| `11` | `SPI_MOSI` | SPI2 MOSI — W5500 + MicroSD |
| `12` | `SPI_SCK` | SPI2 SCK |
| `13` | `SPI_MISO` | SPI2 MISO |
| `14` | `SPI_CS_W5500` | W5500 chip select |
| `15` | `ULTRASONIC_TRIG` | HC-SR04 trigger |
| `16` | `ULTRASONIC_ECHO` | HC-SR04 echo (3.3 V, direct connection) |
| `17` | `I2C_SCL` | I2C bus: DS3231, AHT21B, optional ENS160/GP8403 |
| `18` | `I2C_SDA` | I2C bus: DS3231, AHT21B, optional ENS160/GP8403 |
| `21` | `WSINT` | W5500 interrupt |
| `38` | `NETZ_OK_PIN` | Mains power monitoring — HIGH = mains present, LOW = running on UPS (2000 ms debounced) |
| `39` | `SPI_CS_SD` | MicroSD chip select |
| `47` | `UMWAELZPUMPE_GPIO` | Circulation pump (reservoir recirculation) |
| `48` | `PUMPE_GPIO` | Main pump |

The driver polarity of every direct output (valves 1–4, main pump, circulation pump) is switchable via `OUTPUT_ACTIVE_LOW` in `pinout.h` (0 = active-high, default; 1 = active-low for inverted MOSFET drivers).

#### I2C Devices (SDA=18, SCL=17)

| Address | Chip | Function |
|---|---|---|
| `0x38` | AHT21B | Room climate reference sensor (default address of the Adafruit_AHTX0 library) |
| `0x52` | ENS160 | Optional eCO₂ sensor for room air (e.g. DFRobot SEN0515, combo module with AHT21), see [Room Climate & VPD](#room-climate--vpd) |
| `0x58` | GP8403 | Optional 2-channel 0–10V DAC, see [Analog Output Module](#analog-output-module) |
| `0x68` | DS3231 | Real-time clock + onboard EEPROM (AT24C32) |

### Valves & Pumps

The four solenoid valves, the main pump, and the circulation pump are direct GPIO outputs (see [Hardware & Pinout](#hardware--pinout-1)) — no I2C expander.

| Constant | Valve | Function |
|---|---|---|
| `VENTIL1_PIN` | MV1 | Inlet reservoir 1 |
| `VENTIL2_PIN` | MV2 | Inlet reservoir 2 |
| `VENTIL3_PIN` | MV3 | Inlet reservoir 3 |
| `VENTIL4_PIN` | MV4 | Return / drain |

#### Main Pump & Circulation Pump

The main pump (`PUMPE_GPIO`) switches on automatically whenever any reservoir is actively watering (valve open) — regardless of whether the normal or batch timing mode is active (see [Valve Control](#features)).

The circulation pump (`UMWAELZPUMPE_GPIO`, reservoir recirculation) runs a configurable run/pause cycle (`laufzeit_s` / `pausenzeit_min`) whenever any reservoir is enabled at all — regardless of whether that reservoir is currently watering or just pausing between cycles. If no reservoir is enabled, it stays off.

### Features

#### Network

W5500 (Ethernet) and WiFi run simultaneously via the lwIP stack. On Ethernet link-up, **NTP** is fetched automatically and the RTC is set (threshold: ±60 s deviation, resynced every 60 min). The web interface is available on both IPs with no code changes.

Without a router: the master starts as an **AP on channel 6** (`ESPNOW_DEFAULT_CHANNEL`). Nodes in "no WiFi" mode use the same channel.

#### Valve Control

Three independent irrigation circuits with configurable open time (sec.) and pause interval (min., separately configurable for the light and dark phases). Optional return pre-flush via MV4 (configurable pre-flush time). In "same times for all" mode, a single shared batch run replaces the three independent timers (pre-flush once before the first active reservoir, then all active reservoirs directly in sequence, one shared pause).

After a restart, watering starts **immediately** instead of first waiting out the full pause time (up to 60 min).

**Manual single cycle:** On the Aeroponik page, a single watering cycle can be triggered manually per reservoir — the valve opens once for the currently valid open time (light or dark phase) and closes again automatically (`/api/ventile/dose/start`, `/status`, `/cancel`). Normal automation is paused meanwhile.

**Output test:** The direct output test (wiring check, flow sensor calibration) no longer interrupts a running watering cycle — if a valve/pump is currently active automatically, test mode only becomes active once the cycle has finished on its own. The UI shows the waiting state (`GET /api/output/test`).

#### Tank Monitoring (Truncated Cone)

Fill level via either an **ultrasonic sensor** (HC-SR04/RCWL-1670/JSN-SR04T) or a **pressure sensor** (HX710B-based 0–40kPa modules, 2-wire protocol similar to HX711, no I2C) — switchable via configuration; both sensor types share the same two GPIOs (`ULTRASONIC_TRIG`/`ULTRASONIC_ECHO` resp. `HX710_SCK`/`HX710_DOUT`), no board change needed when switching. Conversion distance/raw pressure value → water height → volume, accounting for the conical tank geometry (configurable top/bottom radii and height). For ultrasonic, a median filter over the last 5 readings smooths out single outliers.

#### DS18B20 — Role Assignment by ROM Address

The DS18B20 sensors on the shared 1-Wire bus (reservoir, plant, optional internal temperature for the enclosure fan) are assigned to a role via their unique 64-bit ROM address, rather than relying on bus-scan order (which changes with swapped or differently-lengthed cables). As long as a role isn't assigned, the firmware falls back to the old index order (0=reservoir, 1=plant, 2=internal temperature).

#### Pressure Sensor (Pump Pressure)

The analog pressure sensor on `PRESSURE_PIN` (GPIO5) can be switched on/off on the Aeroponik page (own EEPROM block, magic `0x19`) — for setups without a sensor or with a defective one. When disabled, the pressure card disappears from the home page and no pressure is measured (`/api/drucksensor`, `/api/drucksensor/save`).

#### Enclosure Fan

A simple on/off switch (no PWM) on the controller enclosure, controlled via the internal DS18B20. Hysteresis: on at/above `temp_max`, off at/below `temp_min` — in between, the last state is held so the fan doesn't chatter.

#### Mains Power Monitoring

`NETZ_OK_PIN` detects a mains power outage (running on the UPS) — HIGH = mains present, LOW = mains outage, debounced 2000 ms against brief voltage dips. The status is shown in the web interface and can optionally trigger a [notification](#notifications).

#### Notifications

Three independently enabled channels — CallMeBot tends to be unreliable at times, so Telegram and/or email keep working in parallel if one channel is down. A global switch (`global_enabled`) overrides all channels and alerts; each alert can also be enabled separately:

| Channel | Setup |
|---|---|
| WhatsApp | Via [CallMeBot](https://www.callmebot.com/blog/free-api-whatsapp-messages/) — phone number + API key, obtained once via a WhatsApp activation message |
| Telegram | Create a bot with @BotFather (gives a token), find your own chat ID e.g. via @userinfobot — sent via the Telegram Bot API (HTTPS) |
| Email | Real SMTP (implicit TLS, port 465 typical) through your own mailbox, e.g. Gmail with an app password (not the regular account password) — no third-party service needed |

| Alert | Trigger |
|---|---|
| Power outage | See [Mains Power Monitoring](#mains-power-monitoring) |
| Sensor failure | AHT21 / RTC / DS18B20 unreachable |
| Humidity | Tent humidity (RS485 multisensor) outside `feuchte_min`/`feuchte_max` |
| Temperature | Tent temperature (RS485 multisensor) outside `temperatur_min`/`temperatur_max` |
| Reservoir nearly empty | Reservoir fill level below `tank_min_prozent` |
| Tent fan RPM | Actual RPM deviates from the expected value, see [Lueftermodul](#lueftermodul-1) |
| Nozzle flow | Target/actual deviation per reservoir, see [Nozzle Monitoring](#flow-sensor-calibration--nozzle-monitoring) |

Each channel has its own rate limit (30 s) against accidental spamming; a dedicated test button per channel in the configuration verifies its credentials independently of the others.

#### SD Logging

Every 5 minutes, all measurement and control values are written to CSV files following the scheme `/YYYY-MM-DD.csv`, including room/tent climate, fan and light status. Details, full column list, and download/delete via the web interface: see [Data Log & Download](#data-log--download).

#### Node OTA

An OTA update can be triggered for any registered node via the API. The master briefly opens a helper AP (`Aeroponik-OTA`, time-limited random password); the node connects and downloads the firmware over HTTP.

### Lighting Scheduler

Simulates sunrise and sunset over up to 4 SSRs. Sends `CMD_SET_ONOFF` every 30 seconds — on mask change — to all online nodes of type `NODE_TYPE_LIGHT_SSR`. Requires a working RTC.

#### Parameters (EEPROM, Magic 0xD5)

| Field | Default | Unit | Description |
|---|---|---|---|
| `enabled` | false | bool | Scheduler active |
| `dawn_start` | 360 | min since 0:00 | Sunrise start (6:00) |
| `dawn_end` | 480 | min since 0:00 | Sunrise end / full brightness (8:00) |
| `dusk_start` | 1080 | min since 0:00 | Sunset start (18:00) |
| `dusk_end` | 1200 | min since 0:00 | Sunset end / dark (20:00) |
| `num_ssr` | 4 | 1–4 | Number of SSRs used |

#### Relay Mask — Calculation Logic

```c
// t = linear progress 0.0→1.0 through sunrise/sunset
active = ceilf(t × num_ssr);        // even steps, first/last step exactly at the window edge
mask   = (active == 0) ? 0x00 : (uint8_t)((1 << active) - 1);
```

> **Note:** `round()` was replaced with `ceil()`: with `round()`, the first and last step were only half as wide as the middle ones (the bin edges at 0 and n only cover `[0, 0.5/n)` and `[1−0.5/n, 1]`) — causing the first SSR to switch on too late and the last one to switch off too early.

Example `num_ssr=4`, sunrise 6:00–8:00 (120 min):

| Time Range | t | Active SSRs | Mask |
|---|---|---|---|
| before 6:00 | — | 0 | `0x00` |
| 6:00 – 6:30 | 0.0 – 0.25 | 1 | `0x01` (SSR1) |
| 6:30 – 7:00 | 0.25 – 0.5 | 2 | `0x03` (SSR1+2) |
| 7:00 – 7:30 | 0.5 – 0.75 | 3 | `0x07` (SSR1+2+3) |
| 7:30 – 8:00 → 18:00 | 0.75–1.0 / full | 4 | `0x0F` (all) |
| 18:00 – 20:00 | 1.0 → 0.0 | 4 → 0 | mirrored |
| after 20:00 | — | 0 | `0x00` |

### RS485 Bus

A second communication path, independent of the ESP-NOW network, for hard-wired devices in the tent. Physically a shared two-wire bus via a **MAX13487** (automatic transmit/receive direction, no manual DE/RE switching needed) on `RS485_TX` (GPIO1) / `RS485_RX` (GPIO2), **115200 baud, 8N1** — the master and all of our own slaves (Lichtx4, Multisensor, Multimodul, ProtoRS485).

> **Transition for a Lichtx4 not yet reflashed:** An already installed Lichtx4 with old firmware still runs at 9600 baud. For this case there is the define `LICHTX4_LEGACY_9600` — it is set in an optional file `Aeroponik/src/local_config.h` that is excluded via `.gitignore` (`#define LICHTX4_LEGACY_9600`). When active, the master briefly switches to 9600 baud only for the Lichtx4 transactions and back afterwards. Without the file (the repo default), the whole bus runs at 115200 baud.

> **Historical note:** Until recently, a MARS Hydro *iControl* hub also ran on the same bus as a second Modbus master (third-party control of the exhaust fan via the vendor app). This device has been fully removed — the Aeroponik master is now the sole bus master. So that the fan accepts it as master, the master re-creates the iControl hub's periodic broadcast, see [Exhaust Fan](#exhaust-fan).

#### Protocol

Simplified Modbus RTU: **FC03** (Read Holding Registers) for reading, **FC06** (Write Single Register) for writing. CRC16 per the Modbus standard (polynomial `0xA001`). A lean state-machine model (`mb_state_t`) polls the bus participants in turn; only one request outstanding at a time, response timeout **2000 ms**.

After every send, the transceiver's own transmit echo is actively drained (`flush()` + a short pause + clearing any leftover bytes) before waiting for the response — needed because the MAX13487 on all of our boards has RE tied permanently to receive, so it always reflects its own transmission back onto RX.

> **Resync after a slave reboot:** Lueftermodul/Multimodul (`0x51`) have no flash memory for PWM/analog values — an independent reset of the slave (power blip, etc.) resets them to 0 without the master necessarily noticing a bus interruption (too short an outage). Register 8 of the fan register map therefore holds a **boot ID**, re-randomized on every boot (`rp2040.hwrand32()`); if it changes versus the last poll, the master reliably knows "slave just restarted" and automatically rewrites the last commanded fan PWM values **and** both GP8403 analog channels (same physical board, see [Multimodul](#multimodul-1)). Lichtx4 doesn't need this — its relay mask is already persisted node-side via `Preferences` and restored on its own boot.

#### Bus Participants

| Address | Device | Access | Poll Interval |
|---|---|---|---|
| `0x20` | RS485 multisensor (tent climate + PPFD) | FC03 read | 15 s |
| `0x40` | Lichtx4 (4× relay light control) | FC03 read + FC06 write | 15 s (+ immediately on mask change) |
| `6` | Exhaust fan — MARS Hydro | FC03 read + FC06 write | 3 s |
| `0x50` | Analog output module (2× 0-10V) | FC03 read + FC06 write | 15 s |
| `0x51` | Lueftermodul / Multimodul (4× tent fan PWM/tacho) | FC03 read + FC06 write | 3 s |

Lichtx4, the MARS fan, and the analog module are only polled when actually selected as the active output per the [device configuration](#analog-output-module). The RS485 multisensor is always polled; the Lueftermodul/Multimodul likewise, whenever enabled in the configuration (see [Lueftermodul](#lueftermodul-1)).

#### Priority on Concurrent Demand

Pending write commands take priority over cyclic reads. Order: light write → fan (MARS) write → analog write (channel 1, then 2) → Lueftermodul write → multisensor read → light read → fan (MARS) read → analog read → Lueftermodul read.

### RS485 Multisensor Tent

Sensor module hard-mounted in the tent at RS485 address `0x20`, providing climate data and the **PPFD light sensor plus 8-channel spectrum in a single device** — so this deployment has no need for a separate ESP-NOW spectrum node.

#### Registers Read (FC03, 14 registers starting at register 0)

| Field | Unit | Description |
|---|---|---|
| `co2_ppm` | ppm | CO₂ concentration in the tent |
| `temp_c` | °C | Tent air temperature |
| `hum_pct` | %rH | Tent air humidity |
| `ppfd` | µmol/m²/s | Photon flux density |
| `channels[8]` | — | Spectral channels |
| `gain`, `status` | — | Sensor diagnostics |

**PPFD calculation:** physical model from the 8 spectral channels (channel responsivity per the AS7341 datasheet, photon energy per center wavelength, normalized to GAIN_1X) with an empirical calibration factor determined with the Licht-Meter handheld against a reference meter. Replaces the former MLR regression, which was noticeably off for spectra unlike the training lamps. Same calculation in Multisensor and Multimodul.

This sensor's tent temperature, humidity, and CO₂ readings are the basis for the [exhaust fan auto modes](#exhaust-fan), [CO2 control](#co2-control), and the [VPD calculation](#room-climate--vpd).

### Lichtx4

RS485 light control. Standalone firmware (`Lichtx4`) with 4 relay/SSR outputs, connected exclusively via RS485 — **no ESP-NOW**. Has fully replaced the previously planned ESP-NOW light-SSR [socket node](#sockets) for light control.

#### Registers

| Address | Register | Access | Content |
|---|---|---|---|
| `0x40` | 0 | FC03 read | Current relay bitmask (bit0–3 = output 1–4) |
| `0x40` | 0 | FC06 write | Set new relay bitmask |

Driven by the existing [lighting scheduler](#lighting-scheduler) — instead of an ESP-NOW `CMD_SET_ONOFF` packet, the master writes directly via FC06 to register 0 on mask change.

### Exhaust Fan

MARS Hydro (RS485). Factory-built fan controller board, RS485 address `6`, originally controlled via the MARS Hydro *iControl* app. The master can optionally take over control entirely.

> **Safety default:** `enabled=false` — without explicit activation the master sends no write commands, the fan is left untouched. Only after activation in the configuration does the master take over control. (The iControl hardware has since been fully removed.)

#### Registers

| Register | Access | Content |
|---|---|---|
| 0–16 | FC03 read | Always the full set: start 0, 17 registers (like the original controller) |
| 11 | (in block) | Speed (RPM) |
| 15 | FC06 write | Enable bit — set to 1 **before** every power change, otherwise the fan only stores the value in its register but doesn't physically ramp up |
| 16 | FC03 read (in block) / FC06 write | Current / target power 0–100 % |

> **Master detection (experiment):** The fan apparently doesn't recognize a bus master by mere polling, but by the periodic broadcast of the real iControl controller. The master therefore re-sends the same broadcast every 6 s (address 0, FC10, start 1001, 26 registers, `reg[8]=7` as signature; no response expected). If the device address is suspected to have shifted, a diagnostic scan (`POST /api/rs485/scanfan`, JSON `{"from":1,"to":20}`) queries every address in the range via FC03 and returns the raw responses — hits on our own addresses (`0x20`, `0x40`, `0x50`, `0x51`) are normal.

#### Control Modes

| Mode | Source | Behavior |
|---|---|---|
| Manual | — | Fixed power (%) |
| Auto — humidity | Tent humidity (RS485 multisensor) | Linear ramp between `hum_min` (minimum speed) and `hum_max` (100 %) |
| Auto — temperature, absolute | Tent temperature | Ramp between `temp_min`/`temp_max`; optional **differential cap**: if the tent−room difference is below `temp_cap_diff`, only the minimum speed is driven regardless of the ramp (prevents full blast when the room is nearly as warm as the tent) |
| Auto — temperature, differential | Tent temperature − room temperature | Ramp directly over the temperature differential between `temp_min`/`temp_max` |

A shared minimum speed (`min_speed`) applies to both auto modes. The differential modes require the [room climate sensor](#room-climate--vpd); if it is unavailable, the last target power remains unchanged.

#### Configuration (EEPROM, Magic 0x92)

`mode` · `manual_percent` · `min_speed` · `hum_min/max` · `temp_min/max` · `temp_mode` (absolute/differential) · `temp_cap_diff` · `enabled`

### Lueftermodul

> **Standalone Lueftermodul project removed:** The former separate firmware `Lueftermodul` (fans only, no optional I2C peripherals) has been deleted — [Multimodul](#multimodul-1) already covers the plain fan-only use case completely (MS2/analog simply stay inactive when the associated I2C peripheral is missing). Even a board that only drives fans is therefore flashed with the Multimodul firmware — the register/pinout layout below still applies unchanged.

Firmware on an RP2040-Zero (Waveshare), driving up to 4 PC fans mounted directly on the tent (Arctic, 600–3000 RPM, PWM-controlled, 0 RPM below 5 % PWM) with individual or shared speed in 10 % steps. RS485-only, address `0x51`.

#### Pinout

| Function | GPIO |
|---|---|
| RS485 TX (DI) / RX (RO) | 28 / 29 (UART0) |
| PWM fan 1–4 | 0, 2, 4, 6 (25 kHz) |
| Tacho fan 1–4 | 1, 3, 5, 7 (edge interrupt) |
| WS2812 status LED | 16 |

Each fan's PWM and tacho pin are deliberately adjacent (simplifies the PCB layout — both signals of one fan connector sit next to each other).

#### Registers

| Register | Access | Content |
|---|---|---|
| 0–3 | FC06 write / FC03 read | PWM setpoint fan 1–4, 0–100 % |
| 4–7 | FC03 read (read-only) | Actual speed fan 1–4, RPM |
| 8 | FC03 read (read-only) | Boot ID — re-randomized on every start (`rp2040.hwrand32()`), see [RS485 Bus](#rs485-bus-1) |

#### Master-Side Monitoring

The master compares the reported actual RPM against an expected RPM derived from the PWM setpoint (linear interpolation between 600 RPM at 5 % and 3000 RPM at 100 %, 0 RPM below 5 %) with a configurable tolerance, and reports outliers via the [notifications](#features).

### Multimodul

Standalone firmware (`Multimodul`) on the same RP2040-Zero base as the [Lueftermodul](#lueftermodul-1) — identical pinout, plus a second I2C bus (I2C1, SDA=GPIO10/SCL=GPIO11) for optional add-on hardware:

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — the same sensors as in the [Multisensor](#multisensor-2) node. When present, the board additionally answers RS485 address `0x20` with the same 14-register layout as the [RS485 multisensor](#rs485-multisensor-tent).
- **GP8403** (I2C `0x58`, 2-channel 0–10V DAC) — when present, the board additionally answers RS485 address `0x50` with the same 2-register layout as the [analog output module](#analog-output-module).

On boot, the firmware scans the I2C bus; only devices actually found are initialized and have their RS485 address activated. If a device is missing, its address simply stays unanswered on the bus — indistinguishable, from the master's point of view, from a separate Multisensor/analog module that was never installed. This means **no code change is needed on the master**: it already polls `0x20`, `0x50`, and `0x51` independently and already treats a missing response as "device offline".

Depending on what's populated, a Multimodul can therefore be a plain fan module, a fan module with tent climate sensing, a fan module with a 0–10V output, or all of it at once — on the same RS485 bus, with no separate firmware variants.

> **Boot order:** `rs485_slave_init()` deliberately runs at the very start of `setup()`, **before** the I2C scan/sensor init — otherwise the DI line (GP28) would stay unconfigured/high-impedance for as long as the I2C scan or `scd41_init()`/`gp8403_init()` run (which can take arbitrarily long if an I2C transaction hangs), leaving the bus without a defined idle level. The firmware also logs the reset reason at boot (`rp2040.getResetReason()`: power-on, watchdog, brownout, glitch, …) — useful for diagnosing unexplained reboots, since the project itself doesn't use a software watchdog.

> **SCD41 and AS7341 independent:** CO₂/temperature/humidity (SCD41) are written to the `0x20` registers right after each measurement, independently of the AS7341. If an AS7341 measurement hangs for more than 6 s, only the AS7341 is reinitialized (no I2C bus reset that would take the SCD41 down with it); after 3 failed attempts it is disabled for 10 min (light values = 0, gain = `0xFF`, status bit 1 = 0) and then retried automatically — the board, SCD41, fans, and analog output stay online the whole time.

### Analog Output Module

0-10V. 2-channel RS485-to-0-10V converter (ATO), standard Modbus RTU. An optional alternative to Lichtx4 (channel 2, stepless brightness instead of relay steps) and/or the MARS Hydro fan (channel 1) — e.g. for generic fans or dimmers with a 0-10V control input. The same register layout is also served by the [Multimodul](#multimodul-1)'s optional GP8403 output.

> **One-time hardware setup:** The module ships at address `0x01` / 4800 baud by default. Before installing it, it must be reconfigured once **standalone** (not on the shared bus) via a USB-RS485 adapter and the ATO configuration software: address → `0x50`, output type of both channels → 0-10V (register 0x0055 plus the physical solder jumpers set to "V"), baud rate → that of the bus.
>
> **Note on baud rate:** The bus now runs at **115200 baud** (formerly 9600). An ATO module still set to 9600 will no longer respond — it must be switched to 115200 if the ATO software offers that. Otherwise the [Multimodul](#multimodul-1)'s GP8403 output takes over address `0x50`.

#### Registers

| Register | Access | Content |
|---|---|---|
| 0x0000 | FC03 read + FC06 write | Channel 1 (fan), 0–4095 (12-bit) = 0–10V |
| 0x0001 | FC03 read + FC06 write | Channel 2 (light), 0–4095 (12-bit) = 0–10V |

Target percentages (0–100 %) from the fan ramp or lighting schedule are linearly mapped to 0–4095 (`round(percent / 100 × 4095)`).

For testing without the real hardware, the separate `ProtoRS485` project (ESP32-C3 + WS2812 strip) simulates this module on the bus — see [ProtoRS485/README.md](../../ProtoRS485/README.md).

#### Device Configuration (Setup, Not a Runtime Toggle)

Under Configuration → Devices you choose which device actually drives the light and fan (Lichtx4/MARS Hydro or the analog module) — deliberately a setup-time decision, not a runtime switch. This has two effects: only the selected device receives control commands, and only devices actually present are polled on the RS485 bus at all (the multisensor is unaffected by this and is always polled).

For the light output, the selection also changes the control characteristic: Lichtx4 keeps using the stepped relay ramp ([lighting scheduler](#lighting-scheduler)), while the analog module uses a stepless brightness ramp over the same time windows.

### CO2 Control

Hysteresis control of a CO₂ output on any **ESP-NOW socket node**, driven by the CO₂ reading from the [RS485 multisensor](#rs485-multisensor-tent).

#### Logic

- CO₂ ≤ `co2_min` → target relay **ON**
- CO₂ ≥ `co2_max` → target relay **OFF**
- in between: last state is held (classic hysteresis, no chattering)

Only the configured relay bit (`target_relay_bit`, 0–3) of the selected node (`target_node_id`) is switched — the other bits of that node's relay mask remain untouched.

#### Configuration (EEPROM, Magic 0x7C)

`enabled` · `target_node_id` · `target_relay_bit` · `co2_min` (default 800 ppm) · `co2_max` (default 1200 ppm)

A status card appears on the home page only when the feature is enabled.

### Humidity Control

Exactly the same hysteresis logic as [CO2 Control](#co2-control), just using tent humidity (RS485 multisensor) as the control variable instead of CO₂ — also on any ESP-NOW socket node.

#### Logic

- Humidity ≤ `humid_min` → target relay **ON** (humidify)
- Humidity ≥ `humid_max` → target relay **OFF**
- in between: last state is held

#### Configuration (own EEPROM block, Magic 0x44)

`enabled` · `target_node_id` · `target_relay_bit` · `humid_min` (default 50.0 %) · `humid_max` (default 65.0 %) — stored internally as %×10 without a float in EEPROM, shown in the UI as a normal percentage with one decimal.

Own status card on the home page, visible only when enabled.

### Room Light & DWC Timer

Two simple switching features that — like [CO2](#co2-control) and [humidity control](#humidity-control) — switch exactly one output (`target_relay_bit`, 0–3) of an **ESP-NOW socket node** (`target_node_id`); the node's other relay bits are left untouched.

#### Room Light (EEPROM, Magic 0x3F)

A purely **manual** on/off switch, with no automation, hysteresis, or schedule. The output is selected in the climate tab; once one is selected, a switch appears on the home page. The on/off state isn't stored separately but read live from the target node's reported relay mask — so the UI and the actual relay state can't drift apart. Endpoints: `GET /api/raumlicht`, `POST /api/raumlicht/save`, `POST /api/raumlicht/toggle`.

#### DWC Timer (EEPROM, Magic 0x3A)

A simple on/off lighting timer for a separate DWC system, like an ordinary household timer switch (no sunrise/sunset like the [lighting scheduler](#lighting-scheduler)): `enabled` · `target_node_id` · `target_relay_bit` · `on_min` / `off_min` (minutes since midnight; `off_min < on_min` = across midnight).

### Flow Sensor Calibration & Nozzle Monitoring

Flow meter (YF-S401-like, Hall pulse output with a push-pull driver, no pull-up needed) in the shared supply line **before** the valve manifold — so it sees the flow of all 4 valves in turn, since per [valve control](#features) only one is ever open at a time anyway. An interrupt on `FLOW_SENSOR_PIN` (GPIO3) just counts pulses; conversion to ml is done via a one-time calibration.

#### Calibration (Configuration → Valves)

1. **"Start calibration"** — pump on (GPIO48), pressure is shown live. Uses the same output direct-test as the wiring check (which automatically pauses normal valve/pump control).
2. **"Purge"** — valve open/close, as often as needed, until there's no air left in the system (air would skew the pulse count). "Start" is only possible after at least one completed purge cycle.
3. **"Start"** — opens the return valve (MV4), counts pulses from then on (shown live).
4. **"Stop"** — valve closes, pulse count is frozen.
5. Enter the measured amount in ml (e.g. collected in a measuring cup) → **"Calculate"** derives pulses/liter and saves it permanently (own EEPROM block).

#### Nozzle Monitoring (`flowmonitor.cpp`)

Observes the valve state (`behaelterZustand[]`) from the outside: when a reservoir switches to the "active" state, the pulse counter is cleared (discarding leftovers from the MV4 pre-flush phase); when it leaves that state, the amount is calculated and added to running totals (per reservoir + overall, RAM-only since the last restart — a true lifetime total can be reconstructed from the SD log if needed).

Each reservoir keeps a **target value** (ml/s):

- If none is set yet (0, or reset via "Clear targets"), the system automatically adopts the **next completed watering cycle** as the new reference — no manual calculation needed.
- If one is set and the target/actual comparison is enabled, every cycle is compared with a configurable tolerance (%); on deviation below (clogged nozzle) or above (leak), a [notification](#notifications) goes out over all active channels.
- "Clear targets" resets all three values to 0 — handy after cleaning the nozzles, so the old (now valid again) values aren't wrongly flagged as a deviation.

SD log columns: see [Data Log & Download](#data-log--download).

### Room Climate & VPD

#### AHT21B

Sits on the main I2C bus (`I2C_SDA`/`I2C_SCL`, GPIO18/17) together with the RTC and the optional GP8403 — see [I2C Devices](#i2c-devices-sda18-scl17). The sensor measures **room air outside the tent** as a reference.

> **Known library bug:** Adafruit_AHTX0 sends the AHT10 calibration command `0xE1` by default. AHT20/AHT21(B) chips instead expect `0xBE` — otherwise the humidity channel reports a constant ~100 % while temperature is already correct. The firmware manually re-sends the correct command (`0xBE, 0x08, 0x00`) right after `aht21.begin()`.

#### ENS160 (optional)

Optional eCO₂ sensor for room air on the same I2C bus (address `0x52`, e.g. DFRobot Gravity SEN0515 — ENS160 + AHT21 combo module). Detected automatically at boot; since the ENS160 needs current temperature/humidity for compensation, it is continuously fed the AHT21 readings (`setTempAndHum()`). The eCO₂ value (ppm) appears in the status API (`ens160Ok`, `ens160Co2`) and in the web interface. eCO₂ is an estimate derived from VOC measurement, not a true NDIR CO₂ reading like the SCD41 in the tent.

#### VPD (Vapor Pressure Deficit)

Calculated from **tent** temperature and humidity (RS485 multisensor) using the Tetens formula, identical server-side to the UI calculation:

```c
SVP = 0.6108 × exp(17.27 × T / (T + 237.3))   // kPa
VPD = SVP × (1 − RH / 100)
```

Displayed between humidity and CO₂ in the measurements card; room and tent temperature together are also the basis for the [fan differential modes](#exhaust-fan).

### Data Log & Download

One CSV file per day (`/YYYY-MM-DD.csv`) on the MicroSD card, write interval **5 minutes**. Values from a currently unreachable source (room sensor, RS485 multisensor, fan offline) are written as an **empty cell** instead of a misleading zero.

#### Columns

| Column | Source |
|---|---|
| `Zeit` | RTC |
| `TempVorrat_C`, `TempPflanze_C` | 2× DS18B20 |
| `Druck_bar` | Pressure sensor |
| `Fuellstand_%`, `Volumen_L` | Ultrasonic + tank geometry |
| `RaumTemp_C`, `RaumFeuchte_%` | AHT21B (room climate) |
| `ZeltTemp_C`, `ZeltFeuchte_%`, `ZeltCO2_ppm` | RS485 multisensor |
| `ZeltVPD_kPa` | Calculated |
| `LichtMaske` | Lichtx4 relay bitmask |
| `LuefterLeistung_%`, `LuefterRPM` | Exhaust fan |
| `CO2Ausgang` | CO2 control relay state (0/1) |
| `HumidAusgang` | Humidity control relay state (0/1) |
| `FlowBehaelter1_ml` … `FlowBehaelter3_ml` | Nozzle monitoring — total per reservoir since restart |
| `FlowGesamt_ml` | Nozzle monitoring — total of all reservoirs since restart |

> **Automatic migration:** If the firmware encounters an existing daily file with the old, shorter column format (prior to this extension) when writing, it renames it to `_alt.csv` (data is preserved) and starts a new file with the full header — no mixing of different column counts within one file.

#### Download & Delete (Configuration → Data Log)

| Endpoint | Method | Function |
|---|---|---|
| `/api/logs` | GET | List of all `*.csv` and `*.txt` files with file size |
| `/api/logs/download?file=NAME` | GET | Streams the file with `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Deletes the file (UI asks for confirmation first) |
| `/api/logs/delete-all` | POST | Deletes all `*.csv`/`*.txt` files on the SD card, response contains the count (UI asks for confirmation first) |

Filenames are checked server-side against path traversal (no `/` or `..` allowed).

---

### Multisensor

**MCU:** ESP32-C3 · Node ID: `0x02` (compile-time) · either `ESP-NOW · NODE_TYPE_SENSOR 0x02` or `RS485 · addr. 0x20`

The same firmware supports both transport modes; which one is active is set at startup. In the current tent deployment the node runs in **RS485 mode** — register layout and polling behavior: [RS485 Multisensor](#rs485-multisensor-tent).

#### Sensors (I2C: SDA=1, SCL=0)

| Sensor | I2C Address | Measurands |
|---|---|---|
| SCD41 | `0x62` | CO₂ (ppm, NDIR) · temperature · humidity |
| AS7341 | `0x39` | PPFD (µmol/m²/s × 10) · 8 spectral channels 415–680 nm |

> **Bill of materials changed:** Earlier revisions used AHT21 (temp/humidity) + ENS160 (eCO₂/TVOC). Both were replaced by the SCD41 (a true NDIR CO₂ sensor instead of an eCO₂ estimate, plus temperature/humidity in one part).

#### GPIO Pinout

| GPIO | Function | Note |
|---|---|---|
| `0` | I2C_SCL | Safe, not a strapping pin |
| `1` | I2C_SDA | Safe, not a strapping pin |
| `3` | CAN_TX | Reserved for TWAI/CAN transceiver |
| `5` | CAN_RX | JTAG MTDI → `gpio_reset_pin(GPIO_NUM_5)` in setup() |

> **ESP32-C3 strapping pins:** GPIO9 pulls the ESP32-C3 into download mode if LOW at boot — do not use for I2C. GPIO4, 5, 6, 7 are JTAG pins and must be released with `gpio_reset_pin()` before use.

#### Data Packets Sent in ESP-NOW Mode

- `SUBTYPE_AIR (0x01)`: temperature, humidity, CO₂ from the SCD41.
- `SUBTYPE_LIGHT (0x02)`: 8 spectral channels from the AS7341.

In RS485 mode, ESP-NOW transmission is skipped — the same values instead sit in the Modbus registers, see [RS485 Multisensor](#rs485-multisensor-tent).

### Sockets

**MCU:** ESP32 · 4× relay outputs · `NODE_TYPE_SOCKET · 0x03`

General-purpose relay nodes, controlled manually or via API — used e.g. as the target node for [CO2 control](#co2-control), for pumps, fans, and other loads. Commands come from the master on demand (not cyclic).

> **Light-SSR mode unused:** The firmware still offers a second node type `NODE_TYPE_LIGHT_SSR (0x05)` for automatic lighting control at first boot. This function has been replaced by the dedicated RS485 node [Lichtx4](#lichtx4-1) — in the current deployment, all socket nodes run as regular sockets.

#### Relay Command

```c
// Turn all 4 relays on:
MSG_COMMAND + CMD_SET_ONOFF (0x02) + payload[1] = 0x0F

// Only R1 + R3:
payload[1] = 0b00000101  // = 0x05
```

### First Boot & Configuration

#### Sockets — Node Type and ID

On the very first boot (NVS namespace `"nodecfg"` empty), the serial monitor shows:

```
=== FIRST BOOT: Node configuration ===
Select type (60s timeout):
  1 = Socket  (general control)
  2 = Light-SSR  (sunrise/sunset via scheduler)
> 1
Enter node ID (1-254) + Enter:
> 10
[NODE] Saved: Type=Socket  ID=0x0A
```

Stored in NVS `"nodecfg"`: keys `type` (uint8) and `id` (uint8). Clear with `NODECLEAR` → dialog reappears on next boot. Option 2 (light-SSR) is a legacy mode — see the note under [Node: Sockets](#sockets).

#### WiFi Channel / No-WiFi Mode (all nodes)

On first boot without a saved channel (NVS `"wificfg"` empty):

```
[WIFI] 3 networks found
  [ 1] MeinRouter           Channel  6  -62 dBm
  [ 2] Nachbar-AP           Channel 11  -78 dBm
  [ 0] No WiFi (fixed channel 6)
Number (0-3) + Enter (60s timeout):
> 0
[WIFI] No WiFi, channel 6 saved
```

> **Channel consistency:** All nodes and the master AP must run on the same channel. The fixed channel is defined as `ESPNOW_DEFAULT_CHANNEL = 6` in all `espnow_protocol.h` files. The master AP uses this channel automatically when `USE_ACCESS_POINT = true`.

> **Self-healing on router channel change:** If the router later changes its WiFi channel (auto-channel, reboot after a power outage, …), a node that only stores the channel once would stay stuck on the old (now wrong) channel forever and never find the master again — the master itself handles this automatically, since it reconnects fresh via `WiFi.begin(ssid, password)` on every boot and thus always picks up the current channel. Sockets and Multisensor therefore silently re-verify the known SSID via a scan on **every boot** (no serial dialog needed) and update the stored channel if it differs. The same scan additionally runs every 12 hours during normal operation; if the channel changes while running (without a node restart), the node restarts itself to pick up the new channel.

#### 60-Second Timeout

If no input is given within 60 seconds at any setup dialog (type selection, node ID, channel), a default value is used: socket / ID=3 / channel 6.

---

### Serial Commands

Baud rate **115200** · line ending **LF** or **CR+LF**

#### Sockets

| Command | Effect |
|---|---|
| `STATUS` | Current relay bitmask and individual status R1–R4 |
| `MASK <0–15>` | Set all relays via decimal value (e.g. `MASK 5` → R1+R3 on) |
| `R<1–4> <0\|1>` | Switch a single relay (e.g. `R2 1`) |
| `NODEINFO` | Show stored node type and node ID |
| `NODECLEAR` | Clear node type and ID → first-boot dialog on next restart |
| `WIFIINFO` | Show stored SSID and channel |
| `WIFICLEAR` | Clear WiFi configuration → channel scan on next restart |

#### Multisensor

| Command | Effect |
|---|---|
| `WIFIINFO` | Show stored SSID and channel |
| `WIFICLEAR` | Clear WiFi configuration → channel scan on next restart |
