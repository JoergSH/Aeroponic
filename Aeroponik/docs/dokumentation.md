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
 │  Ultraschall HC-SR04 Füllstand · Netzspannungsüberwachung (USV) · Webinterface Port 80
 │
 ├── RS485 (9600 Baud) ⇄
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
| `NODE_TYPE_SPEKTRUM` | `0x04` | AS7341 Spektrum-Lichtsensor |
| `NODE_TYPE_LIGHT_SSR` | `0x05` | Licht-SSR — automatisch per Beleuchtungs-Scheduler (Legacy, siehe [Steckdosen](#steckdosen)) |

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
| `SUBTYPE_LICHT_SUMMARY` | `0x20` | [1..2] PPFD×10 · [3..4] RB-Ratio×100 · [5] blue% · [6] red% · [7] Sättigung · [8..15] F1–F8 normiert |

---

### Hardware & Pinout

**MCU:** ESP32-S3 N16R8 (16 MB Flash, 8 MB OPI-PSRAM)

> **Gesperrte GPIOs:** 0, 3, 45, 46 (Strapping) · 19, 20 (USB-OTG) · 26–32 (Flash) · 33–37 (OPI-PSRAM) · 38–42 (JTAG) · 43, 44 (UART0)

> **Hardware-Revision:** Ab der SMD-Platine (siehe [`SmdVersion_Pinbelegung.pdf`](../../DokuEtc/SmdVersion_Pinbelegung.pdf)) gibt es keinen I2C-GPIO-Expander (PCF8574AP) mehr — Ventile, Pumpe und Umwälzpumpe hängen an direkten GPIOs, und AHT21B teilt sich den Hauptbus mit der RTC statt eines eigenen zweiten I2C-Busses.

| GPIO | Konstante | Peripherie |
|---|---|---|
| `1` | `RS485_RO` | UART1 RX ← MAX13487 Receive Out |
| `2` | `RS485_DI` | UART1 TX → MAX13487 Data In |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (Vorrat, Pflanze, optional Innentemperatur), Zuordnung über ROM-Adresse (siehe [Funktionen](#funktionen)) |
| `5` | `PRESSURE_PIN` | Analog — Drucksensor 0,5–4,5 V ratiometrisch |
| `6` | `GEHAEUSE_FAN_PIN` | Gehäuselüfter, An/Aus (kein PWM), temperaturgeregelt über Innen-DS18B20 |
| `7` | `VENTIL1_PIN` | Magnetventil Behälter 1 (direkte GPIO) |
| `8` | `VENTIL2_PIN` | Magnetventil Behälter 2 |
| `9` | `VENTIL3_PIN` | Magnetventil Behälter 3 |
| `10` | `VENTIL4_PIN` | Rücklaufventil |
| `11` | `SPI_MOSI` | SPI2 MOSI — W5500 + MicroSD |
| `12` | `SPI_SCK` | SPI2 SCK |
| `13` | `SPI_MISO` | SPI2 MISO |
| `14` | `SPI_CS_W5500` | W5500 Chip Select |
| `15` | `ULTRASONIC_TRIG` | HC-SR04 Trigger |
| `16` | `ULTRASONIC_ECHO` | HC-SR04 Echo (3,3 V direkt anschließbar) |
| `17` | `I2C_SCL` | I2C-Bus: DS3231, AHT21B, optional GP8403 |
| `18` | `I2C_SDA` | I2C-Bus: DS3231, AHT21B, optional GP8403 |
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

#### Tanküberwachung (Kegelstumpf)

Füllstand per Ultraschall (HC-SR04). Umrechnung Distanz → Wasserhöhe → Volumen unter Berücksichtigung der konischen Tankgeometrie (einstellbare Radien oben/unten und Höhe).

#### DS18B20 — Rollenzuordnung über ROM-Adresse

Die DS18B20-Sensoren am gemeinsamen 1-Wire-Bus (Vorrat, Pflanze, optional Innentemperatur für den Gehäuselüfter) werden über ihre eindeutige 64-Bit-ROM-Adresse einer Rolle zugeordnet, statt sich auf die Bus-Scan-Reihenfolge zu verlassen (die sich bei vertauschten oder unterschiedlich langen Kabeln ändert). Solange eine Rolle nicht zugewiesen ist, fällt die Firmware auf die alte Index-Reihenfolge zurück (0=Vorrat, 1=Pflanze, 2=Innentemperatur).

#### Gehäuselüfter

Einfacher An/Aus-Schalter (kein PWM) am Steuerungsgehäuse, geregelt über den Innentemperatur-DS18B20. Hysterese: an ab `temp_max`, aus ab/unter `temp_min` — dazwischen bleibt der zuletzt gesetzte Zustand erhalten, damit der Lüfter nicht dauernd taktet.

#### Netzspannungsüberwachung

`NETZ_OK_PIN` erkennt einen Netzausfall (Betrieb an der USV) — HIGH = Netz vorhanden, LOW = Netzausfall, 2000 ms entprellt gegen kurze Spannungsschwankungen. Der Status ist in der Weboberfläche sichtbar und löst optional eine [WhatsApp-Benachrichtigung](#whatsapp-benachrichtigungen) aus.

#### WhatsApp-Benachrichtigungen

Über [CallMeBot](https://www.callmebot.com/blog/free-api-whatsapp-messages/) (Telefonnummer + API-Key, einmalig per WhatsApp-Aktivierungsnachricht erhalten). Ein globaler Schalter (`global_enabled`) übersteuert alle einzelnen Alarme; jeder Alarm ist zusätzlich einzeln aktivierbar:

| Alarm | Auslöser |
|---|---|
| Netzausfall | Siehe [Netzspannungsüberwachung](#netzspannungsüberwachung) |
| Sensor-Ausfall | AHT21 / RTC / DS18B20 nicht erreichbar |
| Luftfeuchte | Zelt-Luftfeuchte (RS485-Multisensor) außerhalb `feuchte_min`/`feuchte_max` |
| Temperatur | Zelt-Temperatur (RS485-Multisensor) außerhalb `temperatur_min`/`temperatur_max` |
| Behälter fast leer | Vorratsbehälter-Füllstand unter `tank_min_prozent` |
| Zeltlüfter-Drehzahl | Ist-Drehzahl weicht vom Erwartungswert ab, siehe [Lueftermodul](#lueftermodul) |

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

Zweiter, vom ESP-NOW-Netz unabhängiger Kommunikationsweg für fest verdrahtete Geräte im Zelt. Physikalisch ein gemeinsam genutzter Zweidraht-Bus über einen **MAX13487** (automatische Sende-/Empfangsrichtung, kein manuelles DE/RE-Umschalten nötig) an `RS485_DI` (GPIO1) / `RS485_RO` (GPIO2), **9600 Baud, 8N1**.

> **Historisch:** Auf demselben Bus lief bis vor Kurzem zusätzlich ein MARS-Hydro-*iControl*-Hub als weiterer Modbus-Master (Fremdsteuerung des Abluftlüfters per Werks-App). Dieses Gerät wurde vollständig entfernt — der Aeroponik-Master ist jetzt alleiniger Bus-Master.

#### Protokoll

Vereinfachtes Modbus-RTU: **FC03** (Read Holding Registers) zum Auslesen, **FC06** (Write Single Register) zum Schreiben. CRC16 nach Modbus-Standard (Polynom `0xA001`). Ein schlankes Zustandsautomat-Modell (`mb_state_t`) fragt die Busteilnehmer nacheinander ab; nur eine Anfrage gleichzeitig offen, Antwort-Timeout **2000 ms**.

Nach jedem Senden wird das eigene Sende-Echo aktiv weggeräumt (`flush()` + kurze Pause + Restpuffer leeren), bevor auf die Antwort gewartet wird — nötig, weil der MAX13487 auf allen eigenen Platinen mit fest auf Empfang liegendem RE die eigene Sendung immer mit auf RX zurückspiegelt.

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
| 11 | FC03 read (Teil von 6 Registern ab 11) | Drehzahl (RPM) |
| 16 | FC03 read (letztes der 6 Register) | Aktuelle Leistung (%) |
| 0x0010 (16) | FC06 write | Ziel-Leistung 0–100 % |

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

Eigenständige Firmware (`Lueftermodul`) auf einem **RP2040-Zero** (Waveshare), steuert bis zu 4 direkt am Zelt montierte PC-Lüfter (Arctic, 600–3000 RPM, PWM-gesteuert, 0 RPM unter 5 % PWM) mit individueller oder gemeinsamer Drehzahl in 10-%-Schritten. Nur über RS485 angebunden, Adresse `0x51`.

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

#### Master-seitige Überwachung

Der Master vergleicht die gemeldete Ist-Drehzahl gegen eine aus dem PWM-Sollwert erwartete Drehzahl (lineare Interpolation zwischen 600 RPM bei 5 % und 3000 RPM bei 100 %, 0 RPM unter 5 %) mit einstellbarer Toleranz und meldet auffällige Abweichungen über die [WhatsApp-Benachrichtigungen](#funktionen).

### Multimodul

Eigenständige Firmware (`Multimodul`) auf derselben RP2040-Zero-Basis wie das [Lueftermodul](#lueftermodul) — identisches Pinout, plus ein zweiter I2C-Bus (I2C1, SDA=GPIO10/SCL=GPIO11) für optionale Zusatz-Hardware:

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — dieselben Sensoren wie im [Multisensor](#multisensor)-Node. Wenn angeschlossen, beantwortet das Board zusätzlich RS485-Adresse `0x20` mit demselben 14-Register-Layout wie das [RS485-Multisensor](#rs485-multisensor-zelt).
- **GP8403** (I2C `0x58`, 2-Kanal-0–10V-DAC) — wenn angeschlossen, beantwortet das Board zusätzlich RS485-Adresse `0x50` mit demselben 2-Register-Layout wie das [Analog-Ausgangsmodul](#analog-ausgangsmodul).

Beim Boot scannt die Firmware den I2C-Bus; nur tatsächlich gefundene Geräte werden initialisiert und ihre RS485-Adresse aktiviert. Fehlt ein Gerät, bleibt die zugehörige Adresse auf dem Bus unbeantwortet — für den Master nicht von einem schlicht nicht verbauten separaten Multisensor/Analogmodul zu unterscheiden. Dadurch ist am Master **keine Codeänderung** nötig: er pollt `0x20`, `0x50` und `0x51` bereits unabhängig voneinander und behandelt Ausbleiben der Antwort ohnehin als „Gerät offline“.

Ein Multimodul kann also je nach Bestückung ein reines Lüftermodul, ein Lüftermodul mit Zelt-Klimasensorik, ein Lüftermodul mit 0–10V-Ausgang oder alles zusammen sein — auf demselben RS485-Bus, ohne separate Firmware-Varianten.

### Analog-Ausgangsmodul

0-10V. 2-Kanal RS485-zu-0-10V-Wandler (ATO), Standard-Modbus-RTU. Wahlweise Alternative zu Lichtx4 (Kanal 2, stufenlose Helligkeit statt Relais-Stufen) und/oder zum MARS-Hydro-Lüfter (Kanal 1) — z. B. für generische Lüfter oder Dimmer mit 0-10V-Steuereingang. Dasselbe Register-Layout wird auch vom optionalen GP8403-Ausgang des [Multimodul](#multimodul) bedient.

> **Einmaliger Hardware-Setup:** Das Modul kommt ab Werk auf Adresse `0x01` / 4800 Baud — inkompatibel mit unserem 9600-Baud-Bus. Vor dem Einbau muss es einmalig **einzeln** (nicht am gemeinsamen Bus) per USB-RS485-Adapter und der ATO-Konfigurationssoftware umgestellt werden: Adresse → `0x50`, Baudrate → 9600 (Registerwert 2), Ausgangstyp beider Kanäle → 0-10V (Register 0x0055 sowie die physischen Lötbrücken auf "V").

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

### Raumklima & VPD

#### AHT21B

Hängt am Haupt-I2C-Bus (`I2C_SDA`/`I2C_SCL`, GPIO18/17) zusammen mit der RTC und dem optionalen GP8403 — siehe [I2C-Geräte](#i2c-geräte-sda18-scl17). Der Sensor misst die **Raumluft außerhalb des Zelts** als Referenz.

> **Bekannter Library-Bug:** Adafruit_AHTX0 sendet standardmäßig den AHT10-Kalibrierbefehl `0xE1`. AHT20/AHT21(B)-Chips erwarten stattdessen `0xBE` — sonst liefert der Feuchtekanal dauerhaft ~100 %, während die Temperatur bereits korrekt ist. Die Firmware sendet den korrekten Befehl (`0xBE, 0x08, 0x00`) direkt nach `aht21.begin()` manuell nach.

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

> **Automatische Migration:** Trifft die Firmware beim Schreiben auf eine bereits vorhandene Tagesdatei mit dem alten, kürzeren Spaltenformat (vor dieser Erweiterung), wird diese zu `_alt.csv` umbenannt (Daten bleiben erhalten) und eine neue Datei mit vollständigem Header begonnen — kein Vermischen unterschiedlicher Spaltenzahlen in einer Datei.

#### Download & Löschen (Konfiguration → Daten-Log)

| Endpunkt | Methode | Funktion |
|---|---|---|
| `/api/logs` | GET | Liste aller `*.csv`- und `*.txt`-Dateien mit Dateigröße |
| `/api/logs/download?file=NAME` | GET | Streamt die Datei mit `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Löscht die Datei (UI fragt vorher per Bestätigungsdialog nach) |

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
 │  W5500 Ethernet (LAN + WiFi in parallel) · PCF8574AP solenoid valves MV1–MV4 · DS3231 RTC (I2C)
 │  MicroSD data logging · RS485 UART1 (MAX13487) · 2× DS18B20 · pressure sensor 0.5–4.5V
 │  HC-SR04 ultrasonic fill level · web interface, port 80
 │
 ├── RS485 (9600 baud) ⇄
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
| `NODE_TYPE_SPEKTRUM` | `0x04` | AS7341 spectral light sensor |
| `NODE_TYPE_LIGHT_SSR` | `0x05` | Light SSR — automatic via lighting scheduler (legacy, see [Sockets](#sockets)) |

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
| `SUBTYPE_LICHT_SUMMARY` | `0x20` | [1..2] PPFD×10 · [3..4] RB ratio×100 · [5] blue% · [6] red% · [7] saturation · [8..15] F1–F8 normalized |

---

### Hardware & Pinout

**MCU:** ESP32-S3 N16R8 (16 MB flash, 8 MB OPI-PSRAM)

> **Reserved GPIOs:** 0, 3, 45, 46 (strapping) · 19, 20 (USB-OTG) · 26–32 (flash) · 33–37 (OPI-PSRAM) · 38–42 (JTAG) · 43, 44 (UART0)

> **Hardware revision:** As of the SMD board (see [`SmdVersion_Pinbelegung.pdf`](../../DokuEtc/SmdVersion_Pinbelegung.pdf)), there is no longer an I2C GPIO expander (PCF8574AP) — valves, main pump, and circulation pump are direct GPIOs, and the AHT21B shares the main bus with the RTC instead of having its own second I2C bus.

| GPIO | Constant | Peripheral |
|---|---|---|
| `1` | `RS485_RO` | UART1 RX ← MAX13487 Receive Out |
| `2` | `RS485_DI` | UART1 TX → MAX13487 Data In |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (reservoir, plant, optional internal temperature), assigned by ROM address (see [Features](#features)) |
| `5` | `PRESSURE_PIN` | Analog — pressure sensor 0.5–4.5 V ratiometric |
| `6` | `GEHAEUSE_FAN_PIN` | Enclosure fan, on/off (no PWM), temperature-controlled via the internal DS18B20 |
| `7` | `VENTIL1_PIN` | Solenoid valve reservoir 1 (direct GPIO) |
| `8` | `VENTIL2_PIN` | Solenoid valve reservoir 2 |
| `9` | `VENTIL3_PIN` | Solenoid valve reservoir 3 |
| `10` | `VENTIL4_PIN` | Return valve |
| `11` | `SPI_MOSI` | SPI2 MOSI — W5500 + MicroSD |
| `12` | `SPI_SCK` | SPI2 SCK |
| `13` | `SPI_MISO` | SPI2 MISO |
| `14` | `SPI_CS_W5500` | W5500 chip select |
| `15` | `ULTRASONIC_TRIG` | HC-SR04 trigger |
| `16` | `ULTRASONIC_ECHO` | HC-SR04 echo (3.3 V, direct connection) |
| `17` | `I2C_SCL` | I2C bus: DS3231, AHT21B, optional GP8403 |
| `18` | `I2C_SDA` | I2C bus: DS3231, AHT21B, optional GP8403 |
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

#### Tank Monitoring (Truncated Cone)

Fill level via ultrasonic sensor (HC-SR04). Conversion distance → water height → volume, accounting for the conical tank geometry (configurable top/bottom radii and height).

#### DS18B20 — Role Assignment by ROM Address

The DS18B20 sensors on the shared 1-Wire bus (reservoir, plant, optional internal temperature for the enclosure fan) are assigned to a role via their unique 64-bit ROM address, rather than relying on bus-scan order (which changes with swapped or differently-lengthed cables). As long as a role isn't assigned, the firmware falls back to the old index order (0=reservoir, 1=plant, 2=internal temperature).

#### Enclosure Fan

A simple on/off switch (no PWM) on the controller enclosure, controlled via the internal DS18B20. Hysteresis: on at/above `temp_max`, off at/below `temp_min` — in between, the last state is held so the fan doesn't chatter.

#### Mains Power Monitoring

`NETZ_OK_PIN` detects a mains power outage (running on the UPS) — HIGH = mains present, LOW = mains outage, debounced 2000 ms against brief voltage dips. The status is shown in the web interface and can optionally trigger a [WhatsApp notification](#whatsapp-notifications).

#### WhatsApp Notifications

Via [CallMeBot](https://www.callmebot.com/blog/free-api-whatsapp-messages/) (phone number + API key, obtained once via a WhatsApp activation message). A global switch (`global_enabled`) overrides all individual alerts; each alert can also be enabled separately:

| Alert | Trigger |
|---|---|
| Power outage | See [Mains Power Monitoring](#mains-power-monitoring) |
| Sensor failure | AHT21 / RTC / DS18B20 unreachable |
| Humidity | Tent humidity (RS485 multisensor) outside `feuchte_min`/`feuchte_max` |
| Temperature | Tent temperature (RS485 multisensor) outside `temperatur_min`/`temperatur_max` |
| Reservoir nearly empty | Reservoir fill level below `tank_min_prozent` |
| Tent fan RPM | Actual RPM deviates from the expected value, see [Lueftermodul](#lueftermodul-1) |

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

A second communication path, independent of the ESP-NOW network, for hard-wired devices in the tent. Physically a shared two-wire bus via a **MAX13487** (automatic transmit/receive direction, no manual DE/RE switching needed) on `RS485_DI` (GPIO1) / `RS485_RO` (GPIO2), **9600 baud, 8N1**.

> **Historical note:** Until recently, a MARS Hydro *iControl* hub also ran on the same bus as a second Modbus master (third-party control of the exhaust fan via the vendor app). This device has been fully removed — the Aeroponik master is now the sole bus master.

#### Protocol

Simplified Modbus RTU: **FC03** (Read Holding Registers) for reading, **FC06** (Write Single Register) for writing. CRC16 per the Modbus standard (polynomial `0xA001`). A lean state-machine model (`mb_state_t`) polls the bus participants in turn; only one request outstanding at a time, response timeout **2000 ms**.

After every send, the transceiver's own transmit echo is actively drained (`flush()` + a short pause + clearing any leftover bytes) before waiting for the response — needed because the MAX13487 on all of our boards has RE tied permanently to receive, so it always reflects its own transmission back onto RX.

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
| 11 | FC03 read (part of 6 registers starting at 11) | Speed (RPM) |
| 16 | FC03 read (last of the 6 registers) | Current power (%) |
| 0x0010 (16) | FC06 write | Target power 0–100 % |

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

Standalone firmware (`Lueftermodul`) on an RP2040-Zero (Waveshare), driving up to 4 PC fans mounted directly on the tent (Arctic, 600–3000 RPM, PWM-controlled, 0 RPM below 5 % PWM) with individual or shared speed in 10 % steps. RS485-only, address `0x51`.

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

#### Master-Side Monitoring

The master compares the reported actual RPM against an expected RPM derived from the PWM setpoint (linear interpolation between 600 RPM at 5 % and 3000 RPM at 100 %, 0 RPM below 5 %) with a configurable tolerance, and reports outliers via the [WhatsApp notifications](#features).

### Multimodul

Standalone firmware (`Multimodul`) on the same RP2040-Zero base as the [Lueftermodul](#lueftermodul-1) — identical pinout, plus a second I2C bus (I2C1, SDA=GPIO10/SCL=GPIO11) for optional add-on hardware:

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — the same sensors as in the [Multisensor](#multisensor-2) node. When present, the board additionally answers RS485 address `0x20` with the same 14-register layout as the [RS485 multisensor](#rs485-multisensor-tent).
- **GP8403** (I2C `0x58`, 2-channel 0–10V DAC) — when present, the board additionally answers RS485 address `0x50` with the same 2-register layout as the [analog output module](#analog-output-module).

On boot, the firmware scans the I2C bus; only devices actually found are initialized and have their RS485 address activated. If a device is missing, its address simply stays unanswered on the bus — indistinguishable, from the master's point of view, from a separate Multisensor/analog module that was never installed. This means **no code change is needed on the master**: it already polls `0x20`, `0x50`, and `0x51` independently and already treats a missing response as "device offline".

Depending on what's populated, a Multimodul can therefore be a plain fan module, a fan module with tent climate sensing, a fan module with a 0–10V output, or all of it at once — on the same RS485 bus, with no separate firmware variants.

### Analog Output Module

0-10V. 2-channel RS485-to-0-10V converter (ATO), standard Modbus RTU. An optional alternative to Lichtx4 (channel 2, stepless brightness instead of relay steps) and/or the MARS Hydro fan (channel 1) — e.g. for generic fans or dimmers with a 0-10V control input. The same register layout is also served by the [Multimodul](#multimodul-1)'s optional GP8403 output.

> **One-time hardware setup:** The module ships at address `0x01` / 4800 baud by default — incompatible with our 9600-baud bus. Before installing it, it must be reconfigured once **standalone** (not on the shared bus) via a USB-RS485 adapter and the ATO configuration software: address → `0x50`, baud rate → 9600 (register value 2), output type of both channels → 0-10V (register 0x0055 plus the physical solder jumpers set to "V").

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

### Room Climate & VPD

#### AHT21B

Sits on the main I2C bus (`I2C_SDA`/`I2C_SCL`, GPIO18/17) together with the RTC and the optional GP8403 — see [I2C Devices](#i2c-devices-sda18-scl17). The sensor measures **room air outside the tent** as a reference.

> **Known library bug:** Adafruit_AHTX0 sends the AHT10 calibration command `0xE1` by default. AHT20/AHT21(B) chips instead expect `0xBE` — otherwise the humidity channel reports a constant ~100 % while temperature is already correct. The firmware manually re-sends the correct command (`0xBE, 0x08, 0x00`) right after `aht21.begin()`.

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

> **Automatic migration:** If the firmware encounters an existing daily file with the old, shorter column format (prior to this extension) when writing, it renames it to `_alt.csv` (data is preserved) and starts a new file with the full header — no mixing of different column counts within one file.

#### Download & Delete (Configuration → Data Log)

| Endpoint | Method | Function |
|---|---|---|
| `/api/logs` | GET | List of all `*.csv` and `*.txt` files with file size |
| `/api/logs/download?file=NAME` | GET | Streams the file with `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Deletes the file (UI asks for confirmation first) |

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
