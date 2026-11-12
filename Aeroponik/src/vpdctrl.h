#ifndef VPDCTRL_H
#define VPDCTRL_H

#include <stdint.h>
#include <stdbool.h>

// VPD-Regelung (seit FW 1.21.0): fuehrt Befeuchter und Abluftluefter gemeinsam auf einen
// VPD-Zielbereich, der sich aus der Wachstumsphase und Licht an/aus ergibt. Geregelt wird der
// Blatt-VPD (Blatttemperatur = Lufttemperatur + Offset, nur bei Licht an -- nachts sind Blatt und
// Luft praktisch gleich warm). Befeuchter/Luefter folgen den Vorgaben nur, wenn use_humid bzw.
// use_fan gesetzt ist; ihre eigenen Einstellungen (Steckdose, Ausgang, Mindest-/Hoechstdrehzahl)
// bleiben gueltig, ihre Min/Max-Regelung wird dann uebergangen.

#define VPD_PHASE_STECKLING  0
#define VPD_PHASE_WACHSTUM   1
#define VPD_PHASE_BLUETE_1   2
#define VPD_PHASE_BLUETE_2   3
#define VPD_PHASE_EIGENE     4
#define VPD_PHASE_COUNT      5

struct VpdConfig {
    bool    enabled;
    uint8_t phase;          // VPD_PHASE_*
    uint8_t day_min;        // kPa*100, nur fuer VPD_PHASE_EIGENE (sonst Tabelle in vpdctrl.cpp)
    uint8_t day_max;
    uint8_t night_min;
    uint8_t night_max;
    int8_t  leaf_offset;    // Zehntel °C, Blatt = Luft + Offset (bei Licht an), z.B. -15 = -1,5 °C
    bool    use_humid;      // Befeuchter folgt der VPD-Regelung
    bool    use_fan;        // Abluftluefter folgt der VPD-Regelung
    uint8_t temp_max;       // °C -- darueber Luefter hoch, egal was der VPD sagt
    uint8_t hum_max;        // %rH -- darueber Befeuchter aus + Luefter hoch (Schimmelschutz)
    uint8_t alarm_min;      // Minuten ausserhalb des Zielbereichs bis zur Benachrichtigung, 0 = aus
    uint8_t temp_min;       // °C -- darunter wird die Abluft fuer den VPD nicht weiter hochgefahren
                            // (kuehlt sonst nachts aus), Feuchtegrenze hat weiter Vorrang
};

// Gruende / Zustaende fuer die Statusanzeige (Bitmaske)
#define VPD_R_SENSOR      0x0001   // keine gueltigen Zelt-Klimawerte
#define VPD_R_IN_BAND     0x0002
#define VPD_R_TOO_HUMID   0x0004   // VPD unter Soll
#define VPD_R_TOO_DRY     0x0008   // VPD ueber Soll
#define VPD_R_TEMP_LIMIT  0x0010   // Temperaturgrenze erzwingt Luefter
#define VPD_R_HUM_LIMIT   0x0020   // Feuchtegrenze erzwingt Luefter / Befeuchter aus
#define VPD_R_ROOM_WET    0x0040   // Raumluft nicht trockener als Zeltluft -> Abluft trocknet nicht
#define VPD_R_FAN_MAX     0x0080   // Luefter am Anschlag, trotzdem zu feucht
#define VPD_R_CO2         0x0100   // CO2 wird dosiert -> Luefter wird nicht hochgefahren
#define VPD_R_RAMP        0x0200   // Zielbereich wandert gerade (Licht an/aus)
#define VPD_R_COLD        0x0400   // Zelt an/unter temp_min -> Abluft wird nicht hochgefahren

struct VpdInputs {
    bool    klimaOk;
    float   tempC, humPct;
    bool    roomOk;
    float   roomTempC, roomHumPct;
    bool    lightOn;
    bool    co2Dosing;
    uint8_t fanMin, fanMax;
    int     fanNow;         // aktuelle Luefter-Vorgabe (Startwert der Regelung), -1 = unbekannt
};

struct VpdState {
    bool     valid;         // Regelung hat gueltige Werte berechnet
    float    vpdAir;        // kPa (Luft)
    float    vpdLeaf;       // kPa (Blatt) -- Regelgroesse
    float    sollMin, sollMax;   // aktueller (ggf. gerade wandernder) Zielbereich
    bool     humidWant;     // Befeuchter-Ausgang (getaktet, siehe humidDemand)
    bool     humidDemand;   // Befeuchter-Bedarf laut Regelung (ungetaktet)
    int      fanPct;        // Luefter-Wunsch in %, -1 = keine Vorgabe (Luefter haelt seinen Wert)
    uint16_t reasons;       // VPD_R_*
    bool     outOfBand;
    float    roomAbsHum, tentAbsHum;   // g/m³ (nur Anzeige)
};

extern VpdConfig vpdConfig;
extern VpdState  vpdState;

void  loadVpdConfig();
void  saveVpdConfig();
// Zielbereich (kPa) der Phase fuer Tag/Nacht -- bei VPD_PHASE_EIGENE aus der Konfiguration
void  vpdPhaseBand(uint8_t phase, bool day, float& lo, float& hi);
// Alle ~5 s aufrufen (main.cpp, loopVpdCtrl)
void  vpdUpdate(const VpdInputs& in);
void  vpdReset();   // nach Konfigurationsaenderung: Rampe/Integrator neu aufsetzen

#endif
