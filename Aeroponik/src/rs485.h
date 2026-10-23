#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stdbool.h>

// Adress-Raum RS485 Bus
// 0x01-0x0E  Mars Hydro Original (MH-AE=2, iHub4=5, iHub10=13, ...)
// 0x20-0x3F  Eigene Sensoren
// 0x40-0x5F  Eigene Aktoren (reserviert)

#define RS485_ADDR_MS2      0x20   // Multisensor2
#define RS485_MS2_REGS      14     // CO2, Temp, Humi, PPFD, 8×Kanal, Gain, Status

#define RS485_ADDR_LICHT    0x40   // Lichtx4 (4× Relais)
#define RS485_REG_MASK      0      // Relaismaske, Bit0-3 = Relais 1-4

#define RS485_ADDR_FAN       6     // MARS Hydro iControl Abluftluefter (fremdes Geraet, feste Adresse)
// Der Luefter-Controller selbst fragt laut verifizierter Original-Spec
// (CLAUDE_CODE_SPEC_RS485_MARS_FAN.md) IMMER Start=0, Count=17 ab (volles Register-Set) --
// andere Lesebereiche wurden nicht getestet/erwartet. RPM/Prozent liegen innerhalb dieses
// 17-Register-Blocks an fester Position.
#define RS485_FAN_REG_RPM    11    // 0-basiert, Ist-Drehzahl
#define RS485_FAN_REG_ENABLE 15    // 0-basiert, Enable/An-Bit -- muss vor einer Leistungsaenderung
                                    // auf 1 gesetzt werden, sonst wird der Leistungswert zwar im
                                    // Register uebernommen, aber nicht physisch ausgefuehrt
                                    // (siehe MARS_Hydro_RE_Notizen.md Abschnitt 5b, Fund 7)
#define RS485_FAN_REG_PERCENT 16   // 0-basiert, Leistung 0-100% (auch Schreibregister FC06)
#define RS485_FAN_READ_COUNT 17    // immer das volle Set ab Register 0 lesen

#define RS485_ADDR_ANALOG     0x50 // 2-Kanal RS485->0-10V Modul (ATO), einmalig per
                                    // Herstellersoftware auf diese Adresse + 9600 Baud
                                    // eingestellt (Werkseinstellung 0x01 / 4800 Baud)
#define RS485_REG_ANALOG_CH1  0    // Kanal 1 (Luefter), Wertebereich 0-4095 (12-bit)
#define RS485_REG_ANALOG_CH2  1    // Kanal 2 (Licht),   Wertebereich 0-4095 (12-bit)

#define RS485_ADDR_LUEFT      0x51 // Lueftermodul (RP2040-Zero, 4x PWM-Zeltluefter + Tacho)
#define RS485_LUEFT_COUNT     4
#define RS485_LUEFT_REG_PWM_BASE 0 // Reg 0-3, FC06 schreiben: PWM-Sollwert 0-100%
#define RS485_LUEFT_REG_RPM_BASE 4 // Reg 4-7, FC03 lesen: Ist-Drehzahl U/min
#define RS485_LUEFT_REG_BOOTID  8 // Reg 8, FC03 lesen: zufaellige Boot-ID -- aendert sie sich
                                  // gegenueber dem letzten Poll, hat der Slave neu gestartet und
                                  // seine PWM-Sollwerte verloren (siehe parse_lueft_read() in
                                  // rs485.cpp), auch wenn der Reset zu kurz war um als
                                  // Bus-Unterbrechung aufzufallen.

struct Rs485SensorData {
    bool     online;
    uint32_t last_update_ms;
    uint16_t co2_ppm;
    float    temp_c;
    float    hum_pct;
    float    ppfd;
    uint8_t  channels[8];
    uint8_t  gain;
    uint8_t  status;
};

struct Rs485LichtData {
    bool     online;
    uint8_t  mask;
    uint32_t last_update_ms;
};

struct Rs485FanData {
    bool     online;
    uint16_t rpm;
    uint8_t  percent;
    uint32_t last_update_ms;
};

struct Rs485AnalogData {
    bool     online;
    uint16_t ch1_raw;   // 0-4095, zuletzt am Geraet bestaetigter Wert Kanal 1 (Luefter)
    uint16_t ch2_raw;   // 0-4095, Kanal 2 (Licht)
    uint32_t last_update_ms;
};

struct Rs485LueftData {
    bool     online;
    uint16_t rpm[RS485_LUEFT_COUNT];       // zuletzt gelesene Ist-Drehzahl je Kanal
    uint8_t  percent[RS485_LUEFT_COUNT];   // zuletzt bestaetigter PWM-Sollwert je Kanal
    uint32_t last_update_ms;
};

void setupRS485();
void loopRS485();

const Rs485SensorData& rs485_get_ms2();

// Setzt die Relaismaske am Lichtx4 (nicht-blockierend, wird beim naechsten
// freien Bus-Slot per Modbus FC06 gesendet).
bool rs485_set_licht_mask(uint8_t mask);
const Rs485LichtData& rs485_get_licht();

// Setzt die Luefterleistung (0-100%) am MARS-Hydro-Luefter (Adresse 6, FC06
// Register 0x10). Nicht-blockierend, siehe rs485_set_licht_mask().
bool rs485_set_fan_percent(uint8_t percent);
const Rs485FanData& rs485_get_fan();

// Setzt Kanal 1/2 (0-4095) am Analog-Ausgangsmodul. Nicht-blockierend, siehe
// rs485_set_licht_mask().
bool rs485_set_analog_ch1(uint16_t value);
bool rs485_set_analog_ch2(uint16_t value);
const Rs485AnalogData& rs485_get_analog();

// Setzt den PWM-Sollwert (0-100%) eines Zeltluefter-Kanals (0-3) am Lueftermodul.
// Nicht-blockierend, siehe rs485_set_licht_mask().
bool rs485_set_lueft_percent(uint8_t channel, uint8_t percent);
const Rs485LueftData& rs485_get_lueft();

// Schaltet das Pollen einzelner RS485-Geraete ab/an (Standard: alle an). Wird von
// main.cpp anhand der Geraete-Auswahl (outputctrl.h) gesetzt, damit nicht angeschlossene
// Hardware nicht sinnlos abgefragt wird. Ausstehende Schreibbefehle sind davon unabhaengig.
void rs485_set_licht_polling(bool enabled);
void rs485_set_fan_polling(bool enabled);
void rs485_set_analog_polling(bool enabled);
void rs485_set_lueft_polling(bool enabled);

// Diagnose: probiert eine einzelne Adresse direkt (FC03, blockierend, kurzer Timeout) --
// umgeht die normale Poll-Zustandsmaschine bewusst, fuer Adress-/Baudraten-Scans bei
// Verdacht auf eine verschobene/zurueckgesetzte Geraeteadresse (siehe handleApiRs485ScanFan()
// in main.cpp). Rueckgabe: Anzahl empfangener Bytes (0 = keine Antwort).
uint8_t rs485_probe_fc03(uint8_t addr, uint16_t start, uint16_t count, uint8_t* out_buf, uint8_t max_len);

#endif
