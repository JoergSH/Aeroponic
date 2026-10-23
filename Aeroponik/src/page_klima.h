R"rawliteral(
        <!-- ============================================================ -->
        <!-- KLIMA — Licht, CO2, Luftfeuchte, Abluftlüfter, Zeltlüfter      -->
        <!-- ============================================================ -->
        <div id="klimaTab" class="tab-content">

            <!-- Geraete-Auswahl: Licht-/Luefter-Ausgang -->
            <h3 class="section-heading">🔌 <span class="de">Geräte</span><span class="en">Devices</span></h3>
            <div class="status-box-info">
                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Licht-Ausgang</span><span class="en">Light Output</span></div>
                        <select id="outputLight" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0" class="i18n-opt" data-de="Lichtx4 (Relais)" data-en="Lichtx4 (Relay)">Lichtx4 (Relais)</option>
                            <option value="1" class="i18n-opt" data-de="RS485-Analogmodul (0-10V)" data-en="RS485 analog module (0-10V)">RS485-Analogmodul (0-10V)</option>
                            <option value="2" class="i18n-opt" data-de="GP8403 I2C-DAC (0-10V)" data-en="GP8403 I2C DAC (0-10V)">GP8403 I2C-DAC (0-10V)</option>
                        </select>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Lüfter-Ausgang</span><span class="en">Fan Output</span></div>
                        <select id="outputFan" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0" class="i18n-opt" data-de="MARS Hydro (RS485)" data-en="MARS Hydro (RS485)">MARS Hydro (RS485)</option>
                            <option value="1" class="i18n-opt" data-de="RS485-Analogmodul (0-10V)" data-en="RS485 analog module (0-10V)">RS485-Analogmodul (0-10V)</option>
                            <option value="2" class="i18n-opt" data-de="GP8403 I2C-DAC (0-10V)" data-en="GP8403 I2C DAC (0-10V)">GP8403 I2C-DAC (0-10V)</option>
                        </select>
                    </div>
                </div>
                <div style="font-size:0.78em;color:#888;margin-bottom:16px;">
                    <span class="de">Legt fest, welches Gerät angesteuert wird — nur dieses wird auch auf dem RS485-Bus abgefragt. Feineinstellungen (Rampen, Zeiten) stehen weiter unten in diesem Tab.</span>
                    <span class="en">Determines which device is driven — only that device is polled on the RS485 bus. Fine-tuning (ramps, timing) is further down in this tab.</span>
                </div>
                <div style="display:flex;gap:8px;align-items:center;margin-bottom:12px;">
                    <button onclick="saveOutputConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="outputSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
                <div style="font-size:0.82em;color:#555;">
                    <span class="de">Analog-Modul:</span><span class="en">Analog module:</span> <span id="analogStatus">--</span>
                </div>
            </div>

            <!-- Zeitplan-Konfiguration -->
            <h3 class="section-heading">🌅 <span class="de">Zeitplan-Konfiguration</span><span class="en">Schedule Configuration</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Ausgang (Lichtx4 / Analog) wird oben unter Geräte festgelegt.</span>
                    <span class="en">Output (Lichtx4 / Analog) is set above under Devices.</span>
                </div>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="schedEnabled" onchange="schedDirty();updateSchedPhase()" style="width:18px;height:18px;">
                    </label>
                </div>

                <div id="schedNumSsrRow" style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Anzahl Kanäle</span><span class="en">Channel Count</span></div>
                        <input type="number" id="schedNumSsr" min="1" max="4" value="4"
                            oninput="schedDirty();drawSchedChart();updateSchedPhase()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Anzahl SSR-Ausgänge (1–4)</span><span class="en">Number of SSR outputs (1–4)</span></div>
                    </div>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Sonnenaufgang — Start</span><span class="en">Sunrise — Start</span></div>
                        <input type="time" id="schedDawnStart" oninput="schedDirty();drawSchedChart();updateSchedPhase()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Beginn Aufblenden</span><span class="en">Fade-in start</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Sonnenaufgang — Ende</span><span class="en">Sunrise — End</span></div>
                        <input type="time" id="schedDawnEnd" oninput="schedDirty();drawSchedChart();updateSchedPhase()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Volle Helligkeit ab</span><span class="en">Full brightness from</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Sonnenuntergang — Start</span><span class="en">Sunset — Start</span></div>
                        <input type="time" id="schedDuskStart" oninput="schedDirty();drawSchedChart();updateSchedPhase()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Beginn Abblenden</span><span class="en">Fade-out start</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Sonnenuntergang — Ende</span><span class="en">Sunset — End</span></div>
                        <input type="time" id="schedDuskEnd" oninput="schedDirty();drawSchedChart();updateSchedPhase()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Licht aus</span><span class="en">Light off</span></div>
                    </div>
                </div>

                <div style="display:flex;gap:8px;align-items:center;">
                    <button id="schedSaveBtn" onclick="saveZeitplan()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="schedSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

            <!-- DWC-Beleuchtungstimer -->
            <h3 class="section-heading">💡 DWC-<span class="de">Beleuchtungstimer</span><span class="en">Lighting Timer</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Einfacher Ein/Aus-Timer für ein separates DWC-System (kein Sonnenauf-/untergang wie beim Hauptzeitplan) — geschaltet über einen Ausgang eines Steckdosen-Nodes.</span>
                    <span class="en">Simple on/off timer for a separate DWC system (no dawn/dusk ramp like the main schedule) — switched via an output on a socket node.</span>
                </div>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="dwcEnabled" style="width:18px;height:18px;">
                    </label>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Steckdose (Node)</span><span class="en">Socket (Node)</span></div>
                        <select id="dwcNode" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="" id="dwcNodeNoneOpt">— keine —</option>
                        </select>
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Nur online erkannte Steckdosen-Nodes</span><span class="en">Only sockets currently detected online</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Ausgang</span><span class="en">Output</span></div>
                        <select id="dwcRelayBit" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0">R1</option>
                            <option value="1">R2</option>
                            <option value="2">R3</option>
                            <option value="3">R4</option>
                        </select>
                    </div>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Ein</span><span class="en">On</span></div>
                        <input type="time" id="dwcOnTime" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Aus</span><span class="en">Off</span></div>
                        <input type="time" id="dwcOffTime" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                </div>

                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveDwcConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="dwcSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

            <!-- Raumlicht: rein manueller Schalter (kein Timer/Automatik), geschaltet ueber einen
                 Ausgang eines Steckdosen-Nodes. Solange kein Ausgang ausgewaehlt ist, bleibt die
                 Karte auf der Startseite versteckt. -->
            <h3 class="section-heading">💡 <span class="de">Raumlicht</span><span class="en">Room Light</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Manueller Ein/Aus-Schalter für ein Raumlicht über einen Ausgang eines Steckdosen-Nodes — erscheint als Schalter auf der Startseite, sobald hier ein Ausgang ausgewählt wurde.</span>
                    <span class="en">Manual on/off switch for a room light via an output on a socket node — appears as a switch on the home page once an output is selected here.</span>
                </div>
                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Steckdose (Node)</span><span class="en">Socket (Node)</span></div>
                        <select id="raumlichtNode" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="" id="raumlichtNodeNoneOpt">— keine —</option>
                        </select>
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Nur online erkannte Steckdosen-Nodes</span><span class="en">Only sockets currently detected online</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Ausgang</span><span class="en">Output</span></div>
                        <select id="raumlichtRelayBit" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0">R1</option>
                            <option value="1">R2</option>
                            <option value="2">R3</option>
                            <option value="3">R4</option>
                        </select>
                    </div>
                </div>
                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveRaumlichtConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="raumlichtSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

            <!-- CO2-Steuerung -->
            <h3 class="section-heading">🌫️ CO2-<span class="de">Steuerung</span><span class="en">Control</span></h3>
            <div class="status-box-info">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="co2Enabled" style="width:18px;height:18px;">
                    </label>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Steckdose (Node)</span><span class="en">Socket (Node)</span></div>
                        <select id="co2Node" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="" id="co2NodeNoneOpt">— keine —</option>
                        </select>
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Nur online erkannte Steckdosen-Nodes</span><span class="en">Only sockets currently detected online</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Ausgang</span><span class="en">Output</span></div>
                        <select id="co2RelayBit" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0">R1</option>
                            <option value="1">R2</option>
                            <option value="2">R3</option>
                            <option value="3">R4</option>
                        </select>
                    </div>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label">CO2 Min (ppm)</div>
                        <input type="number" id="co2Min" min="0" max="5000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darunter: Ausgang EIN</span><span class="en">Below: output ON</span></div>
                    </div>
                    <div>
                        <div class="status-label">CO2 Max (ppm)</div>
                        <input type="number" id="co2Max" min="0" max="5000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: Ausgang AUS</span><span class="en">Above: output OFF</span></div>
                    </div>
                </div>

                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveCo2ConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="co2SaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

            <!-- Luftbefeuchtung-Steuerung — 1:1 Kopie der CO2-Steuerung, Feuchte statt CO2 -->
            <h3 class="section-heading">💧 <span class="de">Luftbefeuchtung</span><span class="en">Humidification</span></h3>
            <div class="status-box-info">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="humidEnabled" style="width:18px;height:18px;">
                    </label>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Steckdose (Node)</span><span class="en">Socket (Node)</span></div>
                        <select id="humidNode" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="">— keine —</option>
                        </select>
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Nur online erkannte Steckdosen-Nodes</span><span class="en">Only sockets currently detected online</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Ausgang</span><span class="en">Output</span></div>
                        <select id="humidRelayBit" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0">R1</option>
                            <option value="1">R2</option>
                            <option value="2">R3</option>
                            <option value="3">R4</option>
                        </select>
                    </div>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Feuchte Min (%)</span><span class="en">Humidity Min (%)</span></div>
                        <input type="number" id="humidMin" min="0" max="100" step="0.1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darunter: Ausgang EIN (befeuchten)</span><span class="en">Below: output ON (humidify)</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Feuchte Max (%)</span><span class="en">Humidity Max (%)</span></div>
                        <input type="number" id="humidMax" min="0" max="100" step="0.1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: Ausgang AUS</span><span class="en">Above: output OFF</span></div>
                    </div>
                </div>

                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveHumidConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="humidSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

            <!-- Abluftluefter-Steuerung -->
            <h3 class="section-heading">🌀 <span class="de">Abluftlüfter-Steuerung</span><span class="en">Exhaust Fan Control</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Ausgang (MARS Hydro / Analog) wird oben unter Geräte festgelegt.</span>
                    <span class="en">Output (MARS Hydro / Analog) is set above under Devices.</span>
                </div>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Eigene Steuerung aktiv</span><span class="en">Own control active</span></span>
                        <input type="checkbox" id="fanEnabled" style="width:18px;height:18px;">
                    </label>
                </div>
                <div style="font-size:0.78em;color:#888;margin-bottom:16px;">
                    <span class="de">Wenn deaktiviert, sendet der Master keine eigenen Befehle — der iControl-Hub behält die Kontrolle über den Lüfter.</span>
                    <span class="en">When disabled, the master sends no commands of its own — the iControl hub retains control over the fan.</span>
                </div>

                <div style="margin-bottom:16px;">
                    <div class="status-label"><span class="de">Modus</span><span class="en">Mode</span></div>
                    <select id="fanMode" onchange="updateFanModeVisibility()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <option value="0" class="i18n-opt" data-de="Manuell" data-en="Manual">Manuell</option>
                        <option value="1" class="i18n-opt" data-de="Automatisch nach Luftfeuchte" data-en="Automatic by humidity">Automatisch nach Luftfeuchte</option>
                        <option value="2" class="i18n-opt" data-de="Automatisch nach Temperatur (senken)" data-en="Automatic by temperature (reduce)">Automatisch nach Temperatur (senken)</option>
                    </select>
                </div>

                <div id="fanManualSection" style="margin-bottom:16px;">
                    <div class="status-label"><span class="de">Leistung (%)</span><span class="en">Power (%)</span></div>
                    <input type="number" id="fanManualPercent" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                </div>

                <div id="fanHumSection" style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Feuchte Min (%rH)</span><span class="en">Humidity Min (%rH)</span></div>
                        <input type="number" id="fanHumMin" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darunter: Mindestdrehzahl</span><span class="en">Below: minimum speed</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Feuchte Max (%rH)</span><span class="en">Humidity Max (%rH)</span></div>
                        <input type="number" id="fanHumMax" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: 100%</span><span class="en">Above: 100%</span></div>
                    </div>
                </div>

                <div id="fanTempSection" style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div style="grid-column:1/-1;">
                        <div class="status-label"><span class="de">Temperatur-Basis</span><span class="en">Temperature Basis</span></div>
                        <select id="fanTempMode" onchange="updateFanTempModeVisibility()" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <option value="0" class="i18n-opt" data-de="Absolute Zelttemperatur" data-en="Absolute tent temperature">Absolute Zelttemperatur</option>
                            <option value="1" class="i18n-opt" data-de="Differenz Zelt − Raum" data-en="Differential tent − room">Differenz Zelt − Raum</option>
                        </select>
                        <div style="font-size:0.78em;color:#888;margin-top:3px;">
                            <span class="de">"Differenz" vermeidet Volllast, wenn der Raum fast so warm wie das Zelt ist — Lüften bringt dann kaum Kühlung.</span>
                            <span class="en">"Differential" avoids full blast when the room is nearly as warm as the tent — venting then barely cools anything.</span>
                        </div>
                    </div>
                    <div>
                        <div class="status-label" id="fanTempMinLabel"><span class="de">Temperatur Min (°C)</span><span class="en">Temperature Min (°C)</span></div>
                        <input type="number" id="fanTempMin" min="0" max="60" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darunter: Mindestdrehzahl</span><span class="en">Below: minimum speed</span></div>
                    </div>
                    <div>
                        <div class="status-label" id="fanTempMaxLabel"><span class="de">Temperatur Max (°C)</span><span class="en">Temperature Max (°C)</span></div>
                        <input type="number" id="fanTempMax" min="0" max="60" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: 100%</span><span class="en">Above: 100%</span></div>
                    </div>
                    <div id="fanTempCapSection" style="grid-column:1/-1;">
                        <div class="status-label"><span class="de">Differenz-Bremse (°C, 0 = aus)</span><span class="en">Differential Cap (°C, 0 = off)</span></div>
                        <input type="number" id="fanTempCapDiff" min="0" max="30" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;">
                            <span class="de">Ist die Differenz Zelt−Raum kleiner als dieser Wert, läuft der Lüfter trotz obiger Rampe nur mit Mindestdrehzahl.</span>
                            <span class="en">If the tent−room difference is below this value, the fan runs at minimum speed regardless of the ramp above.</span>
                        </div>
                    </div>
                </div>

                <div style="margin-bottom:16px;">
                    <div class="status-label"><span class="de">Mindestdrehzahl (%) — für beide Automatik-Modi</span><span class="en">Minimum Speed (%) — for both auto modes</span></div>
                    <input type="number" id="fanMinSpeed" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                </div>

                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveFanConfigForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="fanSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>

                <!-- Diagnose: MARS-Hydro-Adresse pruefen, falls der Luefter unerwartet nicht mehr
                     antwortet (Standardadresse 6). Reines Diagnosewerkzeug, kein Dauerbetrieb. -->
                <div style="margin-top:20px;padding-top:16px;border-top:1px solid #eee;">
                    <div style="font-size:0.85em;color:#555;font-weight:bold;margin-bottom:6px;">
                        🔍 <span class="de">Diagnose: RS485-Adresse suchen</span><span class="en">Diagnostics: Find RS485 address</span>
                    </div>
                    <div style="font-size:0.78em;color:#888;margin-bottom:8px;">
                        <span class="de">Falls der Lüfter (normal Adresse 6) nicht mehr antwortet: probiert nacheinander jede Adresse im Bereich per Leseanfrage durch. Bekannte eigene Adressen (MS2, Lichtx4, Analog, Lüftermodul) antworten dabei ebenfalls — das ist normal, interessant sind nur zusätzliche Treffer.</span>
                        <span class="en">If the fan (normally address 6) stops responding: probes every address in the range with a read request. Your own known addresses (MS2, Lichtx4, Analog, fan module) will also answer — that's expected, only extra hits matter.</span>
                    </div>
                    <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;">
                        <span class="de">von</span><span class="en">from</span>
                        <input type="number" id="rs485ScanFrom" min="1" max="247" value="1" style="width:70px;padding:6px;border:1px solid #ddd;border-radius:4px;">
                        <span class="de">bis</span><span class="en">to</span>
                        <input type="number" id="rs485ScanTo" min="1" max="247" value="20" style="width:70px;padding:6px;border:1px solid #ddd;border-radius:4px;">
                        <button id="rs485ScanBtn" onclick="scanRs485Addresses()" style="background:#17a2b8;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                            <span class="de">Scan starten</span><span class="en">Start scan</span></button>
                    </div>
                    <div id="rs485ScanResult" style="margin-top:10px;font-size:0.85em;"></div>
                </div>
            </div>

            <!-- Zeltlüfter-Steuerung (Lueftermodul, RS485 0x51) -->
            <h3 class="section-heading">🌀 <span class="de">Zeltlüfter-Steuerung</span><span class="en">Tent Fan Control</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">4 PWM-Lüfter im Zelt (Arctic, 600–3000 U/min) über das RS485-Lüftermodul (Adresse 0x51). Unter 5 % PWM steht der Lüfter (0 U/min). Einstellung in 10 %-Schritten.</span>
                    <span class="en">4 PWM fans in the tent (Arctic, 600–3000 rpm) via the RS485 fan module (address 0x51). Below 5 % PWM the fan stops (0 rpm). Adjustable in 10 % steps.</span>
                </div>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="lueftEnabled" style="width:18px;height:18px;">
                    </label>
                    <span id="lueftOnlineBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                </div>
                <!-- Immer sichtbare RPM-Anzeige, unabhaengig davon ob "gleiche Werte" die
                     Einzel-Schieberegler (und die Anzeige darin) gerade ausblendet. -->
                <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(70px,1fr));gap:8px;margin-bottom:14px;">
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="lueftRpm0">--</div><div class="sensor-unit">RPM K1</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="lueftRpm1">--</div><div class="sensor-unit">RPM K2</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="lueftRpm2">--</div><div class="sensor-unit">RPM K3</div></div>
                    <div class="sensor-box" style="padding:6px;"><div class="sensor-value" style="font-size:1em;" id="lueftRpm3">--</div><div class="sensor-unit">RPM K4</div></div>
                </div>
                <label style="display:flex;align-items:center;gap:8px;cursor:pointer;margin-bottom:14px;">
                    <input type="checkbox" id="lueftGleicheWerte" onchange="updateLueftSliderVisibility()" style="width:16px;height:16px;">
                    <span style="font-size:0.9em;"><span class="de">Gleiche Geschwindigkeit für alle Kanäle</span><span class="en">Same speed for all channels</span></span>
                </label>

                <div id="lueftSliderSingleRow" style="display:none;margin-bottom:14px;">
                    <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                        <span class="de">Geschwindigkeit (alle Kanäle)</span><span class="en">Speed (all channels)</span>
                        <span id="lueftSliderSingleValue">0%</span>
                    </div>
                    <input type="range" id="lueftSliderSingle" min="0" max="100" step="10" value="0" style="width:100%;"
                        oninput="onLueftSliderInput(-1)" onchange="onLueftSliderChange()">
                </div>

                <div id="lueftSlidersMulti" style="display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:14px;margin-bottom:8px;">
                    <div>
                        <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                            <span class="de">Kanal 1</span><span class="en">Channel 1</span><span id="lueftSlider0Value">0%</span>
                        </div>
                        <input type="range" id="lueftSlider0" min="0" max="100" step="10" value="0" style="width:100%;"
                            oninput="onLueftSliderInput(0)" onchange="onLueftSliderChange()">
                    </div>
                    <div>
                        <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                            <span class="de">Kanal 2</span><span class="en">Channel 2</span><span id="lueftSlider1Value">0%</span>
                        </div>
                        <input type="range" id="lueftSlider1" min="0" max="100" step="10" value="0" style="width:100%;"
                            oninput="onLueftSliderInput(1)" onchange="onLueftSliderChange()">
                    </div>
                    <div>
                        <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                            <span class="de">Kanal 3</span><span class="en">Channel 3</span><span id="lueftSlider2Value">0%</span>
                        </div>
                        <input type="range" id="lueftSlider2" min="0" max="100" step="10" value="0" style="width:100%;"
                            oninput="onLueftSliderInput(2)" onchange="onLueftSliderChange()">
                    </div>
                    <div>
                        <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                            <span class="de">Kanal 4</span><span class="en">Channel 4</span><span id="lueftSlider3Value">0%</span>
                        </div>
                        <input type="range" id="lueftSlider3" min="0" max="100" step="10" value="0" style="width:100%;"
                            oninput="onLueftSliderInput(3)" onchange="onLueftSliderChange()">
                    </div>
                </div>

                <div style="display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px;margin-top:12px;padding-top:12px;border-top:1px solid #eee;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Drehzahlüberwachung</span><span class="en">Speed monitoring</span></span>
                        <input type="checkbox" id="lueftMonitorEnabled" style="width:18px;height:18px;">
                    </label>
                    <label style="font-size:0.85em;color:#6c757d;display:flex;align-items:center;gap:6px;">
                        <span class="de">Toleranz</span><span class="en">Tolerance</span>:
                        <input type="number" id="lueftMonitorTol" min="5" max="100" style="width:60px;padding:4px;border:1px solid #ddd;border-radius:4px;"> %
                    </label>
                </div>
                <div style="font-size:0.78em;color:#888;margin-top:4px;">
                    <span class="de">Großzügige Toleranz um den erwarteten Wert — löst bei Verkabelungs-/Ausfallproblemen aus, nicht bei normalen Drehzahlschwankungen. Alarm bei Bedarf im Tab Alarme aktivieren.</span>
                    <span class="en">Generous tolerance around the expected value — triggers on wiring/failure issues, not normal speed variation. Enable the alert in the Alarms tab if needed.</span>
                </div>

                <div style="display:flex;gap:8px;align-items:center;margin-top:16px;">
                    <button onclick="saveLueftmodulForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="lueftSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>
        </div>
)rawliteral"
