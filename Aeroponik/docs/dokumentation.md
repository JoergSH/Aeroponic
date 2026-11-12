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
- [VPD-Regelung](#vpd-regelung)
- [Raumlicht & DWC-Timer](#raumlicht--dwc-timer)
- [Durchflussmesser & Düsen-Überwachung](#durchflussmesser--düsen-überwachung)
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

Die RS485-Geräte sind fest verdrahtet und werden zyklisch abgefragt — siehe [RS485-Bus](#rs485-bus). ESP-NOW-Nodes sind fest an den Master gekoppelt (siehe [Feste Kopplung](#feste-kopplung-node--steuerung)), registrieren sich beim Start und senden periodische Heartbeats; der Master erkennt ihren Online-/Offline-Status automatisch.

**WLAN-Überwachung:** Bricht die WLAN-Verbindung ab und baut der ESP32 sie nicht innerhalb von 30 s selbst wieder auf, verbindet der Master einmal neu; klappt das 60 s lang nicht, startet er neu. Ohne WLAN fällt auch ESP-NOW aus (gleiches Funkmodul, Steckdosen schalten nicht mehr). Gemeldet wird das (Alarm *Verbindungsausfall*) nach dem Wiederverbinden bzw. nach dem Neustart, mit Dauer und Grundcode.

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
| `MSG_UNPAIR` | `0x50` | Master → Node | Kopplung lösen, Node vergisst seinen Master |

#### Feste Kopplung Node ↔ Steuerung

Damit zwei Steuerungen in Funkreichweite sich nicht gegenseitig die Nodes wegnehmen, sind Nodes fest an einen Master gekoppelt:

- **Node:** speichert die MAC des Masters, der ihm das erste `MSG_REGISTER_ACK` schickt, im NVS (`espnow_pair`/`master`) und nimmt danach nur noch Pakete von dieser MAC an. Ungekoppelt nimmt er ausschließlich ein `REGISTER_ACK` an — Schaltbefehle eines fremden Masters werden nie ausgeführt.
- **Master:** führt eine Kopplungsliste im NVS (`espnow_pair`/`nodes`) und antwortet nur Nodes daraus. Unbekannte Nodes erscheinen im Webinterface (System → Node-Steuerung) unter *Neu gefunden* und werden erst per **Koppeln** angenommen. Ist die Node-ID schon an einen anderen Node vergeben, ist Koppeln gesperrt (ID am Node per `NODECLEAR` ändern).
- **Entkoppeln:** im Webinterface an der Node-Karte (Master schickt `MSG_UNPAIR`) oder am Node selbst **BOOT-Taster 3 s im Betrieb halten** bzw. serieller Befehl `UNPAIR`. Danach koppelt sich der Node mit der ersten Steuerung, die ihn in ihrer Liste hat oder bei der man *Koppeln* drückt — vor dem Umzug an eine andere Steuerung also auch an der alten entkoppeln.
- Es gibt kein automatisches Koppeln — auch nach dem Update auf eine Master-Firmware mit Kopplung muss jeder vorhandene Node einmal per *Koppeln* angenommen werden.

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
| `3` | `FLOW_SENSOR_PIN` | Durchflussmesser (YF-S401-ähnlich, Hall-Impulsgeber, push-pull) — siehe [Durchflussmesser & Düsen-Überwachung](#durchflussmesser--düsen-überwachung) |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (Vorrat, Pflanze, optional Innentemperatur), Zuordnung über ROM-Adresse (siehe [Funktionen](#funktionen)) |
| `5` | `PRESSURE_PIN` | Analog — Drucksensor 0,5–4,5 V ratiometrisch (per Web-UI abschaltbar, siehe [Drucksensor](#drucksensor-pumpendruck)) |
| `6` | `GEHAEUSE_FAN_PIN` | Gehäuselüfter, An/Aus (kein PWM), temperaturgeregelt über Innen-DS18B20 |
| `7` | `VENTIL2_PIN` | Magnetventil Behälter 2 (direkte GPIO) |
| `8` | `VENTIL1_PIN` | Magnetventil Behälter 1 |
| `9` | `VENTIL4_PIN` | Ventil Behälter 4 (bis FW 1.18 Rücklaufventil) |
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
| `VENTIL4_PIN` | MV4 | Zulauf Behälter 4 |

#### Hauptpumpe & Umwälzpumpe

Die Hauptpumpe (`PUMPE_GPIO`) schaltet automatisch mit, sobald irgendein Behälter gerade aktiv wässert (Ventil offen) — unabhängig davon, ob im normalen oder im Batch-Zeitmodus (siehe [Ventilsteuerung](#funktionen)).

**Druckaufbau:** Vor jedem Sprühvorgang läuft die Pumpe eine einstellbare Zeit (Standard 5 s, 0 = aus) bei noch geschlossenem Düsenventil, erst dann öffnet das Ventil. Ablauf je Behälter: Pause → Druckaufbau → Aktiv → Nachlauf. Ohne Druckaufbau sprühte jeder Zyklus mit erst steigendem Druck — Menge und Durchfluss-Impulse schwankten dann deutlich. Gilt auch im Batch-Modus vor jedem einzelnen Behälter (die Pumpe läuft dazwischen weiter) und beim manuellen Einzelzyklus. Voraussetzung: Die Pumpe muss kurz gegen geschlossene Ventile fördern dürfen (Anlage steht unter Druck / Druckschalter).

**Nachlauf:** Nach dem Schließen des Düsenventils läuft die Pumpe noch einmal dieselbe Zeit weiter (Zustand „Nachlauf“). Gedacht für einen Druckspeicher zwischen Durchflussmesser und Ventilen: Er überbrückt den Druckeinbruch beim Öffnen (die Drosseldüse im Durchflussmesser liefert nicht schnell genug nach) und wird im Nachlauf wieder aufgefüllt. Die Durchflussmessung zählt vom Öffnen bis zum Ende des Nachlaufs, damit auch das aus dem Speicher gesprühte Wasser beim richtigen Behälter landet; als Dauer gilt nur die Ventil-offen-Zeit. Im Batch-Modus öffnet der nächste Behälter direkt nach dem Nachlauf des vorigen (kein eigener Druckaufbau mehr, nur noch vor dem ersten Behälter). Ohne Druckspeicher fließt im Nachlauf praktisch nichts, die Messwerte bleiben gleich. Der manuelle Einzelzyklus hat keinen Nachlauf.

Die Umwälzpumpe (`UMWAELZPUMPE_GPIO`, Vorratsbehälter-Zirkulation) läuft in einem konfigurierbaren Lauf-/Pausenzyklus (`laufzeit_s` / `pausenzeit_min`), sobald irgendein Behälter überhaupt aktiviert ist — unabhängig davon, ob dieser Behälter gerade tatsächlich wässert oder nur zwischen zwei Zyklen pausiert. Ist kein Behälter aktiviert, bleibt sie aus.

### Funktionen

#### Netzwerk

W5500 (Ethernet) und WiFi laufen gleichzeitig über das lwIP-Stack. Bei Ethernet-Verbindungsaufbau wird automatisch **NTP** abgerufen und die RTC gestellt (Schwellwert: ±60 s Abweichung, sync alle 60 min). Das Webinterface ist auf beiden IPs ohne Codeänderungen verfügbar.

Ohne Router: Master startet als **AP auf Kanal 6** (`ESPNOW_DEFAULT_CHANNEL`). Nodes im "Kein-WLAN"-Modus nutzen denselben Kanal.

#### Ventilsteuerung

Vier unabhängige Bewässerungskreise (MV1–MV4) mit konfigurierbarer Öffnungszeit (Sek.) und Pausenintervall (Min., jeweils getrennt für Licht- und Dunkelphase). Im Modus "Gleiche Zeiten für alle" läuft statt unabhängiger Timer ein gemeinsamer Batch-Durchlauf (alle aktiven Behälter direkt nacheinander, eine gemeinsame Pause).

**Umbau FW 1.19.0 (11/2026):** MV4 war bisher das Rücklaufventil mit optionalem „Rücklauf-Vorlauf“ vor jedem Zyklus; zwei Behälter hingen an einem gemeinsamen Ventil. Die Rückleitung ist jetzt ein Handventil (Kugelhahn), MV4 versorgt Behälter 4, jeder Behälter hat damit sein eigenes Ventil und seine eigenen Durchfluss-Impulse. Der Vorlauf entfällt. Beim ersten Start mit 1.19.0 übernimmt Behälter 4 die Zeiten von Behälter 3, bleibt aber deaktiviert, bis er auf der Ventil-Seite eingeschaltet wird. Die Konfiguration von Behälter 4 liegt in einem eigenen EEPROM-Block (560–565, Sollwert der Düsen-Überwachung 566–569); der EEPROM-Bereich wächst dafür von 570 auf 600 Bytes, vorhandene Einstellungen bleiben erhalten.

Nach einem Neustart wird **sofort** gewässert, statt erst die volle Pausenzeit (bis zu 60 min) abzuwarten.

**Manueller Einzelzyklus:** Auf der Aeroponik-Seite lässt sich pro Behälter ein einzelner Gießzyklus von Hand auslösen — das Ventil öffnet einmal für die aktuell gültige Öffnungszeit (Licht- bzw. Dunkelphase) und schließt danach automatisch (`/api/ventile/dose/start`, `/status`, `/cancel`). Die normale Automatik pausiert währenddessen.

**Ausgangstest:** Der Direkttest der Ausgänge (Verkabelungsprüfung, manueller Einzelzyklus) unterbricht keinen laufenden Gießzyklus mehr — ist gerade ein Ventil/eine Pumpe automatisch aktiv, wird der Testmodus erst aktiv, sobald der Zyklus von selbst beendet ist. Die UI zeigt den Wartezustand an (`GET /api/output/test`). Beim manuellen Einzelzyklus läuft die Umwälzpumpe in ihrem eigenen Takt weiter und verzögert den Start nicht. **Sicherheitsnetz (seit FW 1.19.2):** Ein vergessener Testmodus pausiert die Bewässerung — er endet deshalb nach spätestens 30 Minuten automatisch, die Automatik läuft weiter und es geht eine [Benachrichtigung](#benachrichtigungen) raus.

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
| Zeltlüfter-Drehzahl | Ist-Drehzahl weicht mindestens 3 Minuten am Stück vom Erwartungswert ab (Anlauf/Modul-Neustart lösen keinen Alarm aus), siehe [Lueftermodul](#lueftermodul) |
| Düsen-Durchfluss | Soll/Ist-Abweichung je Behälter, siehe [Düsen-Überwachung](#durchflussmesser--düsen-überwachung) |

Jeder Kanal hat ein eigenes Rate-Limit (30 s) gegen versehentliches Zuspammen; ein eigener Test-Button pro Kanal in der Konfiguration verifiziert die Zugangsdaten unabhängig von den anderen.

#### SD-Logging

Jede Minute (bis FW 1.20.1: alle 5 Minuten) werden sämtliche Mess- und Steuerwerte in CSV-Dateien nach dem Schema `/JJJJ-MM-TT.csv` geschrieben, inkl. Raum-/Zeltklima, Lüfter- und Lichtstatus. Details, vollständige Spaltenliste sowie Download/Löschen über das Webinterface: siehe [Daten-Log & Download](#daten-log--download).

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

Vereinfachtes Modbus-RTU: **FC03** (Read Holding Registers) zum Auslesen, **FC06** (Write Single Register) zum Schreiben. CRC16 nach Modbus-Standard (Polynom `0xA001`). Ein schlankes Zustandsautomat-Modell (`mb_state_t`) fragt die Busteilnehmer nacheinander ab; nur eine Anfrage gleichzeitig offen, Antwort-Timeout **2000 ms**. Vor jeder Anfrage hält der Master mindestens **10 ms** Buspause ein (seit FW 1.21.4): die Slaves erkennen das Frame-Ende an einer 4-ms-Lücke, ohne Pause klebte die MS2-Anfrage an der 39-Byte-Antwort des MARS-Lüfters und ging verloren (Ursache der sporadischen `MS2_Timeouts`, gefunden über den Zelt-Logger am 12.11.). Das Multimodul (ab FW 1.3.4) löst zusätzlich eine gültige Anfrage am Ende eines verklebten Frames heraus und beantwortet sie.

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

#### Firmware-Update über RS485

Ab Firmware 1.1.0 (einmalig per USB/Programmier-Pads aufzuspielen) lässt sich auch das Lichtx4 im eingebauten Zustand aktualisieren: Weboberfläche → System → **Modul-Update (RS485)**, Modul „Lichtx4“ wählen, Datei `Lichtx4/.pio/build/nologo_esp32c3_super_mini/firmware.bin`. Gleiches Protokoll wie beim [Multimodul](#multimodul) (Funktionscode `0x41`, hier an Adresse `0x40`, `Lichtx4/src/fw_update.h`). Das Image landet in der zweiten OTA-App-Partition und wird erst nach erfolgreicher MD5-Prüfung aktiviert; der erste Block muss ein ESP-Image (Magic `0xE9`) mit der Chip-ID des ESP32-C3 sein. Dauer ca. 1–2 min (~320 KB). Die Relaismaske bleibt per `Preferences` über den Neustart erhalten.

> Ab dieser Firmware läuft das Lichtx4 mit 115200 Baud — danach `#define LICHTX4_LEGACY_9600` aus `Aeroponik/src/local_config.h` entfernen (siehe [RS485-Bus](#rs485-bus)), sonst spricht der Master es weiter mit 9600 an.

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
| Auto — Luftfeuchte | Zelt-Feuchte (RS485-Multisensor) | Linearer Ramp zwischen `hum_min` (Mindestdrehzahl) und `hum_max` (Höchstdrehzahl) |
| Auto — Temperatur, absolut | Zelt-Temperatur | Ramp zwischen `temp_min`/`temp_max`; optionale **Differenz-Bremse**: liegt Zelt−Raum-Differenz unter `temp_cap_diff`, wird trotz Rampe nur die Mindestdrehzahl gefahren (verhindert Volllast, wenn der Raum fast so warm wie das Zelt ist) |
| Auto — Temperatur, Differenz | Zelt-Temperatur − Raumtemperatur | Ramp direkt über die Temperaturdifferenz zwischen `temp_min`/`temp_max` |

Eine gemeinsame Mindest- und Höchstdrehzahl (`min_speed`, `max_speed`, Standard 100 %) gilt für beide Automodi. Bei 0 % wird zusätzlich das Enable-Register (reg15) des MARS-Lüfters auf 0 gesetzt, da er sonst mit Grunddrehzahl weiterläuft. Die Differenz-Modi benötigen den [Raumklimasensor](#raumklima--vpd); ist dieser nicht verfügbar, bleibt die letzte Zielleistung unverändert.
**Taktbetrieb unter der Mindestdrehzahl (seit FW 1.21.3, nur MARS):** Der MARS-Lüfter läuft bei jedem Wert > 0 mit seiner eingebauten Mindestdrehzahl (~900 RPM, meldet 25 %), darunter kennt er nur „aus“. Sollwerte zwischen 1 und 24 % setzt der Master deshalb im 10-Minuten-Takt um: der Lüfter läuft Sollwert/25 der Zeit mit Minimum (mind. 1 min am Stück) und ist sonst aus — z. B. 5 % = 2 von 10 min, 10 % = 4 von 10 min. Die Mindestdrehzahl wird damit zur Grundlüftung, 0 % erlaubt komplettes Abschalten. Die Startseite zeigt im Takt den Sollwert mit ⏱ statt der Rückmeldung.

#### Konfiguration (EEPROM, Magic 0x92)

`mode` · `manual_percent` · `min_speed` · `max_speed` (eigener EEPROM-Block, Magic 0x6B) · `hum_min/max` · `temp_min/max` · `temp_mode` (absolut/Differenz) · `temp_cap_diff` · `enabled`

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

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — dieselben Sensoren wie im [Multisensor](#multisensor)-Node. Wenn mindestens einer davon angeschlossen ist, beantwortet das Board zusätzlich RS485-Adresse `0x20` mit demselben 14-Register-Layout wie das [RS485-Multisensor](#rs485-multisensor-zelt). Ohne SCD41 läuft der AS7341 allein im 10-s-Takt.
- **AHT21 + ENS160** (I2C `0x38` / `0x53` oder `0x52`, z. B. DFRobot-Kombimodul SEN0515) — Ersatz für den SCD41, wird nur genutzt, wenn kein SCD41 gefunden wird. Temperatur/Feuchte vom AHT21, CO₂ als **eCO₂-Schätzwert** des ENS160 (aus flüchtigen organischen Verbindungen abgeleitet, kein echtes NDIR-CO₂ — für eine CO₂-Regelung nur bedingt geeignet). Während der Aufwärmphase des ENS160 (bis ca. 1 h nach dem ersten Start) gilt der CO₂-Wert als ungültig.

**Status-Register `0x20`/13** (vom Master ausgewertet): Bit0 = Temperatur/Feuchte gültig (SCD41 oder AHT21), Bit1 = AS7341 ok, Bit2 = CO₂ ungültig (kein ENS160 oder Aufwärmphase), Bit3 = CO₂ nur Schätzwert (ENS160). Bei ungültigen Werten lässt der Master CO₂-/Feuchte-Regelung, Abluft-Automatik und CSV-Spalten aus; ein eCO₂-Wert erscheint auf der Startseite mit vorangestelltem `~`.
- **GP8403** (I2C `0x58`, 2-Kanal-0–10V-DAC) — wenn angeschlossen, beantwortet das Board zusätzlich RS485-Adresse `0x50` mit demselben 2-Register-Layout wie das [Analog-Ausgangsmodul](#analog-ausgangsmodul).

Beim Boot scannt die Firmware den I2C-Bus; nur tatsächlich gefundene Geräte werden initialisiert und ihre RS485-Adresse aktiviert. Fehlt ein Gerät, bleibt die zugehörige Adresse auf dem Bus unbeantwortet — für den Master nicht von einem schlicht nicht verbauten separaten Multisensor/Analogmodul zu unterscheiden. Dadurch ist am Master **keine Codeänderung** nötig: er pollt `0x20`, `0x50` und `0x51` bereits unabhängig voneinander und behandelt Ausbleiben der Antwort ohnehin als „Gerät offline“.

Ein Multimodul kann also je nach Bestückung ein reines Lüftermodul, ein Lüftermodul mit Zelt-Klimasensorik, ein Lüftermodul mit 0–10V-Ausgang oder alles zusammen sein — auf demselben RS485-Bus, ohne separate Firmware-Varianten.

> **Boot-Reihenfolge:** `rs485_slave_init()` läuft bewusst ganz am Anfang von `setup()`, **vor** dem I2C-Scan/Sensor-Init — sonst bliebe die DI-Leitung (GP28) unkonfiguriert/hochohmig, solange der I2C-Scan oder `scd41_init()`/`gp8403_init()` laufen (können bei einer hängenden I2C-Transaktion beliebig lange dauern), ohne definierten Idle-Pegel auf dem Bus. Zusätzlich loggt die Firmware beim Start die Reset-Ursache (`rp2040.getResetReason()`: Power-On, Watchdog, Brownout, Glitch, …) — nützlich zur Diagnose unerklärter Neustarts, da das Projekt selbst keinen Software-Watchdog nutzt.

> **SCD41 und AS7341 unabhängig:** CO₂/Temperatur/Feuchte (SCD41) werden sofort nach jeder Messung in die `0x20`-Register geschrieben, unabhängig vom AS7341. Bleibt eine AS7341-Messung länger als 6 s hängen, wird nur der AS7341 neu initialisiert (kein I2C-Bus-Reset, der den SCD41 mitreißen würde); nach 3 erfolglosen Versuchen wird er für 10 min deaktiviert (Lichtwerte = 0, Gain = `0xFF`, Status-Bit 1 = 0) und danach automatisch neu versucht — Board, SCD41, Lüfter und Analogausgang bleiben die ganze Zeit online.

#### Firmware-Update über RS485

Das Multimodul lässt sich im eingebauten Zustand über den RS485-Bus aktualisieren: Weboberfläche → System → **Modul-Update (RS485)** (Modul „Multimodul“ wählen), Datei `Multimodul/.pio/build/waveshare_rp2040_zero/firmware.bin` (nicht `.uf2`). Die Update-Funktion muss dafür einmalig per USB aufgespielt sein (ab Firmware 1.1.0).

Ablauf: Der Master puffert die Datei im PSRAM, berechnet die MD5 und überträgt sie in Blöcken zu 200 Byte über einen eigenen Funktionscode `0x41` an Adresse `0x51` (Protokoll: `Multimodul/src/fw_update.h`). Das normale Polling pausiert währenddessen. Das Multimodul schreibt das Image zunächst als Datei in sein LittleFS (`board_build.filesystem_size = 0.5m`), prüft die MD5 und merkt es erst dann für den OTA-Bootloader vor, der es beim Neustart übernimmt. Danach fragt der Master die neue Version ab; die Lüfter- und Analog-Sollwerte werden über die Boot-ID wie nach jedem Neustart automatisch neu geschrieben.

Sicherungen: Bricht die Übertragung ab (Timeout nach 5 Wiederholungen, Abbrechen-Button, Master-Neustart), bleibt die laufende Firmware unangetastet. Verlorene Antworten führen nicht zu doppelt geschriebenen Blöcken — der Slave meldet immer den nächsten erwarteten Offset. Der Master lehnt ESP32- und `.uf2`-Dateien ab, und das Multimodul vergleicht den ersten Block (boot2) mit seinem eigenen Flash, sodass keine Firmware für ein anderes Board geflasht wird. Die Firmware-Version (`FW_VERSION_*` in `fw_update.h`) sollte bei jedem Update erhöht werden, sonst meldet die Oberfläche einen Hinweis.

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

#### Steckerbelegung der 0–10V-Ausgänge — Achtung, Platinen-Layout falsch

Die 0–10V-Ausgänge nutzen denselben Steckertyp wie die RS485-Anschlüsse: Western-Modularbuchse RJ12 (6P6C, also 6 Positionen, alle 6 Kontakte belegt) bzw. Modular-Stecker 6P6C. Ihre aktuelle Belegung auf der Platine passt aber **nicht** zu den Lampen (z. B. Mars Hydro FC-E1500):

| Pin | Aktuelle Platinen-Belegung | Ader |
|---|---|---|
| 3 | GND (GND vom DAC) | rot |
| 4 | 0–10 V Signal (Schleifer, Ausgang vom GP8403) | grün |

Benötigt wird stattdessen:

| Pin | Signal |
|---|---|
| 3 + 4 | GND |
| 2 + 5 | 0–10 V |

Bis zu einer korrigierten Platine muss ein **Adapterkabel** gemacht werden. An der Software ändert sich dadurch nichts.

> **Belegung nicht garantiert:** Diese Belegung ist nur ein Quasi-Standard und nicht bei jedem Gerät gleich. Vor dem Anschließen eines neuen Lampen- oder Lüftermodells die Belegung am Gerät prüfen (Datenblatt bzw. nachmessen).

#### Lampen-Kennlinie (PPFD)

Klima → **Lampen-Kennlinie (PPFD)** misst, wie linear eine Lampe am 0–10V-Lichtausgang (Analogmodul oder GP8403) auf die Steuerspannung reagiert. Der Master setzt den Ausgang nacheinander auf 0, 5, 10, … 100 % (21 Stufen), wartet je Stufe 10 s (ein Messtakt des AS7341, damit kein Wert von vor dem Umschalten mitzählt), fragt den Multisensor (`0x20`) dann alle 5 s statt alle 15 s ab und geht weiter, sobald 3 Werte in Folge innerhalb von 1 % (mindestens 0,5 µmol/m²/s) liegen — deren Mittel ist der Wert der Stufe. Wird eine Stufe in 90 s nicht stabil, gilt das Mittel der letzten Werte. Insgesamt ca. 8–10 min. Währenddessen ist der Zeitplan überstimmt, danach übernimmt er wieder.

Ergebnis: Tabelle und Diagramm mit PPFD je Stufe, relativ zu 100 % (der 0-%-Wert wird als Restlicht abgezogen) und der Abweichung von einer idealen Geraden. Die letzte vollständige Messung wird im NVS gespeichert (`lichtkal`) und beim Öffnen der Seite angezeigt.

**Kennlinie anwenden** (Schalter, Standard aus): Zeitplan-Rampe und manueller Regler geben dann nicht mehr die Steuerspannung direkt vor, sondern den gewünschten Anteil der vollen PPFD. Der Master rechnet ihn über die gespeicherte Messung in die passende Steuerspannung um (lineare Interpolation zwischen den 5-%-Stufen, Messrauschen monoton geglättet). 0 % bleibt aus; Werte unterhalb der kleinsten Stufe, ab der die Lampe leuchtet, ergeben diese Stufe (weiter dimmt die Lampe nicht); 100 % wird die erste Stufe, ab der die Lampe nicht mehr heller wird. Beispiel Testlampe (06.11.2026): leuchtet ab 5 % Steuerspannung mit 11 % Licht, voll ab ca. 92 %, dazwischen linear mit +5 Prozentpunkten Versatz → 50 % Licht = 45 % Steuerspannung, 100 % = 95 %. Nach einem Lampenwechsel neu messen. Abbruch, wenn der Lichtausgang kein 0–10V-Ausgang ist oder in einer Stufe 90 s lang gar kein gültiger PPFD-Wert kommt.

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

### VPD-Regelung

Seit FW 1.21.0 (`vpdctrl.cpp`). Führt **Befeuchter und Abluftlüfter gemeinsam** auf einen VPD-Zielbereich statt auf feste Feuchte-/Temperaturgrenzen. Befeuchter und Lüfter behalten ihre eigenen Einstellungen (Steckdose/Ausgang bzw. Mindest-/Höchstdrehzahl; den Schalter „Eigene Steuerung aktiv“ gibt es seit 1.21.5 nicht mehr, der Master steuert den Lüfter immer); nur deren Min/Max- bzw. Modus-Regelung ruht, solange „Regelt den Befeuchter“ bzw. „Regelt den Abluftlüfter“ angehakt ist. Seit 1.21.5 blendet der Klima-Tab diese ruhenden Felder dann aus und zeigt stattdessen einen Hinweis. Abschalten der VPD-Regelung stellt das alte Verhalten sofort wieder her.

#### Zielbereich

| Phase | ☀️ Licht an | 🌙 Licht aus |
|---|---|---|
| Steckling / Keimling | 0,40–0,80 kPa | 0,30–0,60 kPa |
| Wachstum | 0,80–1,20 kPa | 0,60–1,00 kPa |
| Blüte früh | 1,00–1,40 kPa | 0,80–1,20 kPa |
| Blüte spät | 1,20–1,60 kPa | 1,00–1,40 kPa |
| Eigene Werte | frei einstellbar | frei einstellbar |

Beim Wechsel Licht an/aus wandert der Bereich gleitend (max. 0,45 kPa in 30 min), damit Befeuchter und Lüfter nicht schlagartig umschalten.

**Geregelt wird der Blatt-VPD:** Blatttemperatur = Lufttemperatur + Offset (Standard −1,5 °C, nur bei Licht an – nachts sind Blatt und Luft praktisch gleich warm). VPD Blatt = Sättigungsdampfdruck(Blatt) − Dampfdruck der Luft. Angezeigt werden Blatt- und Luft-VPD.

#### Logik (alle 5 s)

- **Befeuchter:** an, sobald der VPD im oberen Fünftel des Bandes liegt **und die Abluft schon auf Mindestdrehzahl steht** (seit FW 1.21.3 — erst drosseln, dann befeuchten), oder sofort, wenn der VPD über dem Band liegt; aus, sobald er unter die Bandmitte fällt. Mindestens 60 s an bzw. aus. **Takt (seit FW 1.21.3):** bei Bedarf läuft er 60 s und pausiert dann 120 s, damit der träge Feuchtesensor die Wirkung sieht, bevor nachgelegt wird (am Stück lief die Feuchte im Log vom 12.11. in 7 min von 46 auf 75 %). Die Klima-Seite zeigt in der Pause „Takt“. Nach einem Neustart wartet die Regelung auf den echten Lichtstatus, statt mit dem Tag-Zielbereich zu starten. Der Blatttemperatur-Offset wird beim Licht-Wechsel ebenfalls gleitend übernommen (max. 3 °C in 30 min, seit FW 1.21.4), sonst springt das Blatt-VPD beim Einschalten schlagartig nach unten und die Abluft fährt grundlos hoch.
- **Abluftlüfter:** integrierend – unter dem Band (zu feucht) → sanft hoch (max. 1 %/5 s, seit FW 1.21.3); in der oberen Bandhälfte, über dem Band oder solange der Befeuchter läuft → schrittweise runter bis zur Mindestdrehzahl (je näher an der Obergrenze, desto schneller; seit FW 1.21.3); zwischen Untergrenze und Mitte bleibt er stehen. Hochgefahren wird **nur**, wenn die Raumluft (AHT21) absolut trockener ist als die Zeltluft (sonst bringt mehr Abluft nichts), gerade kein CO₂ dosiert wird und das Zelt wärmer als `temp_min` ist (Standard 18 °C, seit FW 1.21.2) — darunter wird die Abluft langsam zurückgenommen, damit das Zelt nachts nicht auskühlt. Auf der Startseite stehen „VPD-Regelung“ als Lüftermodus und der VPD-Zielbereich statt Feuchte-Min/Max.
- **Grenzen mit Vorrang:** Zelttemperatur ab `temp_max` − 1 °C → Lüfter rampt bis `temp_max` + 2 °C auf Höchstdrehzahl. Luftfeuchte ≥ `hum_max` → Befeuchter sofort aus, Lüfter rampt bis `hum_max` + 5 % auf Höchstdrehzahl.
- **Ohne gültige Zelt-Messwerte:** Befeuchter aus, Lüfter hält seinen Wert.
- **Alarm:** liegt der VPD `alarm_min` Minuten am Stück außerhalb des Bandes, geht eine [Benachrichtigung](#benachrichtigungen) mit dem Grund raus (Lüfter am Anschlag, Raumluft zu feucht, Befeuchter schafft es nicht, …).

#### Konfiguration (EEPROM 570–582, Magic 0x56)

`enabled` · `phase` · `day_min/max`, `night_min/max` (kPa×100, nur „Eigene Werte“) · `leaf_offset` (Zehntel °C) · `use_humid` · `use_fan` · `temp_min` (°C, Standard 18) · `temp_max` (°C, Standard 28) · `hum_max` (%, Standard 75) · `alarm_min` (Standard 30, 0 = aus). API: `GET /api/vpd` (Konfiguration, Richtwerte aller Phasen, aktueller Zustand), `POST /api/vpd/save`.

SD-Log: `VPD_Blatt_kPa`, `VPD_Soll_min`, `VPD_Soll_max`; die [Auswertung](#auswertung-tab--auswertung-seit-fw-1200) zeigt den Sollbereich als grünes Band und den Blatt-VPD als eigene Kurve.

### Raumlicht & DWC-Timer

Zwei einfache Schalt-Funktionen, die — wie [CO2-](#co2-steuerung) und [Luftbefeuchtung-Steuerung](#luftbefeuchtung-steuerung) — genau einen Ausgang (`target_relay_bit`, 0–3) eines **ESP-NOW-Steckdosen-Nodes** (`target_node_id`) schalten; die übrigen Relais-Bits des Nodes bleiben unangetastet.

#### Raumlicht (EEPROM, Magic 0x3F)

Rein **manueller** Ein/Aus-Schalter, ohne Automatik, Hysterese oder Zeitplan. Der Ausgang wird im Klima-Tab gewählt; sobald einer gewählt ist, erscheint auf der Startseite ein Schalter. Der Ein/Aus-Zustand wird nicht separat gespeichert, sondern live aus der gemeldeten Relaismaske des Ziel-Nodes gelesen — UI und echter Relaiszustand können so nicht auseinanderlaufen. Endpunkte: `GET /api/raumlicht`, `POST /api/raumlicht/save`, `POST /api/raumlicht/toggle`.

#### DWC-Timer (EEPROM, Magic 0x3A)

Einfacher Ein/Aus-Beleuchtungstimer für ein separates DWC-System, wie eine haushaltsübliche Zeitschaltuhr (kein Sonnenauf-/-untergang wie beim [Beleuchtungs-Scheduler](#beleuchtungs-scheduler)): `enabled` · `target_node_id` · `target_relay_bit` · `on_min` / `off_min` (Minuten seit Mitternacht; `off_min < on_min` = über Mitternacht).

### Durchflussmesser & Düsen-Überwachung

Durchflussmesser (YF-S401-ähnlich, Hall-Impulsgeber mit Push-Pull-Ausgang, kein Pull-up nötig) in der gemeinsamen Zulaufleitung **vor** dem Ventil-Verteiler — sieht damit den Durchfluss aller 4 Ventile nacheinander, da laut [Ventilsteuerung](#funktionen) ohnehin immer nur eines gleichzeitig offen ist. Interrupt an `FLOW_SENSOR_PIN` (GPIO3) zählt die Impulse.

**Keine Umrechnung in ml (seit FW 1.19.0):** Die frühere Kalibrierung (Pulse/Liter) samt ml-Anzeige und Summen ist entfallen — die Summen lagen ohnehin nur im RAM und waren nach jedem Neustart weg, und für die Kontrolle der Düsen reicht die rohe Impulsrate. Wer Mengen braucht, fängt einmal eine bekannte Menge auf und rechnet die Impulse aus dem SD-Log selbst um.

#### Düsen-Überwachung (`flowmonitor.cpp`)

Beobachtet den Ventilzustand (`behaelterZustand[]`) von außen: Wechselt ein Behälter in den Zustand „Aktiv“, wird der Impulszähler geleert (verwirft Reste aus dem Druckaufbau); gezählt wird bis zum Ende des Nachlaufs (ein Druckspeicher hinter dem Zähler wird erst dann wieder aufgefüllt), als Dauer zählt die Ventil-offen-Zeit. Impulse und Dauer des letzten Zyklus werden je Behälter angezeigt und ins SD-Log geschrieben.

Pro Behälter wird ein **Sollwert** (Impulse/s) geführt:

- Ist noch keiner hinterlegt (0 bzw. per „Sollwerte löschen“ zurückgesetzt), übernimmt das System automatisch den **nächsten abgeschlossenen Gießzyklus** als neue Referenz — keine manuelle Berechnung nötig. Beim Update auf 1.19.0 werden die alten ml/s-Sollwerte verworfen und neu gelernt.
- Ist einer hinterlegt und der Soll/Ist-Vergleich aktiviert, wird jeder Zyklus mit konfigurierbarer Toleranz (%) verglichen; bei Abweichung nach unten (verstopfte Düse) oder oben (Leck) geht eine [Benachrichtigung](#benachrichtigungen) über alle aktiven Kanäle raus.
- „Sollwerte löschen“ setzt alle Werte auf 0 zurück — praktisch nach dem Düsenreinigen oder einem Umbau der Leitungen, damit nicht die alten Werte fälschlich als Abweichung gewertet werden.

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

Eine CSV-Datei pro Tag (`/JJJJ-MM-TT.csv`) auf der MicroSD-Karte, Schreibintervall **1 Minute** (bis FW 1.20.1: 5 Minuten; rund 0,4 MB pro Tag). Werte einer gerade nicht erreichbaren Quelle (Raumsensor, RS485-Multisensor, Lüfter offline) werden als **leere Zelle** statt eines irreführenden Nullwerts geschrieben.

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
| `ZeltPPFD_umol` | RS485-Multisensor (AS7341), µmol/m²/s — leer, wenn der Lichtsensor nicht ok ist |
| `LichtMaske` | Lichtx4 Relais-Bitmaske |
| `LuefterLeistung_%`, `LuefterRPM` | Abluftlüfter |
| `CO2Ausgang` | CO2-Steuerung Relaiszustand (0/1) |
| `HumidAusgang` | Luftbefeuchtung-Steuerung Relaiszustand (0/1) |
| `ImpulseBehaelter1` … `4` | Durchflussmesser-Impulse des letzten Sprühvorgangs je Behälter (roh, unabhängig von der Kalibrierung) — bei gleicher Dauer ein direktes Maß für die Durchgängigkeit der Düse |
| `DauerBehaelter1_s` … `4` | Dauer dieses Sprühvorgangs (Licht-/Dunkelphase können unterschiedlich lang sein) |
| `Laufzeit_min` | Minuten seit dem letzten Neustart des Masters — springt der Wert zurück, gab es einen Neustart |
| `NeustartGrund` | Grund des letzten Neustarts (Power-On, Software, Watchdog, Brownout, …) |
| `Lichtx4Starts` | Startzähler des Lichtx4 (ab Lichtx4-FW 1.2.0, sonst leer) — steigt er, hat das Modul neu gestartet und alle Lampen waren kurz aus |
| `Lichtx4Grund` | Grund des letzten Lichtx4-Neustarts |
| `Netzaussetzer` / `NetzaussetzerMax_ms` | Anzahl und längste Dauer kurzer Netzaussetzer seit der vorigen Zeile (USV übernimmt, die LED-Treiber gehen dabei kurz aus) |
| `CO2Steckdose` / `HumidSteckdose` | Von der Steckdose selbst zurückgemeldeter Zustand des CO2-/Befeuchter-Relais (0/1); leer = Steckdose offline. Weicht er von `CO2Ausgang`/`HumidAusgang` (Wunsch der Regelung) ab, kommt der Schaltbefehl nicht an |
| `WLAN_dBm` / `WLAN_Abbrueche` | WLAN-Signalstärke (leer ohne Verbindung) und Zahl der WLAN-Abbrüche seit der vorigen Zeile |
| `EspNow_online` | ESP-NOW-Nodes online / gekoppelt, z. B. `2/3` |
| `HeapMin_kB` | Kleinster freier Arbeitsspeicher seit dem Start |
| `MS2_Abfragen` / `MS2_Timeouts` | RS485-Abfragen an den Zelt-Sensor (Adresse 0x20, alle 15 s) seit der vorigen Zeile und wie viele davon unbeantwortet blieben. Eine einzige ausbleibende Antwort genügt, damit der Master den Sensor bis zur nächsten erfolgreichen Abfrage als offline führt (leere Zelt-Spalten) — die Zähler zeigen solche Aussetzer auch zwischen zwei Log-Zeilen (seit FW 1.20.2) |
| `VPD_Blatt_kPa`, `VPD_Soll_min`, `VPD_Soll_max` | [VPD-Regelung](#vpd-regelung): geregelter Blatt-VPD und aktueller (ggf. gerade wandernder) Zielbereich; leer, wenn die Regelung aus ist (seit FW 1.21.0) |

> **Automatische Migration:** Trifft die Firmware beim Schreiben auf eine bereits vorhandene Tagesdatei mit dem alten, kürzeren Spaltenformat (vor dieser Erweiterung), wird diese zu `_alt.csv` umbenannt (Daten bleiben erhalten) und eine neue Datei mit vollständigem Header begonnen — kein Vermischen unterschiedlicher Spaltenzahlen in einer Datei.

#### Download & Löschen (Konfiguration → Daten-Log)

| Endpunkt | Methode | Funktion |
|---|---|---|
| `/api/logs` | GET | Liste aller `*.csv`- und `*.txt`-Dateien mit Dateigröße |
| `/api/logs/download?file=NAME` | GET | Streamt die Datei mit `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Löscht die Datei (UI fragt vorher per Bestätigungsdialog nach) |
| `/api/logs/delete-all` | POST | Löscht alle `*.csv`/`*.txt`-Dateien auf der SD-Karte, Antwort enthält die Anzahl (UI fragt vorher nach) |

Dateinamen werden serverseitig gegen Pfad-Traversal geprüft (kein `/` oder `..` zulässig).

#### Auswertung (Tab „📈 Auswertung“, seit FW 1.20.0)

Diagramme direkt aus den Tageslogs, komplett im Browser berechnet (`page_charts.h`): Der ESP liefert nur die CSV-Dateien über die beiden Endpunkte oben aus, die Seite zeichnet selbst auf `<canvas>` (keine externe Bibliothek, funktioniert auch ohne Internet). Spalten werden über den Header-Namen gefunden, ältere Dateien (`_alt.csv`, nur 3 Behälter) funktionieren ebenso.

- **Zeitraum** 1 Tag oder 1 Woche bis zum gewählten Tag; **Ausschnitt** 1 h … 7 Tage, Laufleiste und ◀/▶ zum Verschieben, Mausrad/Zwei-Finger zum Zoomen, Ziehen/Wischen zum Verschieben, Doppelklick zeigt alles.
- **Diagramme** (je Kurve abschaltbar, Auswahl bleibt im Browser gespeichert): Temperatur (Zelt, Raum/draußen, Nährlösung, Wurzelraum), Luftfeuchte Zelt/Raum + VPD auf eigener Achse mit grünem Zielband 0,8–1,2 kPa, Licht (PPFD), CO₂, Füllstand, Aktoren (Lüfter %, Befeuchter- und CO₂-Ausgang als Balken), Verbindung (WLAN-Signal, unbeantwortete RS485-Abfragen an den Zelt-Sensor). Alle Diagramme teilen sich die Zeitachse; der Cursor zeigt die Werte aller Kurven zur selben Uhrzeit in der Legende.
- **Markierungen:** grau hinterlegt = Licht aus (`LichtMaske` = 0), rot gestrichelt ↻ = Neustart (aus `Laufzeit_min`), orange ⚡ = Netzaussetzer, graues ▲ = Zelt-Sensor nicht erreichbar (Zelt-Temperatur und -Feuchte leer). Kurze Messlücken (unter 16 min) werden in den Linien überbrückt, längere unterbrechen sie; Sensor-Aussetzer werden zusätzlich in der Ereignis-Karte und der Tagesübersicht gezählt.
- **Kennzahlen** für den sichtbaren Ausschnitt: ⌀/min/max je Kurve, getrennt nach Licht an/aus, Anteil VPD im Zielbereich bei Licht, Lichtsumme, Zahl der Neustarts/Netzaussetzer.
- **Tagesübersicht:** je Tag DLI (mol/m²·d aus PPFD), Lichtstunden, Zelt-Temperatur und -Feuchte Tag/Nacht, VPD bei Licht und Anteil im Zielbereich, Zahl der Gießzyklen, Neustarts. Klick auf eine Zeile springt zu diesem Tag.
- **Düsen-Auswertung:** jeder Gießzyklus als Punkt, je Behälter ein gleitender Median über 9 Zyklen, Ausreißer (>15 % vom Median) rot umringt. Anzeige als Impulse pro Sekunde (bleibt vergleichbar, wenn die Öffnungszeit geändert wurde) oder Impulse je Zyklus. Je Behälter eine Karte mit Median, Streuung, Trend (letztes gegen erstes Drittel der Zyklen im Ausschnitt), Ausreißern, Abweichung zum Durchschnitt aller Behälter und einer Ampel: 🟢 stabil · 🟠 Trend ≤ −10 % (Düse/Filter setzt sich zu) bzw. ≥ +10 % (Leck, lose Düse) · 🟡 Streuung > 15 % (Luft, Schmutz) · ⚪ weniger als 6 Zyklen.

Der laufende Tag wird bei „Neu laden“ jedes Mal frisch geholt, ältere Tage nur einmal pro Seitenaufruf.

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

> **Kein Licht-SSR-Modus mehr:** Der frühere zweite Node-Typ `NODE_TYPE_LIGHT_SSR (0x05)` ist aus der Firmware entfernt — Lichtsteuerung macht der RS485-Knoten [Lichtx4](#lichtx4). Ein Steckdosen-Node meldet sich immer als `NODE_TYPE_SOCKET (0x03)`, auch wenn im NVS noch ein alter Typ gespeichert ist.

#### Relay-Befehl

```c
// Alle 4 Relais einschalten:
MSG_COMMAND + CMD_SET_ONOFF (0x02) + payload[1] = 0x0F

// Nur R1 + R3:
payload[1] = 0b00000101  // = 0x05
```

### Erststart & Konfiguration

#### Steckdosen — Node-ID

Beim allerersten Start (NVS-Namespace `"nodecfg"` leer) erscheint im Serial-Monitor:

```
=== ERSTSTART: Node-Konfiguration ===
Node-ID eingeben (1-254) + Enter (60s Timeout, sonst 3):
> 10
[NODE] Gespeichert: Steckdose  ID=0x0A
```

Gespeichert in NVS `"nodecfg"`: Key `id` (uint8). Mit `NODECLEAR` löschen → Dialog erscheint beim nächsten Boot.

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

Wenn bei allen Setup-Dialogen (Node-ID, Kanal) innerhalb von 60 Sekunden keine Eingabe erfolgt, wird ein Standardwert verwendet: ID=3 / Kanal 6.

---

### Serial-Kommandos

Baudrate **115200** · Zeilenende **LF** oder **CR+LF**

#### Steckdosen

| Kommando | Wirkung |
|---|---|
| `STATUS` | Aktuelle Relais-Bitmaske und Einzelstatus R1–R4 |
| `MASK <0–15>` | Alle Relais per Dezimalwert setzen (z. B. `MASK 5` → R1+R3 ein) |
| `R<1–4> <0\|1>` | Einzelrelais schalten (z. B. `R2 1`) |
| `NODEINFO` | Node-ID und Kopplungsstatus anzeigen |
| `NODECLEAR` | Node-ID löschen → Erststart-Dialog beim nächsten Neustart |
| `UNPAIR` | Kopplung mit dem Master lösen (wie BOOT-Taster 3 s) |
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
- [VPD Control](#vpd-control)
- [Room Light & DWC Timer](#room-light--dwc-timer)
- [Flow Sensor & Nozzle Monitoring](#flow-sensor--nozzle-monitoring)
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

The RS485 devices are hard-wired and polled cyclically — see [RS485 Bus](#rs485-bus-1). ESP-NOW nodes are paired to the master (see [Fixed Pairing](#fixed-pairing-node--controller)), register on startup and send periodic heartbeats; the master detects their online/offline status automatically.

**WiFi watchdog:** if the WiFi connection drops and the ESP32 does not restore it by itself within 30 s, the master reconnects once; if that does not work within 60 s, it restarts. Without WiFi, ESP-NOW is down as well (same radio, sockets no longer switch). This is reported (alert *Connection loss*) after reconnecting or after the restart, with duration and reason code.

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
| `MSG_UNPAIR` | `0x50` | Master → node | Release pairing, node forgets its master |

#### Fixed Pairing Node ↔ Controller

So that two controllers within radio range do not take each other's nodes, nodes are paired to one master:

- **Node:** stores the MAC of the master that sends it the first `MSG_REGISTER_ACK` in NVS (`espnow_pair`/`master`) and from then on only accepts packets from that MAC. While unpaired it accepts nothing but a `REGISTER_ACK` — switching commands from a foreign master are never executed.
- **Master:** keeps a pairing list in NVS (`espnow_pair`/`nodes`) and only answers nodes on it. Unknown nodes appear in the web interface (System → Node Control) under *Newly found* and are only accepted via **Pair**. If the node ID is already used by another node, pairing is blocked (change the ID on the node with `NODECLEAR`).
- **Unpairing:** in the web interface on the node card (master sends `MSG_UNPAIR`) or on the node itself by **holding the BOOT button for 3 s while running** or the serial command `UNPAIR`. The node then pairs with the first controller that has it on its list or where *Pair* is pressed — so before moving it to another controller, also unpair it on the old one.
- There is no automatic pairing — even after updating to a master firmware with pairing, each existing node has to be accepted once via *Pair*.

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
| `3` | `FLOW_SENSOR_PIN` | Flow meter (YF-S401-like, Hall pulse output, push-pull) — see [Flow Sensor & Nozzle Monitoring](#flow-sensor--nozzle-monitoring) |
| `4` | `DS18B20_PIN` | OneWire — DS18B20 (reservoir, plant, optional internal temperature), assigned by ROM address (see [Features](#features)) |
| `5` | `PRESSURE_PIN` | Analog — pressure sensor 0.5–4.5 V ratiometric (can be disabled in the web UI, see [Pressure Sensor](#pressure-sensor-pump-pressure)) |
| `6` | `GEHAEUSE_FAN_PIN` | Enclosure fan, on/off (no PWM), temperature-controlled via the internal DS18B20 |
| `7` | `VENTIL2_PIN` | Solenoid valve reservoir 2 (direct GPIO) |
| `8` | `VENTIL1_PIN` | Solenoid valve reservoir 1 |
| `9` | `VENTIL4_PIN` | Valve reservoir 4 (return valve up to FW 1.18) |
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
| `VENTIL4_PIN` | MV4 | Supply reservoir 4 |

#### Main Pump & Circulation Pump

The main pump (`PUMPE_GPIO`) switches on automatically whenever any reservoir is actively watering (valve open) — regardless of whether the normal or batch timing mode is active (see [Valve Control](#features)).

**Pressure build-up:** Before every spray cycle the pump runs for a configurable time (default 5 s, 0 = off) with the nozzle valve still closed; only then does the valve open. Sequence per reservoir: pause → pressure build-up → active → refill. Without it, every cycle sprayed at a still-rising pressure — volume and flow pulses varied noticeably. Also applies in batch mode before each individual reservoir (the pump keeps running in between) and to the manual single cycle. Requirement: the pump must be allowed to run briefly against closed valves (system under pressure / pressure switch).

**Refill (Nachlauf):** After the nozzle valve closes, the pump keeps running for the same time again (state "Nachlauf"). Intended for a pressure accumulator between flow meter and valves: it bridges the pressure drop when a valve opens (the restrictor nozzle in the flow meter cannot supply fast enough) and is refilled during this phase. Flow measurement counts from valve opening until the end of the refill, so water sprayed from the accumulator is attributed to the right reservoir; only the valve-open time counts as duration. In batch mode the next reservoir opens directly after the previous one's refill (no separate build-up any more, only before the first reservoir). Without an accumulator practically nothing flows during the refill, so readings stay the same. The manual single cycle has no refill phase.

The circulation pump (`UMWAELZPUMPE_GPIO`, reservoir recirculation) runs a configurable run/pause cycle (`laufzeit_s` / `pausenzeit_min`) whenever any reservoir is enabled at all — regardless of whether that reservoir is currently watering or just pausing between cycles. If no reservoir is enabled, it stays off.

### Features

#### Network

W5500 (Ethernet) and WiFi run simultaneously via the lwIP stack. On Ethernet link-up, **NTP** is fetched automatically and the RTC is set (threshold: ±60 s deviation, resynced every 60 min). The web interface is available on both IPs with no code changes.

Without a router: the master starts as an **AP on channel 6** (`ESPNOW_DEFAULT_CHANNEL`). Nodes in "no WiFi" mode use the same channel.

#### Valve Control

Four independent irrigation circuits (MV1–MV4) with configurable open time (sec.) and pause interval (min., separately configurable for the light and dark phases). In "same times for all" mode, a single shared batch run replaces the independent timers (all active reservoirs directly in sequence, one shared pause).

**Rebuild FW 1.19.0 (11/2026):** MV4 used to be the return valve with an optional "return pre-flush" before each cycle; two reservoirs shared one valve. The return line is now a manual ball valve, MV4 supplies reservoir 4, so every reservoir has its own valve and its own flow pulses. The pre-flush is gone. On the first start with 1.19.0, reservoir 4 takes over the times of reservoir 3 but stays disabled until it is switched on on the valve page. Reservoir 4's configuration lives in its own EEPROM block (560–565, nozzle-monitoring target 566–569); the EEPROM area grows from 570 to 600 bytes for this, existing settings are kept.

After a restart, watering starts **immediately** instead of first waiting out the full pause time (up to 60 min).

**Manual single cycle:** On the Aeroponik page, a single watering cycle can be triggered manually per reservoir — the valve opens once for the currently valid open time (light or dark phase) and closes again automatically (`/api/ventile/dose/start`, `/status`, `/cancel`). Normal automation is paused meanwhile.

**Output test:** The direct output test (wiring check, manual single cycle) no longer interrupts a running watering cycle — if a valve/pump is currently active automatically, test mode only becomes active once the cycle has finished on its own. The UI shows the waiting state (`GET /api/output/test`). During a manual single cycle the circulation pump keeps its own schedule and doesn't delay the start. **Safety net (since FW 1.19.2):** a forgotten test mode pauses watering — so it ends automatically after 30 minutes at the latest, automation resumes and a [notification](#notifications) goes out.

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
| Tent fan RPM | Actual RPM deviates from the expected value for at least 3 minutes in a row (spin-up/module restart do not trigger an alarm), see [Lueftermodul](#lueftermodul-1) |
| Nozzle flow | Target/actual deviation per reservoir, see [Nozzle Monitoring](#flow-sensor--nozzle-monitoring) |

Each channel has its own rate limit (30 s) against accidental spamming; a dedicated test button per channel in the configuration verifies its credentials independently of the others.

#### SD Logging

Every minute (up to FW 1.20.1: every 5 minutes), all measurement and control values are written to CSV files following the scheme `/YYYY-MM-DD.csv`, including room/tent climate, fan and light status. Details, full column list, and download/delete via the web interface: see [Data Log & Download](#data-log--download).

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

Simplified Modbus RTU: **FC03** (Read Holding Registers) for reading, **FC06** (Write Single Register) for writing. CRC16 per the Modbus standard (polynomial `0xA001`). A lean state-machine model (`mb_state_t`) polls the bus participants in turn; only one request outstanding at a time, response timeout **2000 ms**. Before every request the master keeps at least **10 ms** of bus silence (since FW 1.21.4): the slaves detect the end of a frame by a 4 ms gap; without the pause the MS2 request stuck to the 39-byte reply of the MARS fan and got lost (cause of the sporadic `MS2_Timeouts`, found via the tent logger on 12 Nov). The Multimodul (from FW 1.3.4) additionally extracts a valid request at the end of a glued frame and answers it.

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

#### Firmware Update via RS485

From firmware 1.1.0 on (to be flashed once via USB/programming pads), the Lichtx4 can also be updated in place: web interface → System → **Module Update (RS485)**, select "Lichtx4", file `Lichtx4/.pio/build/nologo_esp32c3_super_mini/firmware.bin`. Same protocol as the [Multimodul](#multimodul-1) (function code `0x41`, here at address `0x40`, `Lichtx4/src/fw_update.h`). The image is written to the second OTA app partition and only activated after a successful MD5 check; the first block must be an ESP image (magic `0xE9`) with the ESP32-C3 chip ID. Takes about 1–2 min (~320 KB). The relay mask survives the restart via `Preferences`.

> From this firmware on, the Lichtx4 runs at 115200 baud — afterwards remove `#define LICHTX4_LEGACY_9600` from `Aeroponik/src/local_config.h` (see [RS485 Bus](#rs485-bus-1)), otherwise the master keeps talking to it at 9600.

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
| Auto — humidity | Tent humidity (RS485 multisensor) | Linear ramp between `hum_min` (minimum speed) and `hum_max` (maximum speed) |
| Auto — temperature, absolute | Tent temperature | Ramp between `temp_min`/`temp_max`; optional **differential cap**: if the tent−room difference is below `temp_cap_diff`, only the minimum speed is driven regardless of the ramp (prevents full blast when the room is nearly as warm as the tent) |
| Auto — temperature, differential | Tent temperature − room temperature | Ramp directly over the temperature differential between `temp_min`/`temp_max` |

A shared minimum and maximum speed (`min_speed`, `max_speed`, default 100 %) applies to both auto modes. At 0 % the MARS fan's enable register (reg15) is also set to 0, otherwise it keeps running at base speed. The differential modes require the [room climate sensor](#room-climate--vpd); if it is unavailable, the last target power remains unchanged.
**Pulsed mode below the minimum speed (since FW 1.21.3, MARS only):** the MARS fan runs at its built-in minimum speed (~900 RPM, reports 25 %) for any value > 0; below that it only knows "off". The master therefore turns setpoints of 1–24 % into a 10-minute cycle: the fan runs setpoint/25 of the time at minimum (at least 1 min at a time) and is off otherwise — e.g. 5 % = 2 of 10 min, 10 % = 4 of 10 min. The minimum speed thus becomes the base ventilation, 0 % allows switching off completely. While pulsing, the home page shows the setpoint with ⏱ instead of the read-back value.

#### Configuration (EEPROM, Magic 0x92)

`mode` · `manual_percent` · `min_speed` · `max_speed` (separate EEPROM block, magic 0x6B) · `hum_min/max` · `temp_min/max` · `temp_mode` (absolute/differential) · `temp_cap_diff` · `enabled`

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

- **SCD41 + AS7341** (I2C `0x62` / `0x39`) — the same sensors as in the [Multisensor](#multisensor-2) node. When at least one of them is present, the board additionally answers RS485 address `0x20` with the same 14-register layout as the [RS485 multisensor](#rs485-multisensor-tent). Without SCD41, the AS7341 runs on its own on a 10 s cycle.
- **AHT21 + ENS160** (I2C `0x38` / `0x53` or `0x52`, e.g. DFRobot combo module SEN0515) — replacement for the SCD41, only used if no SCD41 is found. Temperature/humidity from the AHT21, CO₂ as the ENS160's **eCO₂ estimate** (derived from volatile organic compounds, not a real NDIR CO₂ reading — only of limited use for CO₂ control). During the ENS160 warm-up (up to about 1 h after its first start) the CO₂ value is treated as invalid.

**Status register `0x20`/13** (evaluated by the master): bit0 = temperature/humidity valid (SCD41 or AHT21), bit1 = AS7341 ok, bit2 = CO₂ invalid (no ENS160 or warm-up), bit3 = CO₂ is only an estimate (ENS160). With invalid values the master skips CO₂/humidity control, fan automation and the CSV columns; an eCO₂ value is shown on the home page with a leading `~`.
- **GP8403** (I2C `0x58`, 2-channel 0–10V DAC) — when present, the board additionally answers RS485 address `0x50` with the same 2-register layout as the [analog output module](#analog-output-module).

On boot, the firmware scans the I2C bus; only devices actually found are initialized and have their RS485 address activated. If a device is missing, its address simply stays unanswered on the bus — indistinguishable, from the master's point of view, from a separate Multisensor/analog module that was never installed. This means **no code change is needed on the master**: it already polls `0x20`, `0x50`, and `0x51` independently and already treats a missing response as "device offline".

Depending on what's populated, a Multimodul can therefore be a plain fan module, a fan module with tent climate sensing, a fan module with a 0–10V output, or all of it at once — on the same RS485 bus, with no separate firmware variants.

> **Boot order:** `rs485_slave_init()` deliberately runs at the very start of `setup()`, **before** the I2C scan/sensor init — otherwise the DI line (GP28) would stay unconfigured/high-impedance for as long as the I2C scan or `scd41_init()`/`gp8403_init()` run (which can take arbitrarily long if an I2C transaction hangs), leaving the bus without a defined idle level. The firmware also logs the reset reason at boot (`rp2040.getResetReason()`: power-on, watchdog, brownout, glitch, …) — useful for diagnosing unexplained reboots, since the project itself doesn't use a software watchdog.

> **SCD41 and AS7341 independent:** CO₂/temperature/humidity (SCD41) are written to the `0x20` registers right after each measurement, independently of the AS7341. If an AS7341 measurement hangs for more than 6 s, only the AS7341 is reinitialized (no I2C bus reset that would take the SCD41 down with it); after 3 failed attempts it is disabled for 10 min (light values = 0, gain = `0xFF`, status bit 1 = 0) and then retried automatically — the board, SCD41, fans, and analog output stay online the whole time.

#### Firmware Update via RS485

The Multimodul can be updated in place over the RS485 bus: web interface → System → **Module Update (RS485)** (select "Multimodul"), file `Multimodul/.pio/build/waveshare_rp2040_zero/firmware.bin` (not `.uf2`). The update function must have been flashed once via USB for this (firmware 1.1.0 or later).

Flow: the master buffers the file in PSRAM, computes its MD5, and transfers it in 200-byte blocks via a dedicated function code `0x41` to address `0x51` (protocol: `Multimodul/src/fw_update.h`). Normal polling is paused meanwhile. The Multimodul first writes the image as a file into its LittleFS (`board_build.filesystem_size = 0.5m`), verifies the MD5, and only then stages it for the OTA bootloader, which applies it on restart. Afterwards the master queries the new version; fan and analog setpoints are rewritten automatically via the boot ID, as after any restart.

Safeguards: if the transfer is interrupted (timeout after 5 retries, abort button, master restart), the running firmware stays untouched. Lost responses don't cause blocks to be written twice — the slave always reports the next expected offset. The master rejects ESP32 and `.uf2` files, and the Multimodul compares the first block (boot2) against its own flash, so firmware for a different board can't be flashed. The firmware version (`FW_VERSION_*` in `fw_update.h`) should be bumped with every update, otherwise the UI shows a notice.

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

#### 0–10V Output Connector Pinout — Note: PCB Layout Is Wrong

The 0–10V outputs use the same connector type as the RS485 ports: RJ12 modular jack (6P6C, i.e. 6 positions, all 6 contacts fitted) and 6P6C modular plug. Their current pinout on the PCB, however, does **not** match the lamps (e.g. Mars Hydro FC-E1500):

| Pin | Current PCB assignment | Wire |
|---|---|---|
| 3 | GND (DAC ground) | red |
| 4 | 0–10 V signal (wiper, GP8403 output) | green |

Required instead:

| Pin | Signal |
|---|---|
| 3 + 4 | GND |
| 2 + 5 | 0–10 V |

Until the PCB is corrected, an **adapter cable** has to be made. Nothing changes in the software.

> **Pinout not guaranteed:** this assignment is only a de facto standard and is not the same on every device. Before connecting a new lamp or fan model, check its pinout (datasheet or measure it).

#### Lamp Response Curve (PPFD)

Climate → **Lamp Response Curve (PPFD)** measures how linearly a lamp on the 0–10V light output (analog module or GP8403) responds to the control voltage. The master sets the output to 0, 5, 10, … 100 % in turn (21 steps), waits 10 s per step (one AS7341 measurement cycle, so no reading from before the switch counts), then polls the multisensor (`0x20`) every 5 s instead of every 15 s and moves on as soon as 3 consecutive readings lie within 1 % (at least 0.5 µmol/m²/s) — their mean is the step's value. If a step does not become stable within 90 s, the mean of the last readings is used. About 8–10 min in total. The schedule is overridden meanwhile and takes over again afterwards.

Result: table and chart with PPFD per step, relative to 100 % (the 0 % value is subtracted as residual light) and the deviation from an ideal straight line. The last complete measurement is stored in NVS (`lichtkal`) and shown when the page is opened.

**Apply response curve** (switch, off by default): the schedule ramp and the manual slider then no longer set the control voltage directly but the desired share of full PPFD. The master converts it into the matching control voltage using the stored measurement (linear interpolation between the 5 % steps, measurement noise smoothed to be monotonic). 0 % stays off; values below the lowest step at which the lamp lights up give that step (the lamp does not dim any further); 100 % becomes the first step from which the lamp no longer gets brighter. Example test lamp (2026-11-06): lights up from 5 % control voltage at 11 % light, full from about 92 %, linear in between with a +5 percentage point offset → 50 % light = 45 % control voltage, 100 % = 95 %. Measure again after changing the lamp. Aborts if the light output is not a 0–10V output or if no valid PPFD reading at all arrives within 90 s of a step.

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

### VPD Control

Since FW 1.21.0 (`vpdctrl.cpp`). Drives the **humidifier and the exhaust fan together** towards a VPD target range instead of fixed humidity/temperature limits. Humidifier and fan keep their own settings (socket/output, or min/max speed; the "own control active" switch was removed in 1.21.5, the master always drives the fan); only their min/max or mode logic is bypassed while "Controls the humidifier" or "Controls the exhaust fan" is ticked. Since 1.21.5 the Climate tab hides those resting fields and shows a note instead. Switching VPD control off restores the previous behaviour immediately.

#### Target range

| Stage | ☀️ Lights on | 🌙 Lights off |
|---|---|---|
| Clone / seedling | 0.40–0.80 kPa | 0.30–0.60 kPa |
| Vegetative | 0.80–1.20 kPa | 0.60–1.00 kPa |
| Early flower | 1.00–1.40 kPa | 0.80–1.20 kPa |
| Late flower | 1.20–1.60 kPa | 1.00–1.40 kPa |
| Custom | freely adjustable | freely adjustable |

When the lights switch on/off, the range moves gradually (max. 0.45 kPa in 30 min) so humidifier and fan don't switch abruptly.

**The leaf VPD is controlled:** leaf temperature = air temperature + offset (default −1.5 °C, lights on only – at night leaf and air are practically the same temperature). Leaf VPD = saturation vapour pressure(leaf) − vapour pressure of the air. Both leaf and air VPD are shown.

#### Logic (every 5 s)

- **Humidifier:** on as soon as the VPD is in the upper fifth of the band **and the exhaust is already at minimum speed** (since FW 1.21.3 — throttle first, then humidify), or immediately when the VPD is above the band; off once it drops below the middle of the band. At least 60 s on or off. **Pulsing (since FW 1.21.3):** when needed it runs 60 s and then pauses 120 s so the slow humidity sensor sees the effect before more is added (running continuously, humidity went from 46 to 75 % in 7 min in the log of 12 Nov). The climate page shows "pulse" during the pause. After a restart the control waits for the real light status instead of starting with the day target. The leaf temperature offset is also applied gradually when the light switches (max. 3 °C in 30 min, since FW 1.21.4); otherwise the leaf VPD drops abruptly at lights-on and the exhaust ramps up for no reason.
- **Exhaust fan:** integrating – below the band (too humid) → gently up (max. 1 %/5 s, since FW 1.21.3); in the upper half of the band, above the band or while the humidifier runs → step by step down to the minimum speed (faster the closer to the upper limit; since FW 1.21.3); between lower limit and middle it holds. It is **only** increased if the room air (AHT21) is absolutely drier than the tent air, no CO₂ is being dosed and the tent is warmer than `temp_min` (default 18 °C, since FW 1.21.2) — below that the exhaust is slowly reduced so the tent does not cool down at night. The home page then shows "VPD control" as fan mode and the VPD target instead of humidity min/max.
- **Limits with priority:** tent temperature from `temp_max` − 1 °C → fan ramps to maximum speed at `temp_max` + 2 °C. Humidity ≥ `hum_max` → humidifier off immediately, fan ramps to maximum speed at `hum_max` + 5 %.
- **Without valid tent readings:** humidifier off, fan holds its value.
- **Alarm:** if the VPD stays outside the band for `alarm_min` minutes in a row, a [notification](#notifications) with the reason goes out (fan at maximum, room air too humid, humidifier can't keep up, …).

#### Configuration (EEPROM 570–582, Magic 0x56)

`enabled` · `phase` · `day_min/max`, `night_min/max` (kPa×100, "custom" only) · `leaf_offset` (tenths of °C) · `use_humid` · `use_fan` · `temp_min` (°C, default 18) · `temp_max` (°C, default 28) · `hum_max` (%, default 75) · `alarm_min` (default 30, 0 = off). API: `GET /api/vpd` (configuration, reference values of all stages, current state), `POST /api/vpd/save`.

SD log: `VPD_Blatt_kPa`, `VPD_Soll_min`, `VPD_Soll_max`; the [analysis](#analysis-tab--analysis-since-fw-1200) shows the target range as a green band and the leaf VPD as its own curve.

### Room Light & DWC Timer

Two simple switching features that — like [CO2](#co2-control) and [humidity control](#humidity-control) — switch exactly one output (`target_relay_bit`, 0–3) of an **ESP-NOW socket node** (`target_node_id`); the node's other relay bits are left untouched.

#### Room Light (EEPROM, Magic 0x3F)

A purely **manual** on/off switch, with no automation, hysteresis, or schedule. The output is selected in the climate tab; once one is selected, a switch appears on the home page. The on/off state isn't stored separately but read live from the target node's reported relay mask — so the UI and the actual relay state can't drift apart. Endpoints: `GET /api/raumlicht`, `POST /api/raumlicht/save`, `POST /api/raumlicht/toggle`.

#### DWC Timer (EEPROM, Magic 0x3A)

A simple on/off lighting timer for a separate DWC system, like an ordinary household timer switch (no sunrise/sunset like the [lighting scheduler](#lighting-scheduler)): `enabled` · `target_node_id` · `target_relay_bit` · `on_min` / `off_min` (minutes since midnight; `off_min < on_min` = across midnight).

### Flow Sensor & Nozzle Monitoring

Flow meter (YF-S401-like, Hall pulse output with a push-pull driver, no pull-up needed) in the shared supply line **before** the valve manifold — so it sees the flow of all 4 valves in turn, since per [valve control](#features) only one is ever open at a time anyway. An interrupt on `FLOW_SENSOR_PIN` (GPIO3) counts the pulses.

**No conversion to ml (since FW 1.19.0):** The former calibration (pulses/liter) together with the ml display and totals has been removed — the totals lived in RAM only and were lost on every restart, and the raw pulse rate is enough to keep an eye on the nozzles. If you need amounts, collect a known amount once and convert the pulses from the SD log yourself.

#### Nozzle Monitoring (`flowmonitor.cpp`)

Observes the valve state (`behaelterZustand[]`) from the outside: when a reservoir switches to the "active" state, the pulse counter is cleared (discarding leftovers from the pressure build-up); counting runs until the end of the refill phase (a pressure accumulator behind the meter is only refilled then), the duration is the valve-open time. Pulses and duration of the last cycle are shown per reservoir and written to the SD log.

Each reservoir keeps a **target value** (pulses/s):

- If none is set yet (0, or reset via "Clear targets"), the system automatically adopts the **next completed watering cycle** as the new reference — no manual calculation needed. When updating to 1.19.0 the old ml/s targets are discarded and learned again.
- If one is set and the target/actual comparison is enabled, every cycle is compared with a configurable tolerance (%); on deviation below (clogged nozzle) or above (leak), a [notification](#notifications) goes out over all active channels.
- "Clear targets" resets all values to 0 — handy after cleaning the nozzles or reworking the plumbing, so the old values aren't wrongly flagged as a deviation.

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

One CSV file per day (`/YYYY-MM-DD.csv`) on the MicroSD card, write interval **1 minute** (up to FW 1.20.1: 5 minutes; about 0.4 MB per day). Values from a currently unreachable source (room sensor, RS485 multisensor, fan offline) are written as an **empty cell** instead of a misleading zero.

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
| `ZeltPPFD_umol` | RS485 multisensor (AS7341), µmol/m²/s — empty if the light sensor is not ok |
| `LichtMaske` | Lichtx4 relay bitmask |
| `LuefterLeistung_%`, `LuefterRPM` | Exhaust fan |
| `CO2Ausgang` | CO2 control relay state (0/1) |
| `HumidAusgang` | Humidity control relay state (0/1) |
| `ImpulseBehaelter1` … `4` | Flow meter pulses of the last spray cycle per reservoir (raw, independent of calibration) — at equal duration a direct measure of nozzle throughput |
| `DauerBehaelter1_s` … `4` | Duration of that spray cycle (light/dark phase may differ) |
| `Laufzeit_min` | Minutes since the master's last restart — if the value drops, a restart occurred |
| `NeustartGrund` | Reason of the last restart (power-on, software, watchdog, brownout, …) |
| `Lichtx4Starts` | Lichtx4 boot counter (Lichtx4 FW 1.2.0 or later, otherwise empty) — if it increases, the module restarted and all lamps were briefly off |
| `Lichtx4Grund` | Reason of the last Lichtx4 restart |
| `Netzaussetzer` / `NetzaussetzerMax_ms` | Number and longest duration of short mains dropouts since the previous row (UPS takes over, the LED drivers briefly switch off) |
| `CO2Steckdose` / `HumidSteckdose` | State of the CO2/humidifier relay as reported back by the socket itself (0/1); empty = socket offline. If it differs from `CO2Ausgang`/`HumidAusgang` (what the control wants), the switching command is not getting through |
| `WLAN_dBm` / `WLAN_Abbrueche` | WiFi signal strength (empty without connection) and number of WiFi disconnects since the previous row |
| `EspNow_online` | ESP-NOW nodes online / paired, e.g. `2/3` |
| `HeapMin_kB` | Lowest free memory since startup |
| `MS2_Abfragen` / `MS2_Timeouts` | RS485 polls of the tent sensor (address 0x20, every 15 s) since the previous row and how many of them went unanswered. A single missing reply is enough for the master to treat the sensor as offline until the next successful poll (empty tent columns) — the counters show such dropouts even between two log rows (since FW 1.20.2) |
| `VPD_Blatt_kPa`, `VPD_Soll_min`, `VPD_Soll_max` | [VPD control](#vpd-control): controlled leaf VPD and current (possibly moving) target range; empty when the control is off (since FW 1.21.0) |

> **Automatic migration:** If the firmware encounters an existing daily file with the old, shorter column format (prior to this extension) when writing, it renames it to `_alt.csv` (data is preserved) and starts a new file with the full header — no mixing of different column counts within one file.

#### Download & Delete (Configuration → Data Log)

| Endpoint | Method | Function |
|---|---|---|
| `/api/logs` | GET | List of all `*.csv` and `*.txt` files with file size |
| `/api/logs/download?file=NAME` | GET | Streams the file with `Content-Disposition: attachment` |
| `/api/logs/delete` | POST (JSON `{"file":"NAME"}`) | Deletes the file (UI asks for confirmation first) |
| `/api/logs/delete-all` | POST | Deletes all `*.csv`/`*.txt` files on the SD card, response contains the count (UI asks for confirmation first) |

Filenames are checked server-side against path traversal (no `/` or `..` allowed).

#### Analysis (tab "📈 Analysis", since FW 1.20.0)

Charts straight from the daily logs, computed entirely in the browser (`page_charts.h`): the ESP only serves the CSV files via the two endpoints above, the page draws on `<canvas>` itself (no external library, works without internet too). Columns are looked up by header name, so older files (`_alt.csv`, only 3 reservoirs) work as well.

- **Period** 1 day or 1 week up to the selected day; **window** 1 h … 7 days, slider and ◀/▶ to pan, mouse wheel/pinch to zoom, drag/swipe to pan, double-click shows everything.
- **Charts** (each curve can be toggled, the choice is remembered in the browser): temperature (tent, room/outside, nutrient solution, root zone), humidity tent/room + VPD on its own axis with a green target band 0.8–1.2 kPa, light (PPFD), CO₂, tank level, actuators (fan %, humidifier and CO₂ output as bars), connection (WiFi signal, unanswered RS485 polls of the tent sensor). All charts share the time axis; the cursor shows the values of all curves at the same time in the legend.
- **Markers:** grey background = lights off (`LichtMaske` = 0), red dashed ↻ = restart (from `Laufzeit_min`), orange ⚡ = mains dropout, grey ▲ = tent sensor unreachable (tent temperature and humidity empty). Short gaps (under 16 min) are bridged in the lines, longer ones break them; sensor dropouts are also counted in the events card and the daily summary.
- **Key figures** for the visible window: ⌀/min/max per curve, split by lights on/off, share of VPD in the target range with lights on, light sum, number of restarts/mains dropouts.
- **Daily summary:** per day DLI (mol/m²·d from PPFD), light hours, tent temperature and humidity day/night, VPD with lights on and share on target, number of watering cycles, restarts. Clicking a row jumps to that day.
- **Nozzle analysis:** every watering cycle as a dot, a rolling median over 9 cycles per reservoir, outliers (>15 % from the median) circled in red. Shown as pulses per second (stays comparable when the open time was changed) or pulses per cycle. One card per reservoir with median, spread, trend (last vs. first third of the cycles in the window), outliers, deviation from the average of all reservoirs and a traffic light: 🟢 stable · 🟠 trend ≤ −10 % (nozzle/filter clogging) or ≥ +10 % (leak, loose nozzle) · 🟡 spread > 15 % (air, dirt) · ⚪ fewer than 6 cycles.

The current day is fetched fresh on every "Reload", older days only once per page load.

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

> **No light-SSR mode any more:** the former second node type `NODE_TYPE_LIGHT_SSR (0x05)` has been removed from the firmware — lighting is handled by the RS485 node [Lichtx4](#lichtx4-1). A socket node always announces itself as `NODE_TYPE_SOCKET (0x03)`, even if an old type is still stored in NVS.

#### Relay Command

```c
// Turn all 4 relays on:
MSG_COMMAND + CMD_SET_ONOFF (0x02) + payload[1] = 0x0F

// Only R1 + R3:
payload[1] = 0b00000101  // = 0x05
```

### First Boot & Configuration

#### Sockets — Node ID

On the very first boot (NVS namespace `"nodecfg"` empty), the serial monitor shows:

```
=== ERSTSTART: Node-Konfiguration ===
Node-ID eingeben (1-254) + Enter (60s Timeout, sonst 3):
> 10
[NODE] Gespeichert: Steckdose  ID=0x0A
```

Stored in NVS `"nodecfg"`: key `id` (uint8). Clear with `NODECLEAR` → dialog reappears on next boot.

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

If no input is given within 60 seconds at any setup dialog (node ID, channel), a default value is used: ID=3 / channel 6.

---

### Serial Commands

Baud rate **115200** · line ending **LF** or **CR+LF**

#### Sockets

| Command | Effect |
|---|---|
| `STATUS` | Current relay bitmask and individual status R1–R4 |
| `MASK <0–15>` | Set all relays via decimal value (e.g. `MASK 5` → R1+R3 on) |
| `R<1–4> <0\|1>` | Switch a single relay (e.g. `R2 1`) |
| `NODEINFO` | Show node ID and pairing status |
| `NODECLEAR` | Clear node ID → first-boot dialog on next restart |
| `UNPAIR` | Release pairing with the master (same as BOOT button 3 s) |
| `WIFIINFO` | Show stored SSID and channel |
| `WIFICLEAR` | Clear WiFi configuration → channel scan on next restart |

#### Multisensor

| Command | Effect |
|---|---|
| `WIFIINFO` | Show stored SSID and channel |
| `WIFICLEAR` | Clear WiFi configuration → channel scan on next restart |
