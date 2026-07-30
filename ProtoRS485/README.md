# ProtoRS485

**[Deutsch](#deutsch) | [English](#english)**

---

## Deutsch

Generischer RS485-Modbus-RTU-Slave-Simulator auf einem ESP32-C3, mit Live-Anzeige der
empfangenen Registerwerte auf einem WS2812-LED-Streifen. Gedacht, um gegen eine
Master-Firmware zu testen, bevor die echte Zielhardware verfügbar ist, oder um ein
RS485-Protokoll ohne das reale Gerät zu debuggen.

Aktuell simuliert es konkret das ATO-2-Kanal-RS485-zu-0-10V-Analogausgangsmodul (Adresse
`0x50`) aus dem [`Aeroponik`](../Aeroponik)-Projekt: Kanal 1 (Lüfter) und Kanal 2 (Licht)
werden je als Helligkeit einer LED-Adresse angezeigt.

### Hardware

- ESP32-C3 (getestet: "nologo_esp32c3_super_mini")
- RS485-Transceiver-Modul mit Auto-Direction (z. B. MAX13487), an UART1
- WS2812/WS2812B/NeoPixel-kompatibler LED-Streifen

Pinbelegung siehe [`src/pinout.h`](src/pinout.h).

**Hinweis bei 12V-LED-Streifen:** manche 12V-Streifen steuern 3 physische LEDs in Serie
über eine gemeinsame Adresse an (statt 1 LED pro Adresse wie bei üblichen 5V-Streifen) —
`LED_COUNT` ist dann kleiner als die Anzahl sichtbarer LEDs. Siehe Kommentar in
`pinout.h`.

### Für eigene Anwendungen anpassen

Um ein anderes Modbus-RTU-Slave-Gerät zu simulieren, statt des ATO-Analogmoduls:

| Datei | Anzupassen |
|---|---|
| `src/rs485_slave.h` | `RS485_SLAVE_ADDR` (Modbus-Adresse), `RS485_REG_*`/`RS485_NUM_REGS` (Register-Layout) |
| `src/pinout.h` | `RS485_TX`/`RS485_RX` (UART-Pins), `LED_PIN`/`LED_COUNT` |
| `src/main.cpp` | `onWrite()`/`redraw()` — wie ankommende Registerwerte visualisiert werden |
| `src/rs485_slave.cpp` | generischer Modbus-RTU-Slave-Kern (CRC16, FC03 Read Holding Registers, FC06 Write Single Register) — i. d. R. unverändert wiederverwendbar |

### Bauen/Flashen

```
pio run -t upload
```

(VSCode + [pioarduino](https://github.com/pioarduino)-Extension, siehe Haupt-README des
Workspace.)

---

## English

Generic RS485 Modbus RTU slave simulator on an ESP32-C3, with a live display of received
register values on a WS2812 LED strip. Meant for testing against a master firmware
before the real target hardware is available, or for debugging an RS485 protocol without
the actual device.

It currently simulates the ATO 2-channel RS485-to-0-10V analog output module (address
`0x50`) from the [`Aeroponik`](../Aeroponik) project: channel 1 (fan) and channel 2
(light) are each shown as the brightness of one LED address.

### Hardware

- ESP32-C3 (tested: "nologo_esp32c3_super_mini")
- RS485 transceiver module with auto-direction (e.g. MAX13487), on UART1
- WS2812/WS2812B/NeoPixel-compatible LED strip

Pin assignments in [`src/pinout.h`](src/pinout.h).

**Note on 12V LED strips:** some 12V strips drive 3 physical LEDs in series per
addressable position (instead of 1 LED per address like typical 5V strips) — `LED_COUNT`
is then smaller than the number of visible LEDs. See the comment in `pinout.h`.

### Adapting for your own use

To simulate a different Modbus RTU slave device instead of the ATO analog module:

| File | What to change |
|---|---|
| `src/rs485_slave.h` | `RS485_SLAVE_ADDR` (Modbus address), `RS485_REG_*`/`RS485_NUM_REGS` (register layout) |
| `src/pinout.h` | `RS485_TX`/`RS485_RX` (UART pins), `LED_PIN`/`LED_COUNT` |
| `src/main.cpp` | `onWrite()`/`redraw()` — how incoming register values get visualized |
| `src/rs485_slave.cpp` | generic Modbus RTU slave core (CRC16, FC03 Read Holding Registers, FC06 Write Single Register) — usually reusable as-is |

### Build/flash

```
pio run -t upload
```

(VSCode + [pioarduino](https://github.com/pioarduino) extension, see the workspace's main
README.)
