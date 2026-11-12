#include "storage.h"
#include "pinout.h"
#include "dbg.h"
#include <SD.h>
#include <SPI.h>

static bool sdOk = false;

bool storage_init(SPIClass &spi) {
    if (!SD.begin(SPI_CS_SD, spi)) {
        dbgPrintln("[SD] Nicht gefunden");
        sdOk = false;
        return false;
    }
    uint64_t mb = (uint64_t)SD.cardSize() / (1024ULL * 1024ULL);
    dbgPrintf("[SD] OK — %llu MB (Typ %d)\n", mb, (int)SD.cardType());
    sdOk = true;
    return true;
}

bool storage_is_ok() { return sdOk; }

void storage_log(time_t ts, const LogEntry &e) {
    if (!sdOk) return;
    // ts kommt aus rtc.now().unixtime() und ist bereits Lokalzeit (die RTC laeuft auf Ortszeit,
    // siehe NTP-Abgleich in main.cpp) -- localtime_r() wuerde die Zeitzone ein zweites Mal
    // aufschlagen und den Tageswechsel um 1-2 h vorverlegen.
    struct tm t;
    gmtime_r(&ts, &t);
    char filename[24];
    snprintf(filename, sizeof(filename), "/%04d-%02d-%02d.csv",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    bool isNew = !SD.exists(filename);

    if (!isNew) {
        // Bereits vorhandene Tagesdatei kann noch das alte, kuerzere Spaltenformat haben
        // (vor Erweiterung um Raum/Zelt/Luefter/CO2/Luftbefeuchtung/Duesen-Durchfluss). In dem
        // Fall beiseitelegen statt Zeilen mit unterschiedlicher Spaltenzahl zu mischen. Pruefung
        // auf ein Merkmal des neuesten Formats (aktuell die VPD-Regelungsspalten) faengt
        // automatisch auch alle aelteren Formate.
        File check = SD.open(filename, FILE_READ);
        if (check) {
            String firstLine = check.readStringUntil('\n');
            check.close();
            if (firstLine.indexOf("VPD_Soll_max") < 0) {
                // Mehrere Formatwechsel am selben Tag (mehrere Updates): nie ein vorhandenes
                // _alt ueberschreiben, sondern _alt2, _alt3 ... verwenden (sonst Datenverlust).
                char oldName[32];
                snprintf(oldName, sizeof(oldName), "/%04d-%02d-%02d_alt.csv",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
                for (int n = 2; SD.exists(oldName) && n < 100; n++)
                    snprintf(oldName, sizeof(oldName), "/%04d-%02d-%02d_alt%d.csv",
                             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, n);
                SD.rename(filename, oldName);
                isNew = true;
            }
        }
    }

    File f = SD.open(filename, FILE_APPEND);
    if (!f) {
        dbgPrintf("[SD] Öffnen fehlgeschlagen: %s\n", filename);
        return;
    }
    if (isNew) {
        f.println("Zeit,TempVorrat_C,TempPflanze_C,Druck_bar,Fuellstand_%,Volumen_L,"
                   "RaumTemp_C,RaumFeuchte_%,ZeltTemp_C,ZeltFeuchte_%,ZeltCO2_ppm,ZeltVPD_kPa,ZeltPPFD_umol,"
                   "LichtMaske,LuefterLeistung_%,LuefterRPM,CO2Ausgang,HumidAusgang,"
                   "ImpulseBehaelter1,DauerBehaelter1_s,ImpulseBehaelter2,DauerBehaelter2_s,"
                   "ImpulseBehaelter3,DauerBehaelter3_s,ImpulseBehaelter4,DauerBehaelter4_s,Laufzeit_min,NeustartGrund,Lichtx4Starts,Lichtx4Grund,Netzaussetzer,NetzaussetzerMax_ms,"
                   "CO2Steckdose,HumidSteckdose,WLAN_dBm,WLAN_Abbrueche,EspNow_online,HeapMin_kB,"
                   "MS2_Abfragen,MS2_Timeouts,VPD_Blatt_kPa,VPD_Soll_min,VPD_Soll_max");
    }

    char zeit[9];
    snprintf(zeit, sizeof(zeit), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    String line = String(zeit) + "," +
                  String(e.tempVorrat, 1) + "," +
                  String(e.tempPflanze, 1) + "," +
                  String(e.druckBar, 2) + "," +
                  String(e.fuellstandProzent) + "," +
                  String(e.volumenLiter, 1) + "," +
                  (e.raumOk ? String(e.raumTemp, 1) : String("")) + "," +
                  (e.raumOk ? String(e.raumFeuchte, 1) : String("")) + "," +
                  (e.zeltOk ? String(e.zeltTemp, 1) : String("")) + "," +
                  (e.zeltOk ? String(e.zeltFeuchte, 1) : String("")) + "," +
                  (e.zeltOk && e.zeltCo2Ok ? String(e.zeltCo2) : String("")) + "," +
                  (e.zeltOk ? String(e.zeltVpd, 2) : String("")) + "," +
                  (e.ppfdOk ? String(e.zeltPpfd, 1) : String("")) + "," +
                  String(e.lichtMaske) + "," +
                  (e.fanOk ? String(e.fanPercent) : String("")) + "," +
                  (e.fanOk ? String(e.fanRpm) : String("")) + "," +
                  String(e.co2Ausgang ? 1 : 0) + "," +
                  String(e.humidAusgang ? 1 : 0);
    for (uint8_t i = 0; i < BEHAELTER_ANZAHL; i++) {
        line += "," + (e.flowValid[i] ? String(e.flowImpulse[i]) : String("")) +
                "," + (e.flowValid[i] ? String(e.flowDauerS[i], 1) : String(""));
    }
    line += "," + String(e.laufzeitMin) + "," + String(e.neustartGrund ? e.neustartGrund : "");
    line += "," + (e.lichtDiagOk ? String(e.lichtStarts) : String("")) +
            "," + (e.lichtDiagOk && e.lichtGrund ? String(e.lichtGrund) : String(""));
    line += "," + String(e.netzAussetzer) + "," + String(e.netzAussetzerMaxMs);
    line += "," + (e.co2Steckdose   >= 0 ? String(e.co2Steckdose)   : String("")) +
            "," + (e.humidSteckdose >= 0 ? String(e.humidSteckdose) : String(""));
    line += "," + (e.wlanOk ? String(e.wlanDbm) : String("")) + "," + String(e.wlanAbbrueche);
    line += "," + String(e.espnowOnline) + "/" + String(e.espnowGekoppelt) + "," + String(e.heapMinKb);
    line += "," + String(e.ms2Abfragen) + "," + String(e.ms2Timeouts);
    line += "," + (e.vpdOk ? String(e.vpdBlatt, 2)   : String("")) +
            "," + (e.vpdOk ? String(e.vpdSollMin, 2) : String("")) +
            "," + (e.vpdOk ? String(e.vpdSollMax, 2) : String(""));

    f.println(line);
    f.close();
}

void storage_list_logs(void (*cb)(const char *name, size_t size, void *ctx), void *ctx) {
    if (!sdOk) return;
    File root = SD.open("/");
    if (!root) return;
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            if (name.startsWith("/")) name = name.substring(1);
            // .txt: Debug-Aufzeichnungen (siehe dbg.h) landen im selben Ordner und werden
            // hier mit aufgelistet, damit dieselbe Download-UI genutzt werden kann.
            if (name.endsWith(".csv") || name.endsWith(".txt")) cb(name.c_str(), file.size(), ctx);
        }
        file = root.openNextFile();
    }
    root.close();
}

File storage_open_log(const char *name) {
    if (!sdOk) return File();
    String n = name;
    if (n.startsWith("/")) n = n.substring(1);
    String path = "/" + n;
    return SD.open(path, FILE_READ);
}

bool storage_delete_log(const char *name) {
    if (!sdOk) return false;
    String n = name;
    if (n.startsWith("/")) n = n.substring(1);
    String path = "/" + n;
    if (!SD.exists(path)) return false;
    return SD.remove(path);
}

// Sucht und loescht jeweils eine passende Datei pro Durchlauf, statt waehrend des Iterierens ueber
// openNextFile() zu loeschen -- manche SD-Implementierungen verhalten sich dabei undefiniert.
uint32_t storage_delete_all_logs() {
    if (!sdOk) return 0;
    uint32_t count = 0;
    for (;;) {
        File root = SD.open("/");
        if (!root) break;
        String found = "";
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                String name = file.name();
                if (name.startsWith("/")) name = name.substring(1);
                if (name.endsWith(".csv") || name.endsWith(".txt")) { found = name; break; }
            }
            file = root.openNextFile();
        }
        root.close();
        if (found.isEmpty()) break;
        if (SD.remove("/" + found)) count++;
        else break;  // Abbruch statt Endlosschleife, falls eine Datei nicht geloescht werden kann
    }
    return count;
}
