#ifndef PINOUT_H
#define PINOUT_H

// ========== DS18B20 Temperatursensoren ==========
#define DS18B20_PIN       4

// ========== Drucksensor (analog 0.5–4.5V) ==========
#define PRESSURE_PIN      5

// ========== Ultraschall Füllstandssensor (HC-SR04 / RCWL-1670) ==========
#define ULTRASONIC_TRIG  15   // → TRIG
#define ULTRASONIC_ECHO  16   // ← ECHO  (HC-SR04 an 3.3V betreiben → ECHO direkt anschließbar)



// ========== I2C Bus (DS1307 RTC + PCF8574AP) ==========
#define I2C_SDA          18
#define I2C_SCL          17

// ========== PCF8574AP (Magnetventile via I2C) ==========
// Adresse: alle A-Pins auf GND → 0x38
#define PCF8574_ADDR     0x38

// Bit-Positionen im PCF8574AP (1 = Ventil AN, 0 = Ventil AUS)
#define MV_BEHAELTER_1   0   // P0
#define MV_BEHAELTER_2   1   // P1
#define MV_BEHAELTER_3   2   // P2
#define MV_RUECKLAUF     3   // P3
// P4 = MV5 Reserve, P5 = MV6 Reserve

// ========== I2C Bus 2 (AHT21B Temp/Feuchte) ==========
// Nutzt die urspruenglich als TDS_PPM/FreeAnalogIn vorgesehenen Pins (Platine
// bereits gefertigt, beide waren noch unbeschaltet). GPIO46 hat keine ADC-Funktion
// (nur GPIO1-10/11-20 sind ADC-faehig) und war daher ohnehin nie als Analogeingang nutzbar.
// GPIO46 ist Strapping-Pin (Bootmodus zusammen mit GPIO0) — nur beim Hochladen ueber
// USB relevant (Auto-Reset-Sequenz), im normalen Betrieb ohne Einfluss.
#define I2C2_SDA           3
#define I2C2_SCL          46


// ========== RS485 (MAX13487 — Auto-Direction) ==========
#define RS485_RO          1   // ← Receive Output  (UART1 RX) — Modul "RX"
#define RS485_DI          2   // → Data Input       (UART1 TX) — Modul "TX"

// ========== SPI Micro SD und W5500 ==========
#define SPI_MOSI    11
#define SPI_SCK     12
#define SPI_MISO    13
#define SPI_CS_W5500   14
#define SPI_CS_SD      39

#define WSINT       21
#define WSRST       10







// ========== Gesperrte GPIO (N16R8) ==========
// 0,3,45,46 = Strapping   19,20 = USB   26-32 = Flash
// 33-37 = OPI-PSRAM       38-42 = JTAG  43,44 = UART0

#endif
