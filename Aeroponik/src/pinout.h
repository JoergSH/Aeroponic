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
#define VENTIL1_PIN        7   // Behälter 1
#define VENTIL2_PIN        8   // Behälter 2
#define VENTIL3_PIN        9   // Behälter 3
#define VENTIL4_PIN       10   // Rücklaufventil

// Treiber-Polaritaet aller direkten Ausgaenge (Ventile 1-4, Pumpe, Umwälzpumpe):
// 0 = High-aktiv (GPIO HIGH -> Ausgang AN, Annahme/Default), 1 = Low-aktiv
// (invertierte MOSFET-Treiber) — an der echten Hardware ggf. hier umstellen.
#define OUTPUT_ACTIVE_LOW  0

// ========== SPI Micro SD und W5500 ==========
#define SPI_MOSI    11
#define SPI_SCK     12
#define SPI_MISO    13
#define SPI_CS_W5500   14

// ========== Ultraschall Füllstandssensor (HC-SR04 / RCWL-1670) ==========
#define ULTRASONIC_TRIG  15   // → TRIG
#define ULTRASONIC_ECHO  16   // ← ECHO  (HC-SR04 an 3.3V betreiben → ECHO direkt anschließbar)

// ========== I2C Bus (RTC + AHT21 + optional GP8403) ==========
#define I2C_SDA          18
#define I2C_SCL          17

// ========== GP8403 (I2C-DAC, 2x 0-10V) — optionaler Analogausgang ==========
// Alternative zum RS485-Analogmodul. A0-A2 auf GND -> Adresse 0x58. Am I2C-Bus (Wire).
#define GP8403_ADDR      0x58

// ========== RS485 (MAX13487 — Auto-Direction) ==========
#define RS485_RO          1   // ← Receive Output  (UART1 RX) — Modul "RX"
#define RS485_DI          2   // → Data Input       (UART1 TX) — Modul "TX"

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
