R"rawliteral(
        <!-- ============================================================ -->
        <!-- STARTSEITE — reine Information, read-only                    -->
        <!-- ============================================================ -->
        <div id="startseiteTab" class="tab-content active">
            <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:15px;">

                <!-- Messwerte (alles außer PPFD) -->
                <div class="status-box-info">
                    <strong>📊 <span class="de">Werte Aeroponik</span><span class="en">Aeroponic Readings</span></strong>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="tempVorrat">--</div><div class="sensor-unit">°C <span class="de">Vorrat</span><span class="en">Reservoir</span></div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="tempPflanze">--</div><div class="sensor-unit">°C <span class="de">Pflanze</span><span class="en">Plant</span></div></div>
                        <div class="sensor-box" id="druckSensorBox" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="druckBar">--</div><div class="sensor-unit">bar <span class="de">Druck</span><span class="en">Pressure</span></div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="volumenLiter">--</div><div class="sensor-unit">L <span class="de">Füllstand</span><span class="en">Fill Level</span></div></div>
                    </div>
                    <div id="messwerteExtra"></div>
                </div>

                <!-- Ventile-Status: Box nur sichtbar wenn mind. ein Behälter aktiv ist -->
                <div class="status-box-info" id="behaelterStatusCard" style="display:none;">
                    <strong>🚿 <span class="de">Behälter</span><span class="en">Reservoirs</span></strong>
                    <div id="behaelterStatusCards"></div>
                </div>

                <!-- Pumpe & Umwälzpumpe (beide automatisch) -->
                <div class="status-box-info">
                    <strong>💧 <span class="de">Pumpe &amp; Umwälzpumpe</span><span class="en">Pump &amp; Circulation Pump</span></strong>
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-top:10px;">
                        <span style="font-size:0.9em;"><span class="de">Pumpe</span><span class="en">Pump</span></span>
                        <span id="pumpeBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-top:8px;">
                        <span style="font-size:0.9em;"><span class="de">Umwälzpumpe</span><span class="en">Circulation pump</span></span>
                        <span id="umwaelzBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <!-- Gehäuselüfter: nur An/Aus (Temperatur bewusst nicht auf der Startseite) -->
                    <div id="gehaeuseFanRow" style="display:none;justify-content:space-between;align-items:center;margin-top:8px;padding-top:8px;border-top:1px solid #eee;">
                        <span style="font-size:0.9em;">🌀 <span class="de">Gehäuselüfter</span><span class="en">Enclosure Fan</span></span>
                        <span id="gehaeuseFanBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                </div>

                <!-- Licht (PPFD) & Zeitplan -->
                <div class="status-box-info">
                    <strong>☀ <span class="de">Licht (PPFD) &amp; Zeitplan</span><span class="en">Light (PPFD) &amp; Schedule</span></strong>
                    <div id="lichtsensorData" style="display:none;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="lichtPpfd">--</div><div class="sensor-unit">µmol/m²/s PPFD</div></div>
                        <div id="lichtSatBox" style="display:none;margin-top:8px;padding:6px;background:#fde8e8;border-radius:4px;text-align:center;color:#e74c3c;font-weight:bold;font-size:0.85em;"><span class="de">SAT! Sensor gesättigt</span><span class="en">SAT! Sensor saturated</span></div>
                        <div id="lichtMissingBox" style="display:none;margin-top:8px;padding:6px;background:#fde8e8;border-radius:4px;text-align:center;color:#e74c3c;font-weight:bold;font-size:0.85em;">⚠ <span class="de">Lichtsensor fehlt/defekt</span><span class="en">Light sensor missing/faulty</span></div>
                        <canvas id="spektrumChart" width="320" height="110"
                            style="margin-top:10px;border-radius:6px;display:block;max-width:100%;background:#111;"></canvas>
                        <div style="text-align:right;font-size:0.75em;color:#aaa;margin-top:4px;">
                            <span class="de">Zuletzt:</span><span class="en">Last:</span> <span id="lichtAge">--</span>
                        </div>
                    </div>
                    <div id="schedPhaseRow" style="display:flex;justify-content:space-between;align-items:center;margin-top:12px;">
                        <span style="font-size:0.85em;color:#6c757d;"><span class="de">Zeitplan</span><span class="en">Schedule</span></span>
                        <div id="schedPhase" style="padding:2px 10px;border-radius:10px;background:#f0f0f0;font-size:0.8em;">—</div>
                    </div>
                    <canvas id="schedChart" width="800" height="130"
                        style="width:100%;border-radius:6px;margin-top:6px;"></canvas>

                    <!-- Nur sichtbar wenn der Zeitplan deaktiviert ist: manuelle Helligkeit
                         statt der Zeitrampe, analog zur manuellen Luefter-Leistung -->
                    <div id="schedManualRow" style="display:none;margin-top:12px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                            <span class="de">Zeitplan deaktiviert — manuelle Helligkeit</span><span class="en">Schedule disabled — manual brightness</span>
                            <span id="schedManualValue">--%</span>
                        </div>
                        <input type="range" id="schedManualSlider" min="0" max="100" value="0" style="width:100%;"
                            oninput="onSchedManualSliderInput()" onchange="onSchedManualSliderChange()">
                    </div>
                </div>

                <!-- CO2-Steuerung + Abluftluefter, gemeinsame Box (Nutzerwunsch), Abschnitte
                     je nach Bedarf einzeln sichtbar (CO2 nur wenn aktiv, Luefter nur wenn der
                     aktive Ausgang online ist) -->
                <div class="status-box-info" id="co2FanStatusCard" style="display:none;">
                    <div id="co2StatusSection" style="display:none;">
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <strong>🌫️ CO2-<span class="de">Steuerung</span><span class="en">Control</span></strong>
                            <span id="co2OutputBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                        </div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                            <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="co2StatusNow">--</div><div class="sensor-unit">ppm CO2</div></div>
                            <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="co2StatusRange">--</div><div class="sensor-unit">Min / Max</div></div>
                        </div>
                    </div>

                    <div id="fanStatusSection" style="display:none;margin-top:14px;padding-top:14px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <strong>🌀 <span class="de">Abluftlüfter</span><span class="en">Exhaust Fan</span></strong>
                            <span id="fanModeBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                        </div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                            <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="fanPercent">--</div><div class="sensor-unit">% <span class="de">Leistung</span><span class="en">Power</span></div></div>
                            <div class="sensor-box" style="padding:8px;" id="fanRpmBox"><div class="sensor-value" style="font-size:1.4em;" id="fanRpm">--</div><div class="sensor-unit">RPM</div></div>
                        </div>
                        <div id="fanManualSliderRow" style="display:none;margin-top:12px;">
                            <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.85em;color:#6c757d;margin-bottom:4px;">
                                <span class="de">Manuelle Leistung</span><span class="en">Manual power</span>
                                <span id="fanManualSliderValue">--%</span>
                            </div>
                            <input type="range" id="fanManualSlider" min="0" max="100" value="0" style="width:100%;"
                                oninput="onFanManualSliderInput()" onchange="onFanManualSliderChange()">
                        </div>
                    </div>
                </div>

                <!-- Luftbefeuchtung-Steuerung — eigene Statuskarte, gleiches Muster wie CO2 -->
                <div class="status-box-info" id="humidStatusCard" style="display:none;">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>💧 <span class="de">Luftbefeuchtung</span><span class="en">Humidification</span></strong>
                        <span id="humidOutputBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="humidStatusNow">--</div><div class="sensor-unit">% <span class="de">Feuchte</span><span class="en">Humidity</span></div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="humidStatusRange">--</div><div class="sensor-unit">Min / Max</div></div>
                    </div>
                </div>

                <!-- DWC-Beleuchtungstimer (separates System, eigener Steckdosen-Ausgang) -->
                <div class="status-box-info" id="dwcStatusCard" style="display:none;">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>💡 DWC-<span class="de">Beleuchtung</span><span class="en">Lighting</span></strong>
                        <span id="dwcOutputBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <div style="font-size:0.85em;color:#6c757d;margin-top:8px;" id="dwcTimerInfo">--</div>
                </div>

                <!-- Raumlicht: manueller Schalter, nur sichtbar sobald im Klima-Tab ein Ausgang
                     ausgewaehlt wurde (siehe updateRaumlichtStatus()). -->
                <div class="status-box-info" id="raumlichtStatusCard" style="display:none;">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>💡 <span class="de">Raumlicht</span><span class="en">Room Light</span></strong>
                        <span id="raumlichtOfflineBadge" style="display:none;background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;"><span class="de">Offline</span><span class="en">Offline</span></span>
                    </div>
                    <div style="margin-top:12px;">
                        <button id="raumlichtToggleBtn" onclick="toggleRaumlicht()" class="button" style="width:100%;padding:12px;font-size:1em;border:none;border-radius:6px;cursor:pointer;">--</button>
                    </div>
                </div>
            </div>
        </div>
)rawliteral"
