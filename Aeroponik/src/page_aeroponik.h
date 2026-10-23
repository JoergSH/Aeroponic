R"rawliteral(
        <!-- ============================================================ -->
        <!-- AEROPONIK — Tank, Ventile, Düsen-Überwachung, Umwälzpumpe      -->
        <!-- ============================================================ -->
        <div id="aeroponikTab" class="tab-content">

            <!-- Tank-Konfiguration -->
            <h3 class="section-heading">🪣 <span class="de">Tank-Konfiguration</span><span class="en">Tank Configuration</span></h3>
            <div id="tankConfigSection">
                <h4><span class="de">Füllstandssensor</span><span class="en">Level Sensor</span></h4>
                <div style="margin-bottom:20px;">
                    <div class="status-label"><span class="de">Sensor-Typ</span><span class="en">Sensor Type</span></div>
                    <select id="tankSensorTyp" onchange="toggleTankSensorUI()" style="width:100%;max-width:300px;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <option value="0">Ultraschall (HC-SR04/JSN-SR04T)</option>
                        <option value="1">Drucksensor (HX710B)</option>
                    </select>
                    <div style="font-size:0.78em;color:#d9822b;margin-top:3px;">
                        <span class="de">⚠️ Nach Wechsel + Speichern ist ein Neustart nötig (Reiter System → Neustart).</span>
                        <span class="en">⚠️ Requires a restart after changing + saving (System tab → Restart).</span>
                    </div>
                </div>

                <h4><span class="de">Tank Geometrie</span><span class="en">Tank Geometry</span></h4>
                <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;margin-bottom:24px;">
                    <div>
                        <div class="status-label"><span class="de">Tankhöhe (mm)</span><span class="en">Tank Height (mm)</span></div>
                        <input type="number" id="tankHoehe" min="50" max="2000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Innenhöhe des Tanks</span><span class="en">Inner height of the tank</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Radius unten (mm)</span><span class="en">Radius Bottom (mm)</span></div>
                        <input type="number" id="tankRadiusUnten" min="10" max="1000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Halbmesser am Boden</span><span class="en">Radius at the bottom</span></div>
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Radius oben (mm)</span><span class="en">Radius Top (mm)</span></div>
                        <input type="number" id="tankRadiusOben" min="10" max="1000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Halbmesser am Rand</span><span class="en">Radius at the rim</span></div>
                    </div>
                </div>

                <div id="ultraschallKalibrierung">
                    <h4><span class="de">Sensor Kalibrierung — Ultraschall</span><span class="en">Sensor Calibration — Ultrasonic</span></h4>
                    <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;margin-bottom:24px;">
                        <div>
                            <div class="status-label"><span class="de">Leer-Abstand (mm)</span><span class="en">Empty Distance (mm)</span></div>
                            <input type="number" id="sensorLeer" min="50" max="5000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Sensor → Boden bei leerem Tank</span><span class="en">Sensor → bottom when tank is empty</span></div>
                        </div>
                        <div>
                            <div class="status-label"><span class="de">Min-Reichweite (mm)</span><span class="en">Min Range (mm)</span></div>
                            <input type="number" id="sensorMin" min="20" max="500" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darunter = Tank voll (100%)</span><span class="en">Below = tank full (100%)</span></div>
                        </div>
                        <div>
                            <div class="status-label"><span class="de">Max-Reichweite (mm)</span><span class="en">Max Range (mm)</span></div>
                            <input type="number" id="sensorMax" min="100" max="10000" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Darüber = Messfehler, ignorieren</span><span class="en">Above = measurement error, ignore</span></div>
                        </div>
                        <div>
                            <div class="status-label"><span class="de">Offset (mm)</span><span class="en">Offset (mm)</span></div>
                            <input type="number" id="sensorOffset" min="-200" max="200" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Korrektur (+ = weiter, - = näher)</span><span class="en">Correction (+ = farther, - = closer)</span></div>
                        </div>
                    </div>

                    <div style="background:#e8f4fd;border-radius:8px;padding:12px;margin-bottom:20px;font-size:0.88em;border-left:4px solid #17a2b8;">
                        <strong><span class="de">Tipp:</span><span class="en">Tip:</span></strong>
                        <span class="de">Leer-Abstand = Min-Reichweite + Tankhöhe (wenn Sensor direkt am Tankrand sitzt).</span>
                        <span class="en">Empty distance = min range + tank height (if the sensor sits directly at the tank rim).</span><br>
                        <span class="de">Aktuell gemessener Sensorabstand:</span><span class="en">Currently measured sensor distance:</span> <strong id="tankSensorLive">-- mm</strong>
                    </div>
                </div>

                <div id="druckKalibrierung" hidden>
                    <h4><span class="de">Sensor Kalibrierung — Drucksensor</span><span class="en">Sensor Calibration — Pressure</span></h4>
                    <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;margin-bottom:24px;">
                        <div>
                            <div class="status-label"><span class="de">Null-Rohwert (leer)</span><span class="en">Zero Raw Value (empty)</span></div>
                            <input type="number" id="druckNull" step="1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Rohwert bei leerem Tank (0mm)</span><span class="en">Raw value with tank empty (0mm)</span></div>
                        </div>
                        <div>
                            <div class="status-label"><span class="de">Rohwert pro mm</span><span class="en">Raw Value per mm</span></div>
                            <input type="number" id="druckProMm" step="0.001" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            <div style="font-size:0.78em;color:#888;margin-top:3px;"><span class="de">Änderung des Rohwerts pro mm Wasserhöhe</span><span class="en">Raw value change per mm of water height</span></div>
                        </div>
                    </div>

                    <div style="background:#e8f4fd;border-radius:8px;padding:12px;margin-bottom:20px;font-size:0.88em;border-left:4px solid #17a2b8;">
                        <strong><span class="de">Tipp:</span><span class="en">Tip:</span></strong>
                        <span class="de">Bei leerem Tank "Null-Rohwert" auf den unten angezeigten Live-Rohwert setzen. Danach mit bekannter Menge auffüllen und "Rohwert pro mm" = (neuer Rohwert − Null-Rohwert) / Wasserhöhe(mm) berechnen.</span>
                        <span class="en">With the tank empty, set "Zero Raw Value" to the live raw value shown below. Then fill with a known amount and compute "Raw Value per mm" = (new raw value − zero raw value) / water height(mm).</span><br>
                        <span class="de">Aktuell gemessener Roh-Druckwert:</span><span class="en">Currently measured raw pressure value:</span> <strong id="tankDruckLive">-- </strong>
                    </div>
                </div>

                <div style="text-align:center;">
                    <button class="button button-on" onclick="saveTankKonfig()">💾 <span class="de">Speichern</span><span class="en">Save</span></button>
                </div>
            </div>

            <!-- Ventile-Konfiguration -->
            <h3 class="section-heading">🚿 <span class="de">Ventile-Konfiguration</span><span class="en">Valve Configuration</span></h3>
            <div style="margin-bottom:15px; padding:12px; background:#f0f8ff; border-radius:8px; border-left:4px solid #17a2b8;">
                <label><input type="checkbox" id="vorlaufAktiv" onchange="saveVentilConfig()"> <span class="de">Vorlauf Rückleitung aktiv</span><span class="en">Return pre-flush active</span></label>
                &nbsp;&nbsp;
                <label><span class="de">Vorlaufzeit</span><span class="en">Pre-flush time</span>: <input type="number" id="vorlaufzeit" min="1" max="30" style="width:55px;" onchange="saveVentilConfig()"> s</label>
            </div>

            <!-- Sprühschema (LPA-Betriebsprotokoll Punkt 2/3): fertige Licht-/Dunkelphase-Profile
                 zum Vorbefüllen aller Zeitfelder — danach normal pruefen/anpassen und speichern. -->
            <div style="margin-bottom:15px; padding:12px; background:#fff8e1; border-radius:8px; border-left:4px solid #ffc107;">
                <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap;">
                    <span style="font-size:0.9em;"><span class="de">Sprühschema übernehmen</span><span class="en">Apply spray schema</span>:</span>
                    <select id="sprayScheme" style="padding:6px;border:1px solid #ddd;border-radius:4px;">
                        <option value="phase1" class="i18n-opt" data-de="Phase 1: Junge Pflanzen" data-en="Phase 1: Young plants">Phase 1: Junge Pflanzen</option>
                        <option value="phase2" class="i18n-opt" data-de="Phase 2: Große Pflanzen" data-en="Phase 2: Large plants">Phase 2: Große Pflanzen</option>
                    </select>
                    <button onclick="applySprayScheme()" style="background:#f39c12;color:white;border:none;padding:7px 16px;border-radius:5px;cursor:pointer;font-size:0.9em;">
                        <span class="de">Übernehmen</span><span class="en">Apply</span></button>
                </div>
                <div style="font-size:0.78em;color:#888;margin-top:6px;">
                    <span class="de">Füllt Licht- und Dunkelphase-Zeiten unten mit den Richtwerten aus dem LPA-Betriebsprotokoll — anschließend prüfen/anpassen und speichern.</span>
                    <span class="en">Fills in the light/dark-phase times below with the reference values from the LPA operating protocol — review/adjust and save afterward.</span>
                </div>
            </div>

            <div style="margin-bottom:15px; padding:12px; background:#f0f8ff; border-radius:8px; border-left:4px solid #17a2b8;">
                <div style="display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px;">
                    <label><input type="checkbox" id="gleicheZeiten" onchange="onGleicheZeitenChange()"> <span class="de">Gleiche Zeiten für alle Behälter</span><span class="en">Same times for all reservoirs</span></label>
                    <span id="lichtStatusBadge" style="padding:2px 10px;border-radius:10px;font-size:0.8em;background:#6c757d;color:white;">--</span>
                </div>
                <div id="gleicheZeitenFields" style="display:none;margin-top:10px;">
                    <div style="font-size:0.82em;color:#888;margin-bottom:4px;"><span class="de">☀ Lichtphase</span><span class="en">☀ Light phase</span></div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;max-width:400px;margin-bottom:10px;">
                        <label style="font-size:0.9em;"><span class="de">Öffnungszeit (s)</span><span class="en">Opening time (s)</span><br>
                            <input type="number" id="oeffnungGemeinsam" min="1" max="60" style="width:100%;padding:4px;" onchange="applyGleicheZeiten()">
                        </label>
                        <label style="font-size:0.9em;"><span class="de">Pausenzeit (min)</span><span class="en">Pause time (min)</span><br>
                            <input type="number" id="pauseGemeinsam" min="1" max="60" style="width:100%;padding:4px;" onchange="applyGleicheZeiten()">
                        </label>
                    </div>
                    <div style="font-size:0.82em;color:#888;margin-bottom:4px;"><span class="de">🌙 Dunkelphase</span><span class="en">🌙 Dark phase</span></div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;max-width:400px;">
                        <label style="font-size:0.9em;"><span class="de">Öffnungszeit (s)</span><span class="en">Opening time (s)</span><br>
                            <input type="number" id="oeffnungGemeinsamDunkel" min="1" max="60" style="width:100%;padding:4px;" onchange="applyGleicheZeiten()">
                        </label>
                        <label style="font-size:0.9em;"><span class="de">Pausenzeit (min)</span><span class="en">Pause time (min)</span><br>
                            <input type="number" id="pauseGemeinsamDunkel" min="1" max="60" style="width:100%;padding:4px;" onchange="applyGleicheZeiten()">
                        </label>
                    </div>
                </div>
                <div style="font-size:0.78em;color:#888;margin-top:6px;">
                    <span class="de">Aktiv laufen alle aktiven Behälter direkt nacheinander in einem Durchlauf — der Rücklauf-Vorlauf öffnet dabei nur einmal vor dem ersten, nicht vor jedem einzelnen Behälter.</span>
                    <span class="en">When active, all active reservoirs run back-to-back in one pass — the return pre-flush only opens once before the first one, not before each individual reservoir.</span>
                </div>
            </div>

            <div id="behaelterCards" style="display:grid; grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); gap:15px;"></div>

            <div style="text-align:center; margin-top:20px;">
                <button class="button button-on" onclick="saveVentilConfig()">💾 <span class="de">Speichern</span><span class="en">Save</span></button>
            </div>

            <div style="margin-top:24px;padding:16px;background:#fff8e1;border-radius:8px;border-left:4px solid #ffc107;">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
                    <h4 style="margin:0;">🔌 <span class="de">Ausgangstest</span><span class="en">Output Test</span></h4>
                    <button id="pcfTestToggle" onclick="togglePcfTest()"
                        style="background:#e74c3c;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;font-size:0.9em;">
                        <span class="de">Test starten</span><span class="en">Start Test</span></button>
                </div>
                <div id="pcfTestWarning" style="display:none;margin-bottom:12px;padding:8px;background:#f8d7da;border-radius:4px;font-size:0.85em;color:#721c24;">
                    ⚠ <span class="de">Automatische Ventil-/Pumpensteuerung pausiert!</span><span class="en">Automatic valve/pump control paused!</span>
                </div>
                <div id="pcfTestPendingInfo" style="display:none;margin-bottom:12px;padding:8px;background:#fff3cd;border-radius:4px;font-size:0.85em;color:#856404;">
                    ⏳ <span class="de">Wird aktiviert, sobald der laufende Gießzyklus beendet ist …</span><span class="en">Activating once the current watering cycle finishes …</span>
                </div>
                <div id="pcfTestButtons" style="display:grid;grid-template-columns:repeat(3,1fr);gap:8px;opacity:0.4;pointer-events:none;">
                    <button id="pcf_btn_0" onclick="pcfToggle(0)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Ventil 1 (Beh. 1)</span><span class="en">Valve 1 (Res. 1)</span></small></button>
                    <button id="pcf_btn_1" onclick="pcfToggle(1)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Ventil 2 (Beh. 2)</span><span class="en">Valve 2 (Res. 2)</span></small></button>
                    <button id="pcf_btn_2" onclick="pcfToggle(2)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Ventil 3 (Beh. 3)</span><span class="en">Valve 3 (Res. 3)</span></small></button>
                    <button id="pcf_btn_3" onclick="pcfToggle(3)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Rücklaufventil</span><span class="en">Return Valve</span></small></button>
                    <button id="pcf_btn_4" onclick="pcfToggle(4)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Umwälzpumpe</span><span class="en">Circulation Pump</span></small></button>
                    <button id="pcf_btn_5" onclick="pcfToggle(5)" class="pcf-btn"><span>AUS</span><br><small><span class="de">Pumpe</span><span class="en">Pump</span></small></button>
                </div>
            </div>

            <!-- Drucksensor (Pumpendruck, GPIO5): nur An/Aus -- Anzeige auf der Startseite entfaellt
                 komplett, solange hier deaktiviert (Sensor evtl. nicht verbaut/defekt). -->
            <div style="margin-top:20px;padding:16px;background:#f0f8ff;border-radius:8px;border-left:4px solid #17a2b8;">
                <div style="display:flex;justify-content:space-between;align-items:center;">
                    <strong>📟 <span class="de">Drucksensor (Pumpendruck)</span><span class="en">Pressure Sensor (Pump)</span></strong>
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="druckSensorEnabled" onchange="saveDruckSensorConfig()" style="width:18px;height:18px;">
                    </label>
                </div>
                <div style="font-size:0.78em;color:#888;margin-top:8px;">
                    <span class="de">Deaktivieren, falls der Sensor nicht verbaut oder defekt ist — die Druckanzeige verschwindet dann von der Startseite.</span>
                    <span class="en">Disable if the sensor isn't installed or is faulty — the pressure card then disappears from the home page.</span>
                </div>
                <span id="druckSensorSaveStatus" style="font-size:0.85em;color:#888;"></span>
            </div>

            <!-- Durchflussmesser-Kalibrierung: Pumpe an -> Druck beobachten -> Start (Ventil 4
                 auf, Pulse zaehlen) -> Stop -> Menge in ml eingeben -> Pulse/Liter berechnen.
                 Nutzt denselben Ausgangs-Direkttest wie oben (Pumpe=Index 5, Ventil 4=Index 3). -->
            <div style="margin-top:20px;padding:16px;background:#e8f4fd;border-radius:8px;border-left:4px solid #17a2b8;">
                <h4 style="margin:0 0 12px 0;">💧 <span class="de">Durchflussmesser-Kalibrierung</span><span class="en">Flow Sensor Calibration</span></h4>

                <div id="flowcalCurrent" style="display:none;margin-bottom:10px;font-size:0.85em;color:#555;">
                    <span class="de">Aktuelle Kalibrierung:</span><span class="en">Current calibration:</span>
                    <strong id="flowcalCurrentValue">--</strong> Pulse/L
                </div>

                <div id="flowcalIdle">
                    <button onclick="flowcalStart()" style="background:#17a2b8;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                        <span class="de">Kalibrierung starten (Pumpe an)</span><span class="en">Start calibration (pump on)</span>
                    </button>
                </div>

                <div id="flowcalRunning" style="display:none;">
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px;max-width:300px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.3em;" id="flowcalDruck">--</div><div class="sensor-unit">bar</div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.3em;" id="flowcalPulses">--</div><div class="sensor-unit"><span class="de">Impulse</span><span class="en">Pulses</span></div></div>
                    </div>

                    <!-- Entlueften: Ventil auf/zu bevor die eigentliche Messung startet, damit Luft
                         im System nicht die Pulszahl verfaelscht. Start bleibt gesperrt, bis dieser
                         Zyklus einmal abgeschlossen (= Ventil wieder zu) wurde. -->
                    <div style="margin-bottom:10px;">
                        <button id="flowcalPurgeBtn" onclick="flowcalPurge()" style="background:#f39c12;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                            <span class="de">Entlüften (Ventil öffnen)</span><span class="en">Purge air (open valve)</span>
                        </button>
                        <div style="font-size:0.78em;color:#888;margin-top:4px;">
                            <span class="de">Vor dem Start: Ventil öffnen, bis evtl. Luft aus dem System raus ist, dann hier wieder stoppen.</span>
                            <span class="en">Before starting: open the valve until any air is out of the system, then stop here again.</span>
                        </div>
                    </div>

                    <div style="display:flex;gap:8px;">
                        <button id="flowcalBeginBtn" onclick="flowcalBegin()" disabled style="background:#28a745;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;opacity:0.4;pointer-events:none;">
                            <span class="de">Start (Ventil öffnen)</span><span class="en">Start (open valve)</span>
                        </button>
                        <button id="flowcalStopBtn" style="display:none;background:#dc3545;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;" onclick="flowcalStop()">
                            <span class="de">Stop</span><span class="en">Stop</span>
                        </button>
                        <button onclick="flowcalCancel()" style="background:#6c757d;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                            <span class="de">Abbrechen</span><span class="en">Cancel</span>
                        </button>
                    </div>
                </div>

                <div id="flowcalVolumeInput" style="display:none;">
                    <div style="font-size:0.9em;margin-bottom:8px;">
                        <span class="de">Gezählte Impulse:</span><span class="en">Counted pulses:</span>
                        <strong id="flowcalPulsesResult">--</strong>
                    </div>
                    <div style="font-size:0.85em;color:#555;margin-bottom:8px;">
                        <span class="de">Jetzt die aufgefangene Menge abmessen und hier eintragen:</span>
                        <span class="en">Now measure the collected amount and enter it here:</span>
                    </div>
                    <div style="display:flex;gap:8px;align-items:center;">
                        <input type="number" id="flowcalMl" placeholder="ml" style="width:120px;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        <button onclick="flowcalCalc()" style="background:#28a745;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                            <span class="de">Berechnen</span><span class="en">Calculate</span></button>
                        <button onclick="flowcalCancel()" style="background:#6c757d;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;">
                            <span class="de">Abbrechen</span><span class="en">Cancel</span></button>
                    </div>
                </div>

                <div id="flowcalResult" style="display:none;margin-top:10px;padding:8px;background:#d4edda;border-radius:4px;font-size:0.9em;color:#155724;">
                    <span class="de">Kalibriert:</span><span class="en">Calibrated:</span> <strong id="flowcalResultValue">--</strong> Pulse/L
                </div>
            </div>

            <!-- Duesen-Ueberwachung: misst pro Giesszyklus die verspruehte Menge (ueber den
                 kalibrierten Durchflussmesser) und vergleicht sie optional gegen einen erwarteten
                 Durchfluss je Behaelter -- bei zu grosser Abweichung (verstopfte Duese oder Leck)
                 geht eine Benachrichtigung ueber die konfigurierten Kanaele raus. -->
            <div style="margin-top:20px;padding:16px;background:#fff3e0;border-radius:8px;border-left:4px solid #fb8c00;">
                <h4 style="margin:0 0 12px 0;">🚿 <span class="de">Düsen-Überwachung</span><span class="en">Nozzle Monitoring</span></h4>

                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Erfasst die verspritzte Menge je Behälter automatisch bei jedem Gießzyklus (braucht die Kalibrierung oben). Soll/Ist-Vergleich + Alarm sind optional.</span>
                    <span class="en">Automatically measures the sprayed amount per reservoir on every watering cycle (needs the calibration above). Target/actual comparison + alert are optional.</span>
                </div>

                <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;margin-bottom:16px;">
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonLast1">--</div><div class="sensor-unit">ml <span class="de">Beh. 1 (letzter Zyklus)</span><span class="en">Res. 1 (last cycle)</span></div></div>
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonLast2">--</div><div class="sensor-unit">ml <span class="de">Beh. 2 (letzter Zyklus)</span><span class="en">Res. 2 (last cycle)</span></div></div>
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonLast3">--</div><div class="sensor-unit">ml <span class="de">Beh. 3 (letzter Zyklus)</span><span class="en">Res. 3 (last cycle)</span></div></div>
                </div>
                <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;margin-bottom:16px;">
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonTotal1">--</div><div class="sensor-unit">ml <span class="de">Beh. 1 gesamt</span><span class="en">Res. 1 total</span></div></div>
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonTotal2">--</div><div class="sensor-unit">ml <span class="de">Beh. 2 gesamt</span><span class="en">Res. 2 total</span></div></div>
                    <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonTotal3">--</div><div class="sensor-unit">ml <span class="de">Beh. 3 gesamt</span><span class="en">Res. 3 total</span></div></div>
                    <div class="sensor-box" style="padding:8px;background:#ffe0b2;"><div class="sensor-value" style="font-size:1.2em;" id="flowmonTotalAll">--</div><div class="sensor-unit">ml <span class="de">Gesamt (seit Neustart)</span><span class="en">Total (since restart)</span></div></div>
                </div>

                <div style="display:flex;align-items:center;gap:8px;margin-bottom:12px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <input type="checkbox" id="flowmonEnabled" style="width:18px;height:18px;">
                        <span class="de">Soll/Ist-Vergleich + Alarm aktiv</span><span class="en">Target/actual comparison + alert active</span>
                    </label>
                </div>

                <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;margin-bottom:12px;">
                    <div>
                        <div class="status-label"><span class="de">Erwartet Beh. 1 (ml/s)</span><span class="en">Expected Res. 1 (ml/s)</span></div>
                        <input type="number" id="flowmonSoll1" min="0" step="0.1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Erwartet Beh. 2 (ml/s)</span><span class="en">Expected Res. 2 (ml/s)</span></div>
                        <input type="number" id="flowmonSoll2" min="0" step="0.1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Erwartet Beh. 3 (ml/s)</span><span class="en">Expected Res. 3 (ml/s)</span></div>
                        <input type="number" id="flowmonSoll3" min="0" step="0.1" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Toleranz (%)</span><span class="en">Tolerance (%)</span></div>
                        <input type="number" id="flowmonToleranz" min="1" max="100" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                </div>
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Steht für einen Behälter noch kein Sollwert (0), wird er automatisch vom nächsten Gießzyklus übernommen — manuelles Eintragen ist nur zum Überschreiben nötig.</span>
                    <span class="en">If a reservoir has no target yet (0), it is learned automatically from the next watering cycle — manual entry is only needed to override it.</span>
                </div>

                <button onclick="saveFlowMonConfig()" style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                    <span class="de">Speichern</span><span class="en">Save</span></button>
                <button onclick="resetFlowMonSollwerte()" style="background:#6c757d;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                    <span class="de">Sollwerte löschen</span><span class="en">Clear targets</span></button>
                <span id="flowmonSaveStatus" style="font-size:0.85em;color:#888;margin-left:8px;"></span>
            </div>

            <!-- Umwälzpumpe-Steuerung (Vorratsbehälter-Zirkulation) -->
            <div style="margin-top:24px;padding:16px;background:#f0f8ff;border-radius:8px;border-left:4px solid #17a2b8;">
                <h4 style="margin:0 0 8px 0;">🔄 <span class="de">Umwälzpumpe</span><span class="en">Circulation Pump</span></h4>
                <div style="font-size:0.78em;color:#888;margin-bottom:12px;">
                    <span class="de">Zirkulationspumpe im Vorratsbehälter (GPIO47), läuft im eingestellten Takt, sobald mindestens ein Behälter aktiviert ist — unabhängig davon, ob gerade tatsächlich gewässert wird. Ist kein Behälter aktiviert, bleibt sie aus.</span>
                    <span class="en">Circulation pump in the reservoir tank (GPIO47), runs on the configured cycle whenever at least one reservoir is enabled — regardless of whether it's currently watering. Stays off if no reservoir is enabled.</span>
                </div>
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="umwEnabled" style="width:18px;height:18px;">
                    </label>
                </div>
                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
                    <div>
                        <div class="status-label"><span class="de">Laufzeit (s)</span><span class="en">Run time (s)</span></div>
                        <input type="number" id="umwLaufzeit" min="1" max="255" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                    <div>
                        <div class="status-label"><span class="de">Pause (min)</span><span class="en">Pause (min)</span></div>
                        <input type="number" id="umwPause" min="1" max="255" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                    </div>
                </div>
                <div style="display:flex;gap:8px;align-items:center;">
                    <button onclick="saveUmwaelzForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="umwSaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>

        </div>
)rawliteral"
