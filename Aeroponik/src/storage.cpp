#include "storage.h"
#include "pinout.h"
#include <SD.h>
#include <SPI.h>

static bool sdOk = false;

bool storage_init(SPIClass &spi) {
    if (!SD.begin(SPI_CS_SD, spi)) {
        Serial.println("[SD] Nicht gefunden");
        sdOk = false;
        return false;
    }
    uint64_t mb = (uint64_t)SD.cardSize() / (1024ULL * 1024ULL);
    Serial.printf("[SD] OK — %llu MB (Typ %d)\n", mb, (int)SD.cardType());
    sdOk = true;
    return true;
}

bool storage_is_ok() { return sdOk; }

void storage_log(time_t ts, const LogEntry &e) {
    if (!sdOk) return;
    struct tm t;
    localtime_r(&ts, &t);
    char filename[24];
    snprintf(filename, sizeof(filename), "/%04d-%02d-%02d.csv",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    bool isNew = !SD.exists(filename);

    if (!isNew) {
        // Bereits vorhandene Tagesdatei kann noch das alte, kuerzere Spaltenformat haben
        // (vor Erweiterung um Raum/Zelt/Luefter/CO2). In dem Fall beiseitelegen statt
        // Zeilen mit unterschiedlicher Spaltenzahl zu mischen.
        File check = SD.open(filename, FILE_READ);
        if (check) {
            String firstLine = check.readStringUntil('\n');
            check.close();
            if (firstLine.indexOf("RaumTemp_C") < 0) {
                char oldName[28];
                snprintf(oldName, sizeof(oldName), "/%04d-%02d-%02d_alt.csv",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
                SD.remove(oldName);
                SD.rename(filename, oldName);
                isNew = true;
            }
        }
    }

    File f = SD.open(filename, FILE_APPEND);
    if (!f) {
        Serial.printf("[SD] Öffnen fehlgeschlagen: %s\n", filename);
        return;
    }
    if (isNew) {
        f.println("Zeit,TempVorrat_C,TempPflanze_C,Druck_bar,Fuellstand_%,Volumen_L,"
                   "RaumTemp_C,RaumFeuchte_%,ZeltTemp_C,ZeltFeuchte_%,ZeltCO2_ppm,ZeltVPD_kPa,"
                   "LichtMaske,LuefterLeistung_%,LuefterRPM,CO2Ausgang");
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
                  (e.zeltOk ? String(e.zeltCo2) : String("")) + "," +
                  (e.zeltOk ? String(e.zeltVpd, 2) : String("")) + "," +
                  String(e.lichtMaske) + "," +
                  (e.fanOk ? String(e.fanPercent) : String("")) + "," +
                  (e.fanOk ? String(e.fanRpm) : String("")) + "," +
                  String(e.co2Ausgang ? 1 : 0);

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
            if (name.endsWith(".csv")) cb(name.c_str(), file.size(), ctx);
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
