R"rawliteral(
        <!-- ============================================================ -->
        <!-- SYSTEM — Gehäuse-Lüfter, Update, Reset, WLAN, Systemanzeige, Log -->
        <!-- ============================================================ -->
        <div id="systemTab" class="tab-content">

            <!-- System-Neustart -->
            <h3 class="section-heading">🔁 <span class="de">Neustart</span><span class="en">Restart</span></h3>
            <div class="status-box-info" style="display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px;">
                <div style="font-size:0.82em;color:#888;">
                    <span class="de">Startet die Steuerung neu (kein Konfigurationsverlust). Laufende Bewässerungs-/Steuerzyklen werden dabei unterbrochen.</span>
                    <span class="en">Restarts the controller (no configuration is lost). Any running watering/control cycles are interrupted.</span>
                </div>
                <div style="display:flex;align-items:center;gap:8px;">
                    <button onclick="systemRestart()" class="button" style="background:#dc3545;padding:9px 22px;">🔁 <span class="de">Neustart</span><span class="en">Restart</span></button>
                    <span id="systemRestartStatus" style="font-size:0.85em;"></span>
                </div>
            </div>

            <!-- WLAN-Konfiguration -->
            <h3 class="section-heading">📶 WLAN</h3>
            <div class="status-box-info">
                <div id="wifiCurrentStatus" style="margin-bottom:10px;font-size:0.9em;color:#555;"><span class="de">Lade...</span><span class="en">Loading...</span></div>
                <div style="display:flex;flex-direction:column;gap:8px;max-width:340px;">
                    <label>SSID:
                        <input type="text" id="wifiSsid" maxlength="32" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </label>
                    <label><span class="de">Passwort</span><span class="en">Password</span>:
                        <input type="password" id="wifiPassword" maxlength="64" placeholder="unverändert lassen" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </label>
                    <div style="display:flex;gap:8px;align-items:center;">
                        <button onclick="saveWifiConfigForm()" class="button" style="background:#3498db;padding:9px 22px;"><span class="de">Speichern & Neustart</span><span class="en">Save & Restart</span></button>
                        <span id="wifiSaveStatus" style="font-size:0.85em;"></span>
                    </div>
                </div>
                <div class="warning" style="margin-top:10px;font-size:0.82em;">
                    <span class="de">Nach dem Speichern startet der Master neu und verbindet sich mit dem neuen WLAN. Ist kein WLAN erreichbar (z. B. falsches Passwort), startet automatisch ein Fallback-Hotspot (<code id="wifiApSsidHint">—</code>), über den die Zugangsdaten erneut geändert werden können.</span>
                    <span class="en">After saving, the master restarts and connects to the new WiFi. If no WiFi is reachable (e.g. wrong password), a fallback hotspot starts automatically (<code id="wifiApSsidHintEn">—</code>), through which the credentials can be changed again.</span>
                </div>
            </div>

            <!-- DS18B20-Sensoren -->
            <h3 class="section-heading">🌡️ <span class="de">DS18B20-Sensoren</span><span class="en">DS18B20 Sensors</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Ordnet jeden Temperatursensor über seine eindeutige Adresse einer Rolle zu — unabhängig davon, in welcher Reihenfolge/an welchem Kabel er am Bus hängt. Zum Identifizieren: Sensor anfassen/erwärmen und beobachten, welcher Messwert sich ändert.</span>
                    <span class="en">Assigns each temperature sensor to a role via its unique address — independent of cable order on the bus. To identify: touch/warm a sensor and watch which reading changes.</span>
                </div>
                <button onclick="scanDs18b20()"
                    style="background:#17a2b8;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                    🔍 <span class="de">Scannen</span><span class="en">Scan</span></button>
                <div id="ds18b20List" style="margin-top:12px;">
                    <div style="color:#aaa;font-size:0.85em;"><span class="de">Noch nicht gescannt.</span><span class="en">Not scanned yet.</span></div>
                </div>
            </div>

            <!-- Node-Steuerung -->
            <h3 class="section-heading">📡 <span class="de">Node-Steuerung</span><span class="en">Node Control</span></h3>
            <div id="nodesControlContainer" style="display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:15px;">
                <div id="nodes_control_loading" style="text-align:center;color:#888;padding:30px;grid-column:1/-1;"><span class="de">Lade...</span><span class="en">Loading...</span></div>
            </div>

            <!-- Gehäuselüfter-Steuerung -->
            <div id="gehaeuseFanSection" style="display:none;">
                <h3 class="section-heading">🌡️ <span class="de">Gehäuselüfter-Steuerung</span><span class="en">Enclosure Fan Control</span></h3>
                <div class="status-box-info">
                    <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                        <span class="de">Reiner An/Aus-Lüfter (GPIO6, kein PWM), geregelt über die Innentemperatur der Steuerung (zusätzlicher DS18B20) mit Hysterese: an ab Temp Max, aus ab/unter Temp Min — dazwischen bleibt der Lüfter im zuletzt gesetzten Zustand.</span>
                        <span class="en">Plain on/off fan (GPIO6, no PWM), controlled by the controller's internal temperature (extra DS18B20) with hysteresis: on at/above temp max, off at/below temp min — in between it keeps its last state.</span>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                            <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                            <input type="checkbox" id="gehEnabled" style="width:18px;height:18px;">
                        </label>
                        <span style="font-size:0.85em;color:#6c757d;"><span class="de">Innentemp.</span><span class="en">Internal temp</span>: <span id="gehTempInnen">--</span> °C &nbsp; → <span id="gehOnOff">--</span></span>
                    </div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                        <div>
                            <div class="status-label"><span class="de">Temp Min (°C)</span><span class="en">Temp Min (°C)</span></div>
                            <input type="number" id="gehTempMin" min="0" max="80" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Ab/darunter: aus</span><span class="en">At/below: off</span></div>
                        </div>
                        <div>
                            <div class="status-label"><span class="de">Temp Max (°C)</span><span class="en">Temp Max (°C)</span></div>
                            <input type="number" id="gehTempMax" min="0" max="80" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Ab: an</span><span class="en">At/above: on</span></div>
                        </div>
                    </div>
                    <div style="display:flex;gap:8px;align-items:center;">
                        <button onclick="saveGehaeuseFanForm()"
                            style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                            <span class="de">Speichern</span><span class="en">Save</span></button>
                        <span id="gehSaveStatus" style="font-size:0.85em;color:#888;"></span>
                    </div>
                </div>
            </div>

            <!-- Firmware-Update -->
            <h3 class="section-heading">🔄 <span class="de">Firmware-Update (OTA)</span><span class="en">Firmware Update (OTA)</span></h3>
            <div class="update-section">
                <div class="warning">
                    <strong><span class="de">Wichtig:</span><span class="en">Important:</span></strong><br>
                    <span class="de">• Stromversorgung nicht unterbrechen<br>• Nur .bin Dateien<br>• System startet nach Update neu</span>
                    <span class="en">• Do not interrupt the power supply<br>• Only .bin files<br>• System restarts after the update</span>
                </div>
                <form method='POST' action='/update' enctype='multipart/form-data' onsubmit='return startUpdate()'>
                    <label>Firmware (.bin):</label>
                    <input type='file' name='update' accept='.bin' class="file-input" required>
                    <div style="text-align: center; margin: 20px 0;">
                        <input type='submit' value='Update starten' class="button button-update" id="updateSubmitBtn">
                    </div>
                    <div id="progressBar" class="progress-bar" style="display: none;">
                        <div id="progressFillUpdate" class="progress-fill">0%</div>
                    </div>
                    <div id="updateStatus" style="text-align: center;"></div>
                </form>
            </div>

            <!-- Firmware-Update Multimodul ueber RS485 (siehe rs485_ota_*() in rs485.cpp) -->
            <h3 class="section-heading">🔌 <span class="de">Multimodul-Update (RS485)</span><span class="en">Multimodul Update (RS485)</span></h3>
            <div class="update-section">
                <div class="warning">
                    <span class="de">• Datei: <code>Multimodul/.pio/build/waveshare_rp2040_zero/firmware.bin</code> (nicht .uf2)<br>• Dauer ca. 30–60 s, währenddessen pausiert der RS485-Bus (Lüfter/Sensoren/Analog melden kurz keine Werte)<br>• Danach startet das Multimodul neu, die Sollwerte werden automatisch neu gesetzt<br>• Bricht die Übertragung ab, bleibt die alte Firmware aktiv</span>
                    <span class="en">• File: <code>Multimodul/.pio/build/waveshare_rp2040_zero/firmware.bin</code> (not .uf2)<br>• Takes about 30–60 s; the RS485 bus is paused meanwhile (fans/sensors/analog briefly report no values)<br>• The Multimodul then restarts, setpoints are restored automatically<br>• If the transfer is interrupted, the old firmware stays active</span>
                </div>
                <div style="margin:10px 0;">
                    <span class="de">Installierte Version:</span><span class="en">Installed version:</span>
                    <strong id="rs485OtaVersion">–</strong>
                    <button type="button" class="button" style="margin-left:10px;padding:4px 10px;" onclick="rs485OtaQueryVersion()"><span class="de">Abfragen</span><span class="en">Query</span></button>
                </div>
                <label>Firmware (.bin):</label>
                <input type="file" id="rs485OtaFile" accept=".bin" class="file-input">
                <div style="text-align:center;margin:20px 0;">
                    <button type="button" class="button button-update" id="rs485OtaBtn" onclick="rs485OtaUpload()"><span class="de">Update starten</span><span class="en">Start update</span></button>
                    <button type="button" class="button" id="rs485OtaAbortBtn" onclick="rs485OtaAbort()" style="display:none;"><span class="de">Abbrechen</span><span class="en">Abort</span></button>
                </div>
                <div id="rs485OtaProgress" class="progress-bar" style="display:none;">
                    <div id="rs485OtaFill" class="progress-fill">0%</div>
                </div>
                <div id="rs485OtaStatus" style="text-align:center;"></div>
            </div>

            <!-- Daten-Log -->
            <h3 class="section-heading">📥 <span class="de">Daten-Log (SD-Karte)</span><span class="en">Data Log (SD Card)</span></h3>
            <div class="status-box-info">
                <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;">
                    <select id="logFileSelect" style="flex:1;min-width:200px;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <option value="" id="logFileSelectLoadingOpt"><span class="de">Lade Dateiliste...</span></option>
                    </select>
                    <button onclick="downloadLogFile()" class="button" style="background:#3498db;padding:9px 22px;">⬇ Download</button>
                    <button onclick="deleteLogFile()" class="button" style="background:#dc3545;padding:9px 16px;">🗑 <span class="de">Löschen</span><span class="en">Delete</span></button>
                    <button onclick="loadLogFileList()" class="button" style="background:#6c757d;padding:9px 16px;">🔄</button>
                </div>
                <div style="margin-top:12px;padding-top:12px;border-top:1px solid #eee;display:flex;gap:8px;align-items:center;flex-wrap:wrap;">
                    <button onclick="deleteAllLogFiles()" class="button" style="background:#721c24;padding:9px 16px;">🗑 <span class="de">Alle Dateien löschen</span><span class="en">Delete all files</span></button>
                    <span id="deleteAllLogStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
                <div style="margin-top:12px;padding-top:12px;border-top:1px solid #eee;display:flex;gap:8px;align-items:center;flex-wrap:wrap;">
                    <button onclick="startDebugCapture()" id="debugCaptureBtn"
                        style="background:#e67e22;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        🐞 <span class="de">60s Debug-Aufzeichnung starten</span><span class="en">Start 60s debug capture</span></button>
                    <span id="debugCaptureStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
                <div style="font-size:0.78em;color:#888;margin-top:6px;">
                    <span class="de">Schreibt 60 Sekunden lang alle seriellen Meldungen zusätzlich in eine Datei auf der SD-Karte — danach oben in der Liste als "debug_....txt" abrufbar (Dateiliste ggf. mit 🔄 aktualisieren).</span>
                    <span class="en">Writes all serial output to an SD card file for 60 seconds — afterwards available above as "debug_....txt" (refresh the list with 🔄 if needed).</span>
                </div>
            </div>

            <!-- System-Info -->
            <h3 class="section-heading">⚙️ <span class="de">System</span><span class="en">System</span></h3>
            <div class="status-box-info">
                <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:0.85em;">
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="freeHeap">--</div><div class="sensor-unit"><span class="de">Speicher</span><span class="en">Memory</span></div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="uptime">--</div><div class="sensor-unit"><span class="de">Laufzeit</span><span class="en">Uptime</span></div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="chipModel">--</div><div class="sensor-unit">Chip</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="chipRevision">--</div><div class="sensor-unit"><span class="de">Rev.</span><span class="en">Rev.</span></div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="flashSize">--</div><div class="sensor-unit">Flash</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="sketchSize">--</div><div class="sensor-unit">Sketch</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="fwVersion">--</div><div class="sensor-unit"><span class="de">FW-Version</span><span class="en">FW version</span></div></div>
                </div>
                <div style="margin-top:10px;font-size:0.85em;color:#555;">
                    <span class="de">Letzter Neustart:</span><span class="en">Last restart:</span> <strong id="lastResetReason">--</strong>
                </div>
            </div>
        </div>
)rawliteral"
