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

            <!-- Lampen-Kennlinie (0-10V-Ausgang gegen PPFD, siehe loopLichtKalib() im Master) -->
            <h3 class="section-heading">📈 <span class="de">Lampen-Kennlinie (PPFD)</span><span class="en">Lamp Response Curve (PPFD)</span></h3>
            <div class="status-box-info">
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Fährt den 0–10V-Lichtausgang in 5-%-Schritten von 0 bis 100 % und misst je Schritt die PPFD des Multisensors — zeigt, wie linear die Lampe auf die Steuerspannung reagiert. Dauer ca. 8–10 min, währenddessen ist der Zeitplan überstimmt. Sensor dabei nicht bewegen, Zelt möglichst geschlossen (der 0-%-Wert wird als Restlicht abgezogen).</span>
                    <span class="en">Steps the 0–10V light output from 0 to 100 % in 5 % steps and measures the multisensor's PPFD at each step — shows how linearly the lamp responds to the control voltage. Takes about 8–10 min, the schedule is overridden meanwhile. Do not move the sensor, keep the tent closed if possible (the 0 % value is subtracted as residual light).</span>
                </div>
                <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-bottom:12px;">
                    <button id="lkStartBtn" onclick="lichtKalibStart()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        ▶ <span class="de">Messung starten</span><span class="en">Start measurement</span></button>
                    <button id="lkAbortBtn" onclick="lichtKalibAbort()" style="display:none;background:#dc3545;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        ■ <span class="de">Abbrechen</span><span class="en">Abort</span></button>
                    <span id="lkStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
                <label style="display:flex;gap:8px;align-items:flex-start;cursor:pointer;margin-bottom:4px;">
                    <input type="checkbox" id="lkApply" onchange="lichtKalibApply(this.checked)" style="width:18px;height:18px;margin-top:1px;">
                    <span style="font-size:0.9em;"><span class="de">Kennlinie anwenden (Zeitplan und manueller Regler)</span><span class="en">Apply response curve (schedule and manual slider)</span></span>
                </label>
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Rechnet die Helligkeit über die gespeicherte Messung in die passende Steuerspannung um, damit z. B. 50 % auch 50 % der vollen PPFD ergeben. 0 % bleibt aus; unterhalb der kleinsten Stufe, ab der die Lampe leuchtet, gibt es diese Stufe. Nach einem Lampenwechsel neu messen.</span>
                    <span class="en">Converts the brightness into the matching control voltage using the stored measurement, so that e.g. 50 % really gives 50 % of full PPFD. 0 % stays off; below the lowest step at which the lamp lights up, that step is used. Measure again after changing the lamp.</span>
                </div>
                <canvas id="lkChart" width="420" height="220" style="width:100%;max-width:420px;display:none;"></canvas>
                <div id="lkTable" style="margin-top:10px;font-size:0.85em;overflow-x:auto;"></div>
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

            <!-- VPD-Regelung (vpdctrl.cpp): fuehrt Befeuchter + Abluftluefter gemeinsam -->
            <h3 class="section-heading">🌿 <span class="de">VPD-Regelung</span><span class="en">VPD Control</span></h3>
            <div class="status-box-info">
                <style>
                    .vpd-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:12px;margin-bottom:14px}
                    .vpd-in{width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}
                    .vpd-hint{font-size:0.78em;color:#888;margin-top:3px}
                    .vpd-live{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:12px}
                    .vpd-box{background:#fff;border-radius:8px;padding:10px 12px;border:1px solid #e3e6ea}
                    .vpd-big{font-size:1.6em;font-weight:bold;color:#2c3e50}
                    .vpd-big small{font-size:0.5em;color:#888;font-weight:normal}
                    .vpd-gauge{position:relative;height:16px;background:#e9ecef;border-radius:8px;margin:6px 0 14px;overflow:hidden}
                    .vpd-gauge .band{position:absolute;top:0;bottom:0;background:rgba(39,174,96,0.35)}
                    .vpd-gauge .mark{position:absolute;top:-2px;bottom:-2px;width:3px;background:#2c3e50;border-radius:2px}
                    .vpd-reasons{font-size:0.85em;color:#555;margin-bottom:14px;line-height:1.6}
                    .vpd-tbl{font-size:0.82em;color:#555;border-collapse:collapse;margin-top:4px}
                    .vpd-tbl td{padding:2px 10px 2px 0}
                </style>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;flex-wrap:wrap;gap:8px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="vpdEnabled" style="width:18px;height:18px;">
                    </label>
                    <span id="vpdBadge" class="device-badge" style="background:#6c757d;">--</span>
                </div>

                <!-- Live-Status -->
                <div id="vpdLiveWrap">
                    <div class="vpd-live">
                        <div class="vpd-box"><div class="status-label"><span class="de">VPD Blatt (geregelt)</span><span class="en">Leaf VPD (controlled)</span></div><div class="vpd-big"><span id="vpdLeaf">--</span> <small>kPa</small></div></div>
                        <div class="vpd-box"><div class="status-label"><span class="de">VPD Luft</span><span class="en">Air VPD</span></div><div class="vpd-big"><span id="vpdAir">--</span> <small>kPa</small></div></div>
                        <div class="vpd-box"><div class="status-label"><span class="de">Zielbereich</span><span class="en">Target range</span> <span id="vpdPhaseNow" style="color:#999;"></span></div><div class="vpd-big"><span id="vpdSoll">--</span> <small>kPa</small></div></div>
                        <div class="vpd-box"><div class="status-label"><span class="de">Befeuchter / Abluft</span><span class="en">Humidifier / exhaust</span></div><div class="vpd-big"><span id="vpdOut">--</span></div></div>
                    </div>
                    <div class="vpd-gauge" title="0 – 2,0 kPa"><div class="band" id="vpdGaugeBand"></div><div class="mark" id="vpdGaugeMark"></div></div>
                    <div class="vpd-reasons" id="vpdReasons"></div>
                </div>

                <!-- Einstellungen -->
                <div class="vpd-grid">
                    <div>
                        <div class="status-label"><span class="de">Wachstumsphase</span><span class="en">Growth stage</span></div>
                        <select id="vpdPhase" class="vpd-in">
                            <option value="0" class="i18n-opt" data-de="Steckling / Keimling" data-en="Clone / seedling">Steckling / Keimling</option>
                            <option value="1" class="i18n-opt" data-de="Wachstum" data-en="Vegetative">Wachstum</option>
                            <option value="2" class="i18n-opt" data-de="Blüte früh" data-en="Early flower">Blüte früh</option>
                            <option value="3" class="i18n-opt" data-de="Blüte spät" data-en="Late flower">Blüte spät</option>
                            <option value="4" class="i18n-opt" data-de="Eigene Werte" data-en="Custom values">Eigene Werte</option>
                        </select>
                        <table class="vpd-tbl"><tr><td>☀️ <span class="de">Licht an</span><span class="en">Lights on</span></td><td><b id="vpdPhaseDay">--</b> kPa</td></tr>
                               <tr><td>🌙 <span class="de">Licht aus</span><span class="en">Lights off</span></td><td><b id="vpdPhaseNight">--</b> kPa</td></tr></table>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Blatttemperatur-Offset (°C)</span><span class="en">Leaf temperature offset (°C)</span></div>
                        <input type="number" id="vpdLeafOffset" class="vpd-in" min="-5" max="3" step="0.1">
                        <div class="vpd-hint"><span class="de">Blatt = Luft + Offset, nur bei Licht an. Unter LED meist −1 … −2 °C.</span><span class="en">Leaf = air + offset, lights on only. Under LED usually −1 … −2 °C.</span></div>
                    </div>
                </div>
                <div class="vpd-grid" id="vpdCustomRow" style="display:none;">
                    <div><div class="status-label">☀️ <span class="de">Tag min (kPa)</span><span class="en">Day min (kPa)</span></div><input type="number" id="vpdDayMin" class="vpd-in" min="0.1" max="2.5" step="0.05"></div>
                    <div><div class="status-label">☀️ <span class="de">Tag max (kPa)</span><span class="en">Day max (kPa)</span></div><input type="number" id="vpdDayMax" class="vpd-in" min="0.1" max="2.5" step="0.05"></div>
                    <div><div class="status-label">🌙 <span class="de">Nacht min (kPa)</span><span class="en">Night min (kPa)</span></div><input type="number" id="vpdNightMin" class="vpd-in" min="0.1" max="2.5" step="0.05"></div>
                    <div><div class="status-label">🌙 <span class="de">Nacht max (kPa)</span><span class="en">Night max (kPa)</span></div><input type="number" id="vpdNightMax" class="vpd-in" min="0.1" max="2.5" step="0.05"></div>
                </div>
                <div class="vpd-grid">
                    <div>
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;"><input type="checkbox" id="vpdUseHumid" style="width:18px;height:18px;"><span style="font-size:0.9em;"><span class="de">Regelt den Befeuchter</span><span class="en">Controls the humidifier</span></span></label>
                        <div class="vpd-hint" id="vpdHumidHint"><span class="de">Steckdose/Ausgang wie unter „Luftbefeuchtung“; deren Min/Max ruht dann.</span><span class="en">Socket/output as under "Humidification"; its min/max is then ignored.</span></div>
                    </div>
                    <div>
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;"><input type="checkbox" id="vpdUseFan" style="width:18px;height:18px;"><span style="font-size:0.9em;"><span class="de">Regelt den Abluftlüfter</span><span class="en">Controls the exhaust fan</span></span></label>
                        <div class="vpd-hint" id="vpdFanHint"><span class="de">Zwischen Mindest- und Höchstdrehzahl aus „Abluftlüfter-Steuerung“; deren Modus ruht dann.</span><span class="en">Between min and max speed from "Exhaust Fan Control"; its mode is then ignored.</span></div>
                    </div>
                </div>
                <div class="vpd-grid">
                    <div><div class="status-label"><span class="de">Temperatur min (°C)</span><span class="en">Temperature min (°C)</span></div><input type="number" id="vpdTempMin" class="vpd-in" min="5" max="30" step="1"><div class="vpd-hint"><span class="de">Darunter wird die Abluft für den VPD nicht weiter erhöht, sondern langsam zurückgenommen (kühlt sonst nachts aus).</span><span class="en">Below this, exhaust is not increased for VPD but slowly reduced (would otherwise cool the tent at night).</span></div></div>
                    <div><div class="status-label"><span class="de">Temperatur max (°C)</span><span class="en">Temperature max (°C)</span></div><input type="number" id="vpdTempMax" class="vpd-in" min="18" max="40" step="1"><div class="vpd-hint"><span class="de">Ab 1 °C darunter fährt der Lüfter hoch – hat Vorrang.</span><span class="en">From 1 °C below the fan ramps up – takes priority.</span></div></div>
                    <div><div class="status-label"><span class="de">Feuchte max (%)</span><span class="en">Humidity max (%)</span></div><input type="number" id="vpdHumMax" class="vpd-in" min="40" max="95" step="1"><div class="vpd-hint"><span class="de">Darüber Befeuchter aus und Lüfter hoch (Schimmelschutz) – hat Vorrang.</span><span class="en">Above: humidifier off and fan up (mould protection) – takes priority.</span></div></div>
                    <div><div class="status-label"><span class="de">Alarm nach (min)</span><span class="en">Alarm after (min)</span></div><input type="number" id="vpdAlarmMin" class="vpd-in" min="0" max="240" step="5"><div class="vpd-hint"><span class="de">So lange außerhalb des Zielbereichs → Benachrichtigung. 0 = aus.</span><span class="en">Out of range this long → notification. 0 = off.</span></div></div>
                </div>
                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="vpdSave()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="vpdSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
                <div class="vpd-hint" style="margin-top:10px;">
                    <span class="de">Zu feucht → Befeuchter aus, dann Abluft schrittweise hoch (nur wenn die Raumluft trockener ist als die Zeltluft und gerade kein CO₂ dosiert wird). Richtung zu trocken (obere Bandhälfte) → erst Abluft runter bis zur Mindestdrehzahl, dann Befeuchter an (mind. 60 s an/aus). Beim Wechsel Licht an/aus wandert der Zielbereich langsam (max. 0,45 kPa in 30 min).</span>
                    <span class="en">Too humid → humidifier off, then exhaust up step by step (only if the room air is drier than the tent air and no CO₂ is being dosed). Towards too dry (upper half of band) → first exhaust down to minimum speed, then humidifier on (min. 60 s on/off). When the lights switch on/off, the target range moves slowly (max. 0.45 kPa in 30 min).</span>
                </div>
                <script>
                (function () {
                    function L(de, en) { return (typeof currentLang !== 'undefined' && currentLang === 'en') ? en : de; }
                    const $ = id => document.getElementById(id);
                    const PHASE_NAMES = () => [L('Steckling','Clone'), L('Wachstum','Vegetative'), L('Blüte früh','Early flower'), L('Blüte spät','Late flower'), L('Eigene Werte','Custom')];
                    let cfgLoaded = false, phases = null, dirty = false;
                    const k = v => (v / 100).toFixed(2);

                    function showPhase() {
                        const p = +$('vpdPhase').value;
                        $('vpdCustomRow').style.display = p === 4 ? '' : 'none';
                        let a;
                        if (p === 4) a = [$('vpdDayMin').value * 100, $('vpdDayMax').value * 100, $('vpdNightMin').value * 100, $('vpdNightMax').value * 100];
                        else if (phases) a = phases[p];
                        if (a) { $('vpdPhaseDay').textContent = k(a[0]) + ' – ' + k(a[1]); $('vpdPhaseNight').textContent = k(a[2]) + ' – ' + k(a[3]); }
                    }
                    ['vpdPhase', 'vpdDayMin', 'vpdDayMax', 'vpdNightMin', 'vpdNightMax'].forEach(id => $(id).addEventListener('input', () => { dirty = true; showPhase(); }));
                    document.querySelectorAll('#klimaTab input[id^="vpd"], #klimaTab select[id^="vpd"]').forEach(el => el.addEventListener('change', () => { dirty = true; }));

                    function fillForm(d) {
                        $('vpdEnabled').checked = d.enabled;
                        $('vpdPhase').value     = d.phase;
                        $('vpdDayMin').value    = k(d.day_min);   $('vpdDayMax').value   = k(d.day_max);
                        $('vpdNightMin').value  = k(d.night_min); $('vpdNightMax').value = k(d.night_max);
                        $('vpdLeafOffset').value = (d.leaf_offset / 10).toFixed(1);
                        $('vpdUseHumid').checked = d.use_humid;
                        $('vpdUseFan').checked   = d.use_fan;
                        $('vpdTempMax').value    = d.temp_max;
                        $('vpdHumMax').value     = d.hum_max;
                        $('vpdAlarmMin').value   = d.alarm_min;
                        $('vpdTempMin').value    = d.temp_min;
                        showPhase();
                    }

                    function showState(d) {
                        const R = d.reasons || 0;
                        // Was die VPD-Regelung fuehrt, wird in Luftbefeuchtung/Abluftluefter ausgeblendet
                        // (gespeicherter Stand, nicht die noch ungespeicherten Haken oben)
                        const vh = !!(d.enabled && d.use_humid), vf = !!(d.enabled && d.use_fan);
                        if ($('humidLimitsSection')) { $('humidLimitsSection').style.display = vh ? 'none' : 'grid'; $('humidVpdNote').style.display = vh ? '' : 'none'; }
                        if ($('fanModeWrap'))        { $('fanModeWrap').style.display        = vf ? 'none' : '';     $('fanVpdNote').style.display   = vf ? '' : 'none'; }
                        const badge = $('vpdBadge');
                        $('vpdLiveWrap').style.opacity = d.enabled ? '1' : '0.45';
                        if (!d.enabled)          { badge.textContent = L('aus', 'off'); badge.style.background = '#6c757d'; }
                        else if (!d.valid)       { badge.textContent = L('keine Messwerte', 'no readings'); badge.style.background = '#dc3545'; }
                        else if (R & 0x0002)     { badge.textContent = L('im Zielbereich', 'on target'); badge.style.background = '#28a745'; }
                        else if (R & 0x0004)     { badge.textContent = L('zu feucht', 'too humid'); badge.style.background = '#2e86de'; }
                        else                     { badge.textContent = L('zu trocken', 'too dry'); badge.style.background = '#e67e22'; }
                        const f2 = v => (+v).toFixed(2);   // JSON verliert die Nullen am Ende (0.90 -> 0.9)
                        $('vpdLeaf').textContent = d.valid ? f2(d.vpd_leaf) : '--';
                        $('vpdAir').textContent  = d.valid ? f2(d.vpd_air) : '--';
                        $('vpdSoll').textContent = d.valid ? f2(d.soll_min) + ' – ' + f2(d.soll_max) : '--';
                        $('vpdPhaseNow').textContent = '(' + PHASE_NAMES()[d.phase] + ', ' + (d.light_on ? '☀️' : '🌙') + ')';
                        const hum = d.use_humid ? (d.humid_want ? '💧' + L('an', 'on') : d.humid_demand ? '💧' + L('Takt', 'pulse') : '💧' + L('aus', 'off')) : '💧–';
                        const fan = d.use_fan ? (d.fan_pct >= 0 ? '🌀' + d.fan_pct + ' %' : '🌀' + L('hält', 'holds')) : '🌀–';
                        $('vpdOut').textContent = d.enabled ? hum + '  ' + fan : '--';
                        // Skala 0..2 kPa
                        const pos = v => Math.max(0, Math.min(100, v / 2 * 100));
                        if (d.valid) {
                            $('vpdGaugeBand').style.left  = pos(+d.soll_min) + '%';
                            $('vpdGaugeBand').style.width = (pos(+d.soll_max) - pos(+d.soll_min)) + '%';
                            $('vpdGaugeMark').style.left  = 'calc(' + pos(+d.vpd_leaf) + '% - 1px)';
                            $('vpdGaugeMark').style.display = '';
                        } else $('vpdGaugeMark').style.display = 'none';
                        const lines = [];
                        if (d.enabled && d.valid) {
                            if (R & 0x0200) lines.push('🌗 ' + L('Zielbereich wandert gerade (Licht an/aus)', 'Target range is moving (lights on/off)'));
                            if (R & 0x0010) lines.push('🌡️ ' + L('Temperaturgrenze erreicht – Lüfter hat Vorrang', 'Temperature limit reached – fan takes priority'));
                            if (R & 0x0020) lines.push('💦 ' + L('Feuchtegrenze erreicht – Befeuchter aus, Lüfter hoch', 'Humidity limit reached – humidifier off, fan up'));
                            if (R & 0x0040) lines.push('🏠 ' + L('Raumluft ist nicht trockener als die Zeltluft – mehr Abluft hilft nicht', 'Room air is not drier than tent air – more exhaust won’t help'));
                            if (R & 0x0400) lines.push('🥶 ' + L('Zelt an der Mindesttemperatur – Abluft wird nicht erhöht', 'Tent at minimum temperature – exhaust is not increased'));
                            if (R & 0x0100) lines.push('🫧 ' + L('CO₂ wird dosiert – Lüfter wird nicht hochgefahren', 'CO₂ is being dosed – fan is not increased'));
                            if (R & 0x0080) lines.push('⚠️ ' + L('Lüfter am Anschlag – Raumluft reicht nicht', 'Fan at maximum – room air is not enough'));
                            if (d.ah_tent) lines.push('<span style="color:#999;">' + L('Absolute Feuchte Zelt ', 'Absolute humidity tent ') + d.ah_tent + ' g/m³' + (d.ah_room ? ' · ' + L('Raum ', 'room ') + d.ah_room + ' g/m³' : '') + '</span>');
                        }
                        if (d.enabled && d.use_humid && !d.humid_dev) lines.push('⚠️ ' + L('„Luftbefeuchtung“ ist nicht aktiv – der Befeuchter wird nicht geschaltet', '"Humidification" is not active – the humidifier is not switched'));
                        $('vpdReasons').innerHTML = lines.join('<br>');
                    }

                    function poll() {
                        const tab = $('klimaTab');
                        if (!tab || !tab.classList.contains('active')) return;
                        fetch('/api/vpd').then(r => r.json()).then(d => {
                            phases = d.phases;
                            if (!cfgLoaded || !dirty) { fillForm(d); cfgLoaded = true; }
                            showState(d);
                        }).catch(() => {});
                    }
                    setInterval(poll, 4000);
                    setTimeout(poll, 500);
                    document.querySelectorAll('.tab').forEach(b => b.addEventListener('click', () => setTimeout(poll, 50)));

                    window.vpdSave = function () {
                        const n = id => parseFloat($(id).value);
                        const body = {
                            enabled: $('vpdEnabled').checked, phase: +$('vpdPhase').value,
                            day_min: Math.round(n('vpdDayMin') * 100), day_max: Math.round(n('vpdDayMax') * 100),
                            night_min: Math.round(n('vpdNightMin') * 100), night_max: Math.round(n('vpdNightMax') * 100),
                            leaf_offset: Math.round(n('vpdLeafOffset') * 10),
                            use_humid: $('vpdUseHumid').checked, use_fan: $('vpdUseFan').checked,
                            temp_max: Math.round(n('vpdTempMax')), hum_max: Math.round(n('vpdHumMax')), alarm_min: Math.round(n('vpdAlarmMin')),
                            temp_min: Math.round(n('vpdTempMin'))
                        };
                        const st = $('vpdSaveStatus');
                        fetch('/api/vpd/save', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
                            .then(r => r.json()).then(d => {
                                st.style.color = d.success ? '#27ae60' : '#dc3545';
                                st.textContent = d.success ? L('Gespeichert', 'Saved') : L('Fehler', 'Error');
                                if (d.success) { dirty = false; setTimeout(() => st.textContent = '', 2000); poll(); }
                            }).catch(() => { st.style.color = '#dc3545'; st.textContent = L('Verbindungsfehler', 'Connection error'); });
                    };
                })();
                </script>
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

                <div id="humidVpdNote" style="display:none;font-size:0.85em;color:#155724;background:#d4edda;border-radius:5px;padding:8px 12px;margin-bottom:16px;"><span class="de">Feuchte Min/Max entfällt – der Befeuchter wird von der VPD-Regelung geschaltet. Hier gelten nur Steckdose und Ausgang.</span><span class="en">Humidity min/max not used – the humidifier is switched by the VPD control. Only socket and output apply here.</span></div>
                <div id="humidLimitsSection" style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
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

                <div id="fanVpdNote" style="display:none;font-size:0.85em;color:#155724;background:#d4edda;border-radius:5px;padding:8px 12px;margin-bottom:16px;"><span class="de">Modus entfällt – der Lüfter wird von der VPD-Regelung geführt (inkl. deren Temperatur- und Feuchtegrenzen). Hier gelten nur Mindest- und Höchstdrehzahl.</span><span class="en">Mode not used – the fan is driven by the VPD control (including its temperature and humidity limits). Only min and max speed apply here.</span></div>
                <div id="fanModeWrap">
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
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: Höchstdrehzahl</span><span class="en">Above: maximum speed</span></div>
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
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber: Höchstdrehzahl</span><span class="en">Above: maximum speed</span></div>
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
                </div><!-- /fanModeWrap -->

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Mindestdrehzahl (%)</span><span class="en">Minimum Speed (%)</span></div>
                        <input type="number" id="fanMinSpeed" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Höchstdrehzahl (%)</span><span class="en">Maximum Speed (%)</span></div>
                        <input type="number" id="fanMaxSpeed" min="0" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div style="grid-column:1/-1;font-size:0.78em;color:#888;margin-top:-6px;"><span class="de">Gilt für beide Automatik-Modi und die VPD-Regelung. MARS-Lüfter: unter 25 % läuft er im 10-min-Takt (z. B. 5 % = 2 von 10 min mit Minimum) — die Mindestdrehzahl ist damit die Grundlüftung, 0 % = darf ganz aus.</span><span class="en">Applies to both auto modes and VPD control. MARS fan: below 25 % it runs in a 10-min cycle (e.g. 5 % = 2 of 10 min at minimum) — the minimum speed is thus the base ventilation, 0 % = may switch off.</span></div>
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
