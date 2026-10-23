#ifndef PINOUT_H
#define PINOUT_H

// ============================================================================
// Pinbelegung SMD-Board (einzige unterstuetzte Hardware-Version ab jetzt —
// die fruehere BOARD_VERSION-Unterscheidung 1/2 entfaellt, siehe
// docs/SmdVersion_Pinbelegung.pdf).
//   - kein PCF8574 mehr: alle Ventile/Pumpe/Umwälzpumpe sind direkte GPIO
//   - nur noch EIN I2C-Bus (RTC + AHT21 + optional GP8403)
//   - GPIO38 ueberwacht die Netzspannung (HIGH = Netz vorhanden, LOW = USV-Betrieb)
// ============================================================================

// ========== DS18B20 Temperatursensoren (1-Wire) ==========
// Rollen (Vorrat/Pflanze/Innentemperatur) werden ueber die eindeutige ROM-Adresse
// jedes Sensors zugeordnet (siehe ds18b20cfg.h/.cpp), nicht ueber die Bus-Scan-
// Reihenfolge, die sich bei vertauschten Kabeln aendern kann.
#define DS18B20_PIN       4

// ========== Drucksensor (analog 0.5–4.5V) ==========
#define PRESSURE_PIN      5

// ========== Gehaeuseluefter (PWM, temperaturgeregelt ueber Innen-DS18B20) ==========
#define GEHAEUSE_FAN_PIN  6

// ========== Magnetventile (direkte GPIO, kein PCF8574 mehr) ==========
#define VENTIL1_PIN        8   // Behälter 1
#define VENTIL2_PIN        7   // Behälter 2
#define VENTIL3_PIN        10   // Behälter 3
#define VENTIL4_PIN       9   // Rücklaufventil

// Treiber-Polaritaet aller direkten Ausgaenge (Ventile 1-4, Pumpe, Umwälzpumpe):
// 0 = High-aktiv (GPIO HIGH -> Ausgang AN, Annahme/Default), 1 = Low-aktiv
// (invertierte MOSFET-Treiber) — an der echten Hardware ggf. hier umstellen.
#define OUTPUT_ACTIVE_LOW  0

// ========== SPI Micro SD und W5500 ==========
#define SPI_MOSI    11
#define SPI_SCK     12
#define SPI_MISO    13
#define SPI_CS_W5500   14

// ========== Tank-Füllstandssensor — Ultraschall ODER Drucksensor, per Konfiguration ==========
// Beide Sensortypen nutzen dieselben zwei GPIOs (nie gleichzeitig aktiv, siehe
// tankSensorConfig.sensor_type in tank.h) — keine Platinenänderung beim Wechsel nötig.
//   Ultraschall (HC-SR04 / RCWL-1670 / JSN-SR04T): TRIG/ECHO
//   Drucksensor (HX710B-basierte 0-40kPa-Module):  SCK/DOUT (2-Draht, kein I2C)
#define ULTRASONIC_TRIG  16   // → TRIG
#define ULTRASONIC_ECHO  15   // ← ECHO  (HC-SR04 an 3.3V betreiben → ECHO direkt anschließbar)
#define HX710_SCK        ULTRASONIC_TRIG   // → PD_SCK (HX710B ebenfalls an 3.3V betreiben)
#define HX710_DOUT       ULTRASONIC_ECHO   // ← DOUT

// ========== I2C Bus (RTC + AHT21 + optional GP8403) ==========
#define I2C_SDA          18
#define I2C_SCL          17

// ========== GP8403 (I2C-DAC, 2x 0-10V) — optionaler Analogausgang ==========
// Alternative zum RS485-Analogmodul. A0-A2 auf GND -> Adresse 0x58. Am I2C-Bus (Wire).
// Name an Multimodul angeglichen (dort I2C_ADDR_GP8403, gleiches Register-Layout).
#define I2C_ADDR_GP8403  0x58

// ========== RS485 (MAX13487 — Auto-Direction) ==========
// Namen an Multimodul angeglichen (dort RS485_TX/RS485_RX aus MCU-Sicht) — vorher hier nach
// Transceiver-Pinnamen RO/DI benannt, funktional identisch: RS485_RX = MCU-RX = Transceiver RO,
// RS485_TX = MCU-TX = Transceiver DI.
#define RS485_TX          2   // → an RXD vom Modul
#define RS485_RX          1   // ← an TXD vom Modul

// ========== Durchflussmesser (YF-S401-aehnlich, Hall-Impulsgeber) ==========
// GPIO3 statt GPIO46: beides Strapping-Pins, aber GPIO3s Strapping-Rolle (JTAG-Signalquelle)
// greift nur, wenn ein bestimmtes eFuse-Bit gesetzt ist (Standard: nein) und der Pin ist beim
// Reset floating. GPIO46 wirkt dagegen zusammen mit GPIO0 an der Boot-Modus-Auswahl -- ein
// Sensor, der den Pin aktiv treibt, koennte das beim Reset verfaelschen.
#define FLOW_SENSOR_PIN   3

#define WSINT       21

// ========== Netzspannungsueberwachung ==========
// HIGH = Netzspannung vorhanden, LOW = Betrieb an der USV (Netzausfall).
#define NETZ_OK_PIN       38

#define SPI_CS_SD      39

// ========== Zusatzausgaenge (direkte GPIO, kein PCF8574 mehr) ==========
#define UMWAELZPUMPE_GPIO 47   // Vorratsbehälter-Zirkulation, automatischer Lauf-/Pausenzyklus
                                // (pausiert waehrend ein Behälter aktiv wässert, siehe valves.cpp)
#define PUMPE_GPIO        48   // automatisch an, sobald ein Behälter aktiv wässert

// ========== Gesperrte GPIO (N16R8) ==========
// 0,3,45,46 = Strapping   19,20 = USB   26-32 = Flash
// 33-37 = OPI-PSRAM       40-42 = JTAG  43,44 = UART0

#endif
