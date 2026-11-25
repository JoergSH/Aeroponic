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

void storage_log_ec(time_t ts, const char* kategorie, uint16_t ec25, bool tempOk, float temp,
                    uint16_t ecRaw, bool overrange) {
    if (!sdOk) return;
    struct tm t;
    gmtime_r(&ts, &t);   // RTC laeuft auf Ortszeit, siehe storage_log()
    const char* path = "/" EC_LOG_NAME;
    bool isNew = !SD.exists(path);
    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        dbgPrintf("[SD] Öffnen fehlgeschlagen: %s\n", path);
        return;
    }
    if (isNew) f.println("Datum,Zeit,Kategorie,EC25_uS,EC25_mS,Temp_C,EC_roh_uS,Hinweis");
    char line[128];
    const char* hinweis = overrange ? "ueber Messbereich" : (!tempOk ? "ohne Temperaturfuehler" : "");
    char tempTxt[8] = "";
    if (tempOk) snprintf(tempTxt, sizeof(tempTxt), "%.1f", temp);
    snprintf(line, sizeof(line), "%04d-%02d-%02d,%02d:%02d:%02d,%s,%u,%.2f,%s,%u,%s",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
             kategorie, ec25, ec25 / 1000.0f, tempTxt, ecRaw, hinweis);
    f.println(line);
    f.close();
}

void storage_log_ruecklauf(const char* datum, const char* zeit, uint32_t dauer_s, uint32_t bisMin_s,
                           const char* art, uint32_t volumen_ml) {
    if (!sdOk) return;
    const char* path = "/" RUECKLAUF_LOG_NAME;
    bool isNew = !SD.exists(path);
    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        dbgPrintf("[SD] Öffnen fehlgeschlagen: %s\n", path);
        return;
    }
    if (isNew) f.println("Datum,Zeit,Dauer_s,BisMIN_s,Art,Volumen_ml");
    char line[96];
    snprintf(line, sizeof(line), "%s,%s,%lu,%lu,%s,%lu", datum, zeit, (unsigned long)dauer_s,
             (unsigned long)bisMin_s, art, (unsigned long)volumen_ml);
    f.println(line);
    f.close();
}

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
        // auf ein Merkmal des neuesten Formats (aktuell die EC-Meter-Spalten) faengt
        // automatisch auch alle aelteren Formate.
        File check = SD.open(filename, FILE_READ);
        if (check) {
            String firstLine = check.readStringUntil('\n');
            check.close();
            if (firstLine.indexOf("EC_VE_Temp_C") < 0) {
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
                   "MS2_Abfragen,MS2_Timeouts,VPD_Blatt_kPa,VPD_Soll_min,VPD_Soll_max,"
                   "EC_Naehr_uS,EC_Naehr_Temp_C,EC_Rueck_uS,EC_Rueck_Temp_C,EC_CalMag_uS,EC_CalMag_Temp_C,"
                   "EC_Roh_uS,EC_Roh_Temp_C,EC_VE_uS,EC_VE_Temp_C");
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
    for (uint8_t k = 0; k < EC_LOG_KATS; k++) {
        line += "," + (e.ecValid[k] ? String(e.ecUs[k]) : String("")) +
                "," + (e.ecValid[k] && e.ecTempOk[k] ? String(e.ecTemp[k], 1) : String(""));
    }

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

// ---------- Upload EC-Messungen.csv ----------
static const char* EC_TMP_PATH = "/EC-Messungen.tmp";
static File   ecUpFile;
static size_t ecUpBytes = 0;
static bool   ecUpTooBig = false;

bool storage_ec_upload_begin() {
    if (!sdOk) return false;
    if (ecUpFile) ecUpFile.close();
    if (SD.exists(EC_TMP_PATH)) SD.remove(EC_TMP_PATH);
    ecUpFile   = SD.open(EC_TMP_PATH, FILE_WRITE);
    ecUpBytes  = 0;
    ecUpTooBig = false;
    return (bool)ecUpFile;
}

bool storage_ec_upload_write(const uint8_t* buf, size_t len) {
    if (!ecUpFile || ecUpTooBig) return false;
    if (ecUpBytes + len > EC_UPLOAD_MAX_BYTES) { ecUpTooBig = true; return false; }
    size_t w = ecUpFile.write(buf, len);
    ecUpBytes += w;
    return w == len;
}

void storage_ec_upload_abort() {
    if (ecUpFile) ecUpFile.close();
    if (sdOk && SD.exists(EC_TMP_PATH)) SD.remove(EC_TMP_PATH);
    ecUpBytes = 0;
}

bool storage_ec_upload_finish(char* err, size_t errLen) {
    if (!ecUpFile) { snprintf(err, errLen, "Kein Upload aktiv / SD fehlt"); return false; }
    ecUpFile.close();
    if (ecUpTooBig) { storage_ec_upload_abort(); snprintf(err, errLen, "Datei zu gross (max. 2 MB)"); return false; }
    if (ecUpBytes == 0) { storage_ec_upload_abort(); snprintf(err, errLen, "Datei leer"); return false; }

    // Kopfzeile pruefen -- dieselben Pflichtspalten, die die Auswertung braucht
    File chk = SD.open(EC_TMP_PATH, FILE_READ);
    String head = chk ? chk.readStringUntil('\n') : String();
    if (chk) chk.close();
    head.trim();
    if (head.indexOf("Datum") < 0 || head.indexOf("Kategorie") < 0 || head.indexOf("EC25_uS") < 0 ||
        head.indexOf(';') >= 0) {
        storage_ec_upload_abort();
        snprintf(err, errLen, "Kopfzeile passt nicht (erwartet Datum,Zeit,Kategorie,EC25_uS,... mit Komma getrennt)");
        return false;
    }

    const char* path = "/" EC_LOG_NAME;
    const char* bak  = "/" EC_BACKUP_NAME;
    if (SD.exists(path)) {
        if (SD.exists(bak)) SD.remove(bak);
        if (!SD.rename(path, bak)) { storage_ec_upload_abort(); snprintf(err, errLen, "Sicherung der alten Datei fehlgeschlagen"); return false; }
    }
    if (!SD.rename(EC_TMP_PATH, path)) {
        if (SD.exists(bak)) SD.rename(bak, path);   // alte Datei zurueck
        storage_ec_upload_abort();
        snprintf(err, errLen, "Ersetzen fehlgeschlagen -- alte Datei unveraendert");
        return false;
    }
    dbgPrintf("[SD] %s ersetzt (%u Bytes hochgeladen, alte Datei als %s)\n", EC_LOG_NAME, (unsigned)ecUpBytes, EC_BACKUP_NAME);
    ecUpBytes = 0;
    return true;
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
