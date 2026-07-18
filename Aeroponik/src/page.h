// page.h - Web Interface für ESP32 Aeroponik
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aeroponik Steuerung</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            background-color: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            margin-bottom: 20px;
        }
        h1 { text-align: center; color: #2c3e50; }
        h3 { color: #2c3e50; border-bottom: 2px solid #27ae60; padding-bottom: 5px; }
        .sensor-data {
            display: flex;
            justify-content: space-around;
            margin: 20px 0;
            flex-wrap: wrap;
        }
        .sensor-box {
            text-align: center;
            background-color: #e8f8f0;
            padding: 15px;
            border-radius: 8px;
            min-width: 120px;
            margin: 5px;
        }
        .sensor-box.main {
            background-color: #d5f5e3;
            border: 2px solid #27ae60;
        }
        .sensor-value {
            font-size: 2em;
            font-weight: bold;
            color: #2c3e50;
        }
        .sensor-value.main {
            font-size: 2.5em;
            color: #27ae60;
        }
        .sensor-unit {
            font-size: 0.8em;
            color: #7f8c8d;
        }
        .control-section {
            margin: 30px 0;
            padding: 20px;
            background-color: #f8f9fa;
            border-radius: 8px;
        }
        .button {
            padding: 12px 24px;
            font-size: 1.1em;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            margin: 5px;
            transition: background-color 0.3s;
        }
        .button-on     { background-color: #27ae60; color: white; }
        .button-off    { background-color: #e74c3c; color: white; }
        .button-update { background-color: #3498db; color: white; }
        .button:hover  { opacity: 0.8; }
        .status {
            text-align: center;
            padding: 10px;
            border-radius: 6px;
            margin: 10px 0;
        }
        .status-on  { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .status-off { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .tabs {
            display: flex;
            border-bottom: 2px solid #ddd;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }
        .tab {
            padding: 12px 24px;
            cursor: pointer;
            border: none;
            background-color: #f8f9fa;
            border-bottom: 3px solid transparent;
            transition: all 0.3s;
        }
        .tab.active {
            background-color: white;
            border-bottom-color: #27ae60;
            font-weight: bold;
        }
        .tab-content         { display: none; }
        .tab-content.active  { display: block; }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .status-box-info {
            background-color: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            border-left: 4px solid #27ae60;
        }
        .status-label {
            font-size: 0.85em;
            color: #6c757d;
            margin-bottom: 5px;
            font-weight: 500;
        }
        .system-info {
            margin-top: 20px;
            padding: 15px;
            background-color: #f8f9fa;
            border-radius: 8px;
            border-left: 4px solid #28a745;
        }
        .system-values {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            font-size: 0.9em;
        }
        .system-value {
            background-color: white;
            padding: 8px;
            border-radius: 4px;
            text-align: center;
        }
        .sys-label {
            font-weight: bold;
            color: #495057;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background-color: #e9ecef;
            border-radius: 15px;
            overflow: hidden;
            margin: 10px 0;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #28a745, #20c997);
            text-align: center;
            line-height: 30px;
            color: white;
            font-weight: bold;
            transition: width 0.3s ease;
        }
        .warning {
            background-color: #f8d7da;
            color: #721c24;
            padding: 10px;
            border-radius: 6px;
            margin: 10px 0;
            border-left: 4px solid #dc3545;
        }
        .file-input {
            margin: 10px 0;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
            width: 100%;
        }
        .update-section {
            margin-top: 20px;
            padding: 20px;
            background-color: #fff3cd;
            border-radius: 8px;
            border-left: 4px solid #ffc107;
        }
        .pcf-btn {
            padding: 10px 4px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            background: #6c757d;
            color: white;
            font-size: 0.85em;
            line-height: 1.5;
            transition: background 0.2s;
        }
        .pcf-btn:hover { opacity: 0.85; }
        .section-heading {
            margin-top: 30px;
        }
        .section-heading:first-child { margin-top: 0; }
        .device-badge {
            color: white;
            padding: 2px 10px;
            border-radius: 10px;
            font-size: 0.85em;
            white-space: nowrap;
        }
        html:not([data-lang="en"]) .en { display: none; }
        html[data-lang="en"] .de { display: none; }
        .lang-toggle {
            display: flex; gap: 2px;
            background: #f8f9fa; border: 1px solid #ddd; border-radius: 5px;
            padding: 2px;
        }
        .lang-btn {
            font: inherit; font-size: 11px; font-weight: 700; letter-spacing: 0.04em;
            border: none; background: none; color: #6c757d;
            padding: 3px 9px; border-radius: 3px; cursor: pointer;
        }
        .lang-btn.active { background: #27ae60; color: #fff; }
    </style>
    <script>document.documentElement.setAttribute('data-lang', localStorage.getItem('aeroponik-ui-lang') || 'de');</script>
</head>
<body>
    <div class="container">
        <div style="display:flex;justify-content:space-between;align-items:baseline;flex-wrap:wrap;gap:14px;margin-bottom:10px;">
            <h1 style="margin:0;">🌿 Grow Center</h1>
            <div style="display:flex;align-items:baseline;gap:14px;flex-wrap:wrap;">
                <div id="ahtRow" style="font-size:1em;color:#555;white-space:nowrap;display:none;">
                    🌡️ <span id="ahtTemp">--</span>°C &nbsp; 💧 <span id="ahtHum">--</span>%
                </div>
                <div style="font-size:1.1em;color:#555;white-space:nowrap;">🕐 <span id="datetime">--</span></div>
                <div class="lang-toggle">
                    <button class="lang-btn" data-lang-btn="de" onclick="setLang('de')">DE</button>
                    <button class="lang-btn" data-lang-btn="en" onclick="setLang('en')">EN</button>
                </div>
            </div>
        </div>

        <div class="tabs">
            <button class="tab active" onclick="showTab('startseite',    this)">🏠 <span class="de">Startseite</span><span class="en">Home</span></button>
            <button class="tab"        onclick="showTab('konfiguration', this)">⚙️ <span class="de">Konfiguration</span><span class="en">Settings</span></button>
        </div>

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
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="druckBar">--</div><div class="sensor-unit">bar <span class="de">Druck</span><span class="en">Pressure</span></div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="volumenLiter">--</div><div class="sensor-unit">L <span class="de">Füllstand</span><span class="en">Fill Level</span></div></div>
                    </div>
                    <div id="messwerteExtra"></div>
                </div>

                <!-- Ventile-Status (nur aktive Behälter) -->
                <div class="status-box-info">
                    <strong>🚿 <span class="de">Behälter</span><span class="en">Reservoirs</span></strong>
                    <div id="behaelterStatusCards"></div>
                </div>

                <!-- Licht (PPFD) & Zeitplan -->
                <div class="status-box-info">
                    <strong>☀ <span class="de">Licht (PPFD) &amp; Zeitplan</span><span class="en">Light (PPFD) &amp; Schedule</span></strong>
                    <div id="lichtsensorData" style="display:none;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="lichtPpfd">--</div><div class="sensor-unit">µmol/m²/s PPFD</div></div>
                        <div id="lichtSatBox" style="display:none;margin-top:8px;padding:6px;background:#fde8e8;border-radius:4px;text-align:center;color:#e74c3c;font-weight:bold;font-size:0.85em;"><span class="de">SAT! Sensor gesättigt</span><span class="en">SAT! Sensor saturated</span></div>
                        <canvas id="spektrumChart" width="320" height="110"
                            style="margin-top:10px;border-radius:6px;display:block;max-width:100%;background:#111;"></canvas>
                        <div style="text-align:right;font-size:0.75em;color:#aaa;margin-top:4px;">
                            <span class="de">Zuletzt:</span><span class="en">Last:</span> <span id="lichtAge">--</span>
                        </div>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-top:12px;">
                        <span style="font-size:0.85em;color:#6c757d;"><span class="de">Zeitplan</span><span class="en">Schedule</span></span>
                        <div id="schedPhase" style="padding:2px 10px;border-radius:10px;background:#f0f0f0;font-size:0.8em;">—</div>
                    </div>
                    <canvas id="schedChart" width="800" height="130"
                        style="width:100%;border-radius:6px;margin-top:6px;"></canvas>
                </div>

                <!-- CO2-Steuerung (nur wenn aktiv) -->
                <div class="status-box-info" id="co2StatusCard" style="display:none;">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>🌫️ CO2-<span class="de">Steuerung</span><span class="en">Control</span></strong>
                        <span id="co2OutputBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="co2StatusNow">--</div><div class="sensor-unit">ppm CO2</div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="co2StatusRange">--</div><div class="sensor-unit">Min / Max</div></div>
                    </div>
                </div>

                <!-- Abluftluefter (RS485 Adresse 6, MARS Hydro) -->
                <div class="status-box-info" id="fanStatusCard" style="display:none;">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>🌀 <span class="de">Abluftlüfter</span><span class="en">Exhaust Fan</span></strong>
                        <span id="fanModeBadge" style="background:#6c757d;color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">--</span>
                    </div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.95em;margin-top:10px;">
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="fanPercent">--</div><div class="sensor-unit">% <span class="de">Leistung</span><span class="en">Power</span></div></div>
                        <div class="sensor-box" style="padding:8px;"><div class="sensor-value" style="font-size:1.4em;" id="fanRpm">--</div><div class="sensor-unit">RPM</div></div>
                    </div>
                </div>
            </div>
        </div>

        <!-- ============================================================ -->
        <!-- KONFIGURATION — alles Einstellbare, untereinander             -->
        <!-- ============================================================ -->
        <div id="konfigurationTab" class="tab-content">

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

            <!-- Ventile-Konfiguration -->
            <h3 class="section-heading">🚿 <span class="de">Ventile-Konfiguration</span><span class="en">Valve Configuration</span></h3>
            <div style="margin-bottom:15px; padding:12px; background:#f0f8ff; border-radius:8px; border-left:4px solid #17a2b8;">
                <label><input type="checkbox" id="vorlaufAktiv" onchange="saveVentilConfig()"> <span class="de">Vorlauf Rückleitung aktiv</span><span class="en">Return pre-flush active</span></label>
                &nbsp;&nbsp;
                <label><span class="de">Vorlaufzeit</span><span class="en">Pre-flush time</span>: <input type="number" id="vorlaufzeit" min="1" max="30" style="width:55px;" onchange="saveVentilConfig()"> s</label>
            </div>

            <div id="behaelterCards" style="display:grid; grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); gap:15px;"></div>

            <div style="text-align:center; margin-top:20px;">
                <button class="button button-on" onclick="saveVentilConfig()">💾 <span class="de">Speichern</span><span class="en">Save</span></button>
            </div>

            <div style="margin-top:24px;padding:16px;background:#fff8e1;border-radius:8px;border-left:4px solid #ffc107;">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
                    <h4 style="margin:0;">🔌 PCF8574 <span class="de">Ausgangstest</span><span class="en">Output Test</span></h4>
                    <button id="pcfTestToggle" onclick="togglePcfTest()"
                        style="background:#e74c3c;color:white;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;font-size:0.9em;">
                        <span class="de">Test starten</span><span class="en">Start Test</span></button>
                </div>
                <div id="pcfTestWarning" style="display:none;margin-bottom:12px;padding:8px;background:#f8d7da;border-radius:4px;font-size:0.85em;color:#721c24;">
                    ⚠ <span class="de">Automatische Ventilsteuerung pausiert!</span><span class="en">Automatic valve control paused!</span>
                </div>
                <div id="pcfTestButtons" style="display:grid;grid-template-columns:repeat(4,1fr);gap:8px;opacity:0.4;pointer-events:none;">
                    <button id="pcf_btn_0" onclick="pcfToggle(0)" class="pcf-btn">P0<br><small>MV1 <span class="de">Beh.1</span><span class="en">Res.1</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_1" onclick="pcfToggle(1)" class="pcf-btn">P1<br><small>MV2 <span class="de">Beh.2</span><span class="en">Res.2</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_2" onclick="pcfToggle(2)" class="pcf-btn">P2<br><small>MV3 <span class="de">Beh.3</span><span class="en">Res.3</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_3" onclick="pcfToggle(3)" class="pcf-btn">P3<br><small><span class="de">Rücklauf</span><span class="en">Return</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_4" onclick="pcfToggle(4)" class="pcf-btn">P4<br><small><span class="de">Reserve</span><span class="en">Spare</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_5" onclick="pcfToggle(5)" class="pcf-btn">P5<br><small><span class="de">Reserve</span><span class="en">Spare</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_6" onclick="pcfToggle(6)" class="pcf-btn">P6<br><small><span class="de">Reserve</span><span class="en">Spare</span></small><br><span>AUS</span></button>
                    <button id="pcf_btn_7" onclick="pcfToggle(7)" class="pcf-btn">P7<br><small><span class="de">Reserve</span><span class="en">Spare</span></small><br><span>AUS</span></button>
                </div>
            </div>

            <!-- Zeitplan-Konfiguration -->
            <h3 class="section-heading">🌅 <span class="de">Zeitplan-Konfiguration</span><span class="en">Schedule Configuration</span></h3>
            <div class="status-box-info">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Aktiv</span><span class="en">Active</span></span>
                        <input type="checkbox" id="schedEnabled" onchange="schedDirty();updateSchedPhase()" style="width:18px;height:18px;">
                    </label>
                </div>

                <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px;">
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

            <!-- Abluftluefter-Steuerung -->
            <h3 class="section-heading">🌀 <span class="de">Abluftlüfter-Steuerung</span><span class="en">Exhaust Fan Control</span></h3>
            <div class="status-box-info">
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
            </div>

            <!-- Tank-Konfiguration -->
            <h3 class="section-heading">🪣 <span class="de">Tank-Konfiguration</span><span class="en">Tank Configuration</span></h3>
            <div id="tankConfigSection">
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

                <h4><span class="de">Sensor Kalibrierung</span><span class="en">Sensor Calibration</span></h4>
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

                <div style="text-align:center;">
                    <button class="button button-on" onclick="saveTankKonfig()">💾 <span class="de">Speichern</span><span class="en">Save</span></button>
                </div>
            </div>

            <!-- Node-Steuerung -->
            <h3 class="section-heading">📡 <span class="de">Node-Steuerung</span><span class="en">Node Control</span></h3>
            <div id="nodesControlContainer" style="display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:15px;">
                <div id="nodes_control_loading" style="text-align:center;color:#888;padding:30px;grid-column:1/-1;"><span class="de">Lade...</span><span class="en">Loading...</span></div>
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
                </div>
            </div>
        </div>

        <!-- Geraetestatus (immer sichtbar, unabhaengig vom aktiven Tab) -->
        <div id="deviceStatusBar" style="margin-top:20px;padding:10px 14px;background:#f8f9fa;border-radius:8px;font-size:0.82em;display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:center;color:#555;">
            <span id="wifiMode"><span class="de">Lade Verbindungsinfo...</span><span class="en">Loading connection info...</span></span>
            <span id="badge_ms2"   class="device-badge" style="background:#6c757d;">Multisensor</span>
            <span id="badge_licht" class="device-badge" style="background:#6c757d;"><span class="de">Licht</span><span class="en">Light</span></span>
            <span id="badge_fan"   class="device-badge" style="background:#6c757d;"><span class="de">Lüfter</span><span class="en">Fan</span></span>
            <span id="espnowBadges" style="display:contents;"></span>
        </div>
    </div>

    <script>
        let updateInProgress = false;

        // ===== Sprachumschaltung (DE/EN) =====
        let currentLang = localStorage.getItem('aeroponik-ui-lang') || 'de';
        const I18N = {
            de: {
                loading: 'Lade...',
                accessPoint: 'Access Point',
                wifiConnected: 'WLAN verbunden',
                confirmUpdateStart: 'Update starten?',
                uploadRunning: 'Upload läuft...',
                updateDone: 'Abgeschlossen! Neustart...',
                unknown: 'Unbekannt',
                setBtn: 'Setzen',
                offBtn: 'Aus',
                on: 'EIN',
                off: 'AUS',
                nodeTypeLight: 'Lichtsteuerung',
                nodeTypeMultisensor: 'Multisensor',
                nodeTypeSocket: 'Steckdose',
                nodeTypeSpektrum: 'Spektrum-Sensor',
                agoSeconds: (s) => 'vor ' + s + ' s',
                otaUploading: (kb) => 'Upload ' + kb + ' KB...',
                otaFlashCmd: (b) => 'OK (' + b + ' B) — Flash-Befehl...',
                otaStarted: 'OTA gestartet! Node flasht...',
                errorPrefix: 'Fehler: ',
                phaseOff: 'Deaktiviert',
                phaseNight: 'Nacht',
                phaseDawn: 'Sonnenaufgang',
                phaseDay: 'Tag',
                phaseDusk: 'Sonnenuntergang',
                channelsLabel: 'Kanäle',
                saved: 'Gespeichert',
                error: 'Fehler',
                connectionError: 'Verbindungsfehler',
                zustandPause: 'Pause',
                zustandVorlauf: 'Vorlauf',
                zustandAktiv: 'Aktiv',
                behaelter: 'Behälter',
                timerLabel: 'Timer',
                noActiveReservoirs: 'Keine aktiven Behälter',
                active: 'Aktiv',
                openingLabel: 'Öffnung (s)',
                pauseLabel: 'Pause (min)',
                tentValues: 'Zeltwerte',
                startTest: 'Test starten',
                stopTest: 'Test beenden',
                noneOption: '— keine —',
                offlineSuffix: 'offline',
                noCo2Value: 'Kein CO2-Wert',
                fanModeManual: 'Manuell',
                fanModeAutoHum: 'Automatik Feuchte',
                fanModeAutoTemp: 'Automatik Temperatur',
                diffMinLabel: 'Differenz Min (°C)',
                diffMaxLabel: 'Differenz Max (°C)',
                tempMinLabel: 'Temperatur Min (°C)',
                tempMaxLabel: 'Temperatur Max (°C)',
                wifiConnectedTo: (ssid, ip) => 'Verbunden mit "' + ssid + '" — IP ' + ip,
                fallbackApActive: (ssid, ip) => 'Fallback-Hotspot aktiv: ' + ssid + ' (' + ip + ')',
                notConnected: 'Nicht verbunden',
                loadError: 'Fehler beim Laden',
                ssidEmptyAlert: 'SSID darf nicht leer sein',
                savedRestarting: 'Gespeichert, Master startet neu…',
                noFilesFound: 'Keine Dateien gefunden',
                confirmDeleteFile: (name) => 'Datei "' + name + '" wirklich löschen?',
                deleteFailed: 'Löschen fehlgeschlagen',
                externalControl: 'Extern (iControl)',
                saveBtn: 'Speichern',
            },
            en: {
                loading: 'Loading...',
                accessPoint: 'Access Point',
                wifiConnected: 'WiFi connected',
                confirmUpdateStart: 'Start update?',
                uploadRunning: 'Uploading...',
                updateDone: 'Done! Restarting...',
                unknown: 'Unknown',
                setBtn: 'Set',
                offBtn: 'Off',
                on: 'ON',
                off: 'OFF',
                nodeTypeLight: 'Light Control',
                nodeTypeMultisensor: 'Multisensor',
                nodeTypeSocket: 'Socket',
                nodeTypeSpektrum: 'Spectrum Sensor',
                agoSeconds: (s) => s + ' s ago',
                otaUploading: (kb) => 'Uploading ' + kb + ' KB...',
                otaFlashCmd: (b) => 'OK (' + b + ' B) — sending flash command...',
                otaStarted: 'OTA started! Node is flashing...',
                errorPrefix: 'Error: ',
                phaseOff: 'Disabled',
                phaseNight: 'Night',
                phaseDawn: 'Sunrise',
                phaseDay: 'Day',
                phaseDusk: 'Sunset',
                channelsLabel: 'Channels',
                saved: 'Saved',
                error: 'Error',
                connectionError: 'Connection error',
                zustandPause: 'Pause',
                zustandVorlauf: 'Pre-flush',
                zustandAktiv: 'Active',
                behaelter: 'Reservoir',
                timerLabel: 'Timer',
                noActiveReservoirs: 'No active reservoirs',
                active: 'Active',
                openingLabel: 'Opening (s)',
                pauseLabel: 'Pause (min)',
                tentValues: 'Tent Values',
                startTest: 'Start Test',
                stopTest: 'Stop Test',
                noneOption: '— none —',
                offlineSuffix: 'offline',
                noCo2Value: 'No CO2 value',
                fanModeManual: 'Manual',
                fanModeAutoHum: 'Auto Humidity',
                fanModeAutoTemp: 'Auto Temperature',
                diffMinLabel: 'Differential Min (°C)',
                diffMaxLabel: 'Differential Max (°C)',
                tempMinLabel: 'Temperature Min (°C)',
                tempMaxLabel: 'Temperature Max (°C)',
                wifiConnectedTo: (ssid, ip) => 'Connected to "' + ssid + '" — IP ' + ip,
                fallbackApActive: (ssid, ip) => 'Fallback hotspot active: ' + ssid + ' (' + ip + ')',
                notConnected: 'Not connected',
                loadError: 'Error loading',
                ssidEmptyAlert: 'SSID must not be empty',
                savedRestarting: 'Saved, master is restarting…',
                noFilesFound: 'No files found',
                confirmDeleteFile: (name) => 'Really delete file "' + name + '"?',
                deleteFailed: 'Delete failed',
                externalControl: 'External (iControl)',
                saveBtn: 'Save',
            }
        };
        function t(key, ...args) {
            const dict = I18N[currentLang] || I18N.de;
            const entry = (key in dict) ? dict[key] : I18N.de[key];
            if (entry === undefined) return key;
            return typeof entry === 'function' ? entry(...args) : entry;
        }
        function applyLangButtons() {
            document.querySelectorAll('.lang-btn').forEach(b => b.classList.toggle('active', b.dataset.langBtn === currentLang));
        }
        // <option> kann keine <span>-Kinder rendern -> Text ueber data-de/data-en Attribute umschalten
        function applyOptionTranslations() {
            document.querySelectorAll('option.i18n-opt').forEach(o => {
                o.textContent = (currentLang === 'en' ? o.dataset.en : o.dataset.de) || o.textContent;
            });
        }
        function setLang(lang) {
            currentLang = lang;
            document.documentElement.setAttribute('data-lang', lang);
            applyLangButtons();
            applyOptionTranslations();
            localStorage.setItem('aeroponik-ui-lang', lang);
            rebuildDynamicContent();
        }
        // Erzwingt einen Neuaufbau aller dynamisch/einmalig aufgebauten Inhalte, damit
        // bereits gerenderte Karten/Labels sofort die neue Sprache zeigen, statt erst
        // auf die naechste natuerliche Datenaenderung zu warten.
        function rebuildDynamicContent() {
            messwerteBuilt.clear();
            const me = document.getElementById('messwerteExtra');
            if (me) me.innerHTML = '';
            nodesControlBuilt.clear();
            const ncc = document.getElementById('nodesControlContainer');
            if (ncc) ncc.innerHTML = '<div id="nodes_control_loading" style="text-align:center;color:#888;padding:30px;grid-column:1/-1;">' + t('loading') + '</div>';
            ventilKonfigGeladen = false;
            updateData(); updateVentile(); updateNodes(); updateRs485(); updateLicht();
            updateCo2Status(); updateFanStatus();
            loadTankKonfig(); loadZeitplan(); loadCo2Config(); loadFanConfig();
            loadWifiConfigForm(); loadLogFileList();
            updateSchedPhase();
            updatePcfUi();
            updateFanModeVisibility();
        }

        function showTab(tabName, btn) {
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.getElementById(tabName + 'Tab').classList.add('active');
            btn.classList.add('active');
            if (tabName === 'konfiguration') {
                loadTankKonfig();
                loadZeitplan();
                loadCo2Config();
                loadFanConfig();
                loadLogFileList();
                loadWifiConfigForm();
            }
        }

        function updateData() {
            if (updateInProgress) return;

            fetch('/api/data')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('datetime').textContent    = d.datetime || '--';
                    document.getElementById('druckBar').textContent   = d.druckBar;
                    document.getElementById('tempVorrat').textContent  = d.tempVorrat.toFixed(1);
                    document.getElementById('tempPflanze').textContent = d.tempPflanze.toFixed(1);
                    document.getElementById('volumenLiter').textContent     = d.volumenLiter;
                    const tsl = document.getElementById('tankSensorLive');
                    if (tsl) tsl.textContent = d.distanzMM + ' mm';

                    const ahtRow = document.getElementById('ahtRow');
                    if (ahtRow) {
                        if (d.aht21Ok) {
                            ahtRow.style.display = '';
                            document.getElementById('ahtTemp').textContent = parseFloat(d.aht21Temp).toFixed(1);
                            document.getElementById('ahtHum').textContent  = parseFloat(d.aht21Hum).toFixed(1);
                        } else {
                            ahtRow.style.display = 'none';
                        }
                    }

                    const wifiEl = document.getElementById('wifiMode');
                    if (d.wifiMode === 'AP') {
                        wifiEl.innerHTML = t('accessPoint') + '<br>IP: ' + d.ipAddress;
                        wifiEl.style.backgroundColor = '#d1ecf1';
                    } else {
                        wifiEl.innerHTML = t('wifiConnected') + '<br>IP: ' + d.ipAddress;
                        wifiEl.style.backgroundColor = '#d4edda';
                    }

                    if (d.freeHeap !== undefined) {
                        document.getElementById('freeHeap').textContent    = Math.round(d.freeHeap / 1024) + ' KB';
                        document.getElementById('uptime').textContent      = Math.round(d.uptime / 60) + ' Min';
                        document.getElementById('chipModel').textContent   = d.chipModel;
                        document.getElementById('chipRevision').textContent = d.chipRevision;
                        document.getElementById('flashSize').textContent   = Math.round(d.flashSize / (1024 * 1024)) + ' MB';
                        document.getElementById('sketchSize').textContent  = Math.round(d.sketchSize / 1024) + ' KB';
                    }
                })
                .catch(() => {});
        }

        function startUpdate() {
            if (!confirm(t('confirmUpdateStart'))) return false;
            updateInProgress = true;
            document.getElementById('progressBar').style.display = 'block';
            document.getElementById('updateStatus').innerHTML = t('uploadRunning');
            let progress = 0;
            const interval = setInterval(() => {
                if (progress < 95) {
                    progress += Math.random() * 5;
                    if (progress > 95) progress = 95;
                    document.getElementById('progressFillUpdate').style.width   = progress + '%';
                    document.getElementById('progressFillUpdate').textContent   = Math.round(progress) + '%';
                }
            }, 1000);
            setTimeout(() => {
                clearInterval(interval);
                document.getElementById('progressFillUpdate').style.width = '100%';
                document.getElementById('progressFillUpdate').textContent = '100%';
                document.getElementById('updateStatus').innerHTML = t('updateDone');
            }, 30000);
            return true;
        }

        // ===== Nodes =====
        function nodeTypeName(type) {
            const names = { 1: t('nodeTypeLight'), 2: t('nodeTypeMultisensor'), 3: t('nodeTypeSocket'), 4: t('nodeTypeSpektrum') };
            return names[type];
        }
        const nodesControlBuilt = new Set();

        // PPFD-Karte hat drei moegliche Quellen: RS485-Multisensor (MS2, /api/rs485 —
        // bei diesem Aufbau die tatsaechliche Quelle), ESP-NOW-Sensor-Node im einfachen
        // Format (/api/espnow), oder ein dedizierter Spektrum-Node (/api/licht).
        // Alle drei laufen unabhaengig/asynchron — nur ausblenden wenn WIRKLICH alle leer sind.
        let lichtHasMs2Data       = false;
        let lichtHasEspNowData    = false;
        let lichtHasDedicatedData = false;
        function updateLichtVisibility() {
            const dataEl = document.getElementById('lichtsensorData');
            if (dataEl) dataEl.style.display = (lichtHasMs2Data || lichtHasEspNowData || lichtHasDedicatedData) ? 'block' : 'none';
        }

        // ===== Messwerte-Bloecke (Startseite): Temp/Hum/VPD/CO2, kein PPFD =====
        // Ein Block pro Quelle (ESP-NOW-Sensor-Node oder RS485-MS2), alle in
        // derselben "Messwerte"-Karte statt eigener Karten pro Quelle.
        const messwerteBuilt = new Set();

        // VPD (Vapor Pressure Deficit) aus Temperatur+Feuchte — Tetens-Formel,
        // ohne Blatt-Temperatur-Offset (einfache/haeufigste Variante).
        function computeVpdDisplay(temp, hum) {
            const t = parseFloat(temp), h = parseFloat(hum);
            if (isNaN(t) || isNaN(h)) return '--';
            const svp = 0.6108 * Math.exp((17.27 * t) / (t + 237.3));  // kPa
            const vpd = svp * (1 - h / 100);
            return vpd.toFixed(2);
        }

        function messwerteBlockHtml(key, label, online, temp, hum, co2, vpd) {
            return `<div class="sensor-box" style="padding:8px;margin-top:8px;text-align:left;" id="msw_block_${key}">
                <div style="display:flex;justify-content:space-between;align-items:center;font-size:0.8em;color:#6c757d;">
                    <span>${label}</span>
                    <span id="msw_badge_${key}" style="background:${online?'#28a745':'#dc3545'};color:white;padding:1px 8px;border-radius:8px;font-size:0.85em;">${online?'Online':'Offline'}</span>
                </div>
                <div style="display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-top:6px;">
                    <div><div class="sensor-value" style="font-size:1.1em;" id="msw_temp_${key}">${temp}</div><div class="sensor-unit">°C</div></div>
                    <div><div class="sensor-value" style="font-size:1.1em;" id="msw_hum_${key}">${hum}</div><div class="sensor-unit">%</div></div>
                    <div><div class="sensor-value" style="font-size:1.1em;" id="msw_vpd_${key}">${vpd}</div><div class="sensor-unit">kPa VPD</div></div>
                    <div><div class="sensor-value" style="font-size:1.1em;" id="msw_co2_${key}">${co2}</div><div class="sensor-unit">ppm</div></div>
                </div>
            </div>`;
        }

        function upsertMesswerteBlock(key, label, online, temp, hum, co2) {
            const vpd = computeVpdDisplay(temp, hum);
            const container = document.getElementById('messwerteExtra');
            if (!container) return;
            let block = document.getElementById('msw_block_' + key);
            if (!block) {
                const wrapper = document.createElement('div');
                wrapper.innerHTML = messwerteBlockHtml(key, label, online, temp, hum, co2, vpd);
                container.appendChild(wrapper.firstChild);
                messwerteBuilt.add(key);
                return;
            }
            const badge = document.getElementById('msw_badge_' + key);
            if (badge) { badge.textContent = online ? 'Online' : 'Offline'; badge.style.background = online ? '#28a745' : '#dc3545'; }
            const set = (id, v) => { const e = document.getElementById(id); if (e) e.textContent = v; };
            set('msw_temp_' + key, temp);
            set('msw_hum_'  + key, hum);
            set('msw_vpd_'  + key, vpd);
            set('msw_co2_'  + key, co2);
        }

        function removeMesswerteBlock(key) {
            const block = document.getElementById('msw_block_' + key);
            if (block) block.remove();
            messwerteBuilt.delete(key);
        }

        // Steuer-Karte (Konfiguration): PWM-Regler, Relais-Buttons, OTA-Flash
        function buildNodeControlCard(n) {
            const typName = nodeTypeName(n.type) || (t('unknown') + ' (0x' + n.type.toString(16) + ')');
            let h = `<div class="status-box-info" id="node_card_ctrl_${n.id}" style="opacity:${n.online?1:0.6}">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
                    <strong>Node ${n.id}</strong>
                    <span id="node_badge_ctrl_${n.id}" style="background:${n.online?'#28a745':'#dc3545'};color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">${n.online?'Online':'Offline'}</span>
                </div>
                <div style="font-size:0.82em;color:#6c757d;margin-bottom:10px;">${typName}</div>`;
            if (n.pwm_ch1 !== undefined) {
                const p1 = Math.round(n.pwm_ch1 / 10), p2 = Math.round(n.pwm_ch2 / 10);
                h += `<div style="margin-top:10px;">
                    <div style="margin-bottom:8px;font-size:0.9em;"><label>CH1 <span id="lbl_ch1_${n.id}">${p1}</span>%</label>
                        <input type="range" id="sl_ch1_${n.id}" min="0" max="100" value="${p1}" style="width:100%;margin:4px 0;"
                            oninput="document.getElementById('lbl_ch1_${n.id}').textContent=this.value"></div>
                    <div style="margin-bottom:10px;font-size:0.9em;"><label>CH2 <span id="lbl_ch2_${n.id}">${p2}</span>%</label>
                        <input type="range" id="sl_ch2_${n.id}" min="0" max="100" value="${p2}" style="width:100%;margin:4px 0;"
                            oninput="document.getElementById('lbl_ch2_${n.id}').textContent=this.value"></div>
                    <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;">
                        <button onclick="setPwm(${n.id})" style="background:#3498db;color:white;border:none;padding:8px;border-radius:4px;cursor:pointer;">${t('setBtn')}</button>
                        <button onclick="setPwmOff(${n.id})" style="background:#6c757d;color:white;border:none;padding:8px;border-radius:4px;cursor:pointer;">${t('offBtn')}</button>
                    </div></div>`;
            }
            if (n.relay_mask !== undefined) {
                h += `<div style="display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-top:10px;">`;
                for (let r = 0; r < 4; r++) {
                    const on = (n.relay_mask >> r) & 1;
                    h += `<button id="relay_btn_${n.id}_${r}" onclick="setRelayMask(${n.id},${n.relay_mask^(1<<r)})"
                        style="background:${on?'#28a745':'#6c757d'};color:white;border:none;padding:8px 4px;border-radius:4px;cursor:pointer;font-size:0.85em;">
                        R${r+1}<br><strong>${on?t('on'):t('off')}</strong></button>`;
                }
                h += `</div>`;
            }
            h += `<div style="margin-top:10px;border-top:1px solid #eee;padding-top:8px;display:flex;gap:8px;align-items:center;">
                <input type="file" id="fw_${n.id}" accept=".bin" style="display:none" onchange="handleFwFile(${n.id},this)">
                <button onclick="document.getElementById('fw_${n.id}').click()"
                    style="background:#6c3483;color:white;border:none;padding:5px 12px;border-radius:4px;cursor:pointer;font-size:0.82em;">Flash</button>
                <span id="ota_status_${n.id}" style="font-size:0.79em;color:#888;"></span>
            </div>`;
            h += `</div>`;
            return h;
        }

        function refreshNodeControlCard(n) {
            const card = document.getElementById('node_card_ctrl_' + n.id);
            if (!card) return;
            const needsRebuild =
                (n.relay_mask !== undefined && !document.getElementById('relay_btn_' + n.id + '_0')) ||
                (n.pwm_ch1    !== undefined && !document.getElementById('sl_ch1_' + n.id));
            if (needsRebuild) {
                nodesControlBuilt.delete(n.id);
                card.remove();
                return;
            }
            card.style.opacity = n.online ? 1 : 0.6;
            const badge = document.getElementById('node_badge_ctrl_' + n.id);
            if (badge) { badge.textContent = n.online ? 'Online' : 'Offline'; badge.style.background = n.online ? '#28a745' : '#dc3545'; }
            if (n.relay_mask !== undefined) {
                for (let r = 0; r < 4; r++) {
                    const btn = document.getElementById('relay_btn_' + n.id + '_' + r);
                    if (btn) {
                        const on = (n.relay_mask >> r) & 1;
                        btn.style.background = on ? '#28a745' : '#6c757d';
                        btn.innerHTML = 'R'+(r+1)+'<br><strong>'+(on?t('on'):t('off'))+'</strong>';
                        btn.onclick = (function(mask){ return function(){ setRelayMask(n.id, mask); }; })(n.relay_mask ^ (1 << r));
                    }
                }
            }
            // PWM-Slider werden NICHT aktualisiert (Nutzer koennte gerade einstellen)
        }

        function updateNodes() {
            fetch('/api/espnow').then(r => r.json()).then(d => {
                const ctrlContainer = document.getElementById('nodesControlContainer');
                const loading2 = document.getElementById('nodes_control_loading');
                if (loading2) loading2.remove();

                const currentIds = new Set((d.nodes || []).map(n => n.id));
                // Messwerte-Bloecke von ESP-NOW-Sensor-Nodes entfernen, die verschwunden sind
                // (MS2 bleibt unabhaengig davon immer bestehen, siehe updateRs485)
                for (const key of [...messwerteBuilt]) {
                    if (!key.startsWith('node')) continue;
                    const id = parseInt(key.slice(4));
                    if (!currentIds.has(id)) removeMesswerteBlock(key);
                }
                for (const id of [...nodesControlBuilt]) {
                    if (!currentIds.has(id)) {
                        const el = document.getElementById('node_card_ctrl_' + id);
                        if (el) el.remove();
                        nodesControlBuilt.delete(id);
                    }
                }
                lichtHasEspNowData = false;
                for (const n of (d.nodes || [])) {
                    // Messwerte (Startseite) — nur Temp/Hum/CO2, kein PPFD/Spektrum
                    if (n.temperature !== undefined) {
                        const typName = nodeTypeName(n.type) || 'Node';
                        upsertMesswerteBlock('node' + n.id, typName + ' (Node ' + n.id + ')',
                                             n.online, n.temperature, n.humidity, n.co2);
                    }
                    // PPFD/Spektrum — bei den meisten Aufbauten steckt der Lichtsensor
                    // im Multisensor (NODE_TYPE_SENSOR, einfaches Format), nicht in
                    // einem eigenen NODE_TYPE_SPEKTRUM-Node (der ueber /api/licht laeuft).
                    if (n.ppfd !== undefined || (n.spektrum && n.spektrum.length === 8)) {
                        lichtHasEspNowData = true;
                        const ppfdEl = document.getElementById('lichtPpfd');
                        if (ppfdEl && n.ppfd !== undefined) ppfdEl.textContent = parseFloat(n.ppfd).toFixed(1);
                        if (n.spektrum && n.spektrum.length === 8)
                            requestAnimationFrame(() => drawSpektrumChart('spektrumChart', n.spektrum));
                        const ageEl = document.getElementById('lichtAge');
                        if (ageEl && n.age_s !== undefined) ageEl.textContent = t('agoSeconds', n.age_s);
                    }
                    // Steuer-Karte (Konfiguration) — unveraendert
                    if (ctrlContainer) {
                        if (!nodesControlBuilt.has(n.id)) {
                            const wrapper = document.createElement('div');
                            wrapper.innerHTML = buildNodeControlCard(n);
                            ctrlContainer.appendChild(wrapper.firstChild);
                            nodesControlBuilt.add(n.id);
                        } else {
                            refreshNodeControlCard(n);
                        }
                    }
                }
                updateLichtVisibility();
                updateCo2NodeOptions((d.nodes || []).filter(n => n.type === 3));
                updateEspnowBadges(d.nodes || []);
            }).catch(() => {});
        }

        // Geraetestatus-Leiste (Fussbereich): ein Badge pro ESP-NOW-Node
        const espnowBadgesBuilt = new Set();
        function updateEspnowBadges(nodes) {
            const container = document.getElementById('espnowBadges');
            if (!container) return;
            const currentIds = new Set(nodes.map(n => n.id));
            for (const id of [...espnowBadgesBuilt]) {
                if (!currentIds.has(id)) {
                    const el = document.getElementById('badge_node_' + id);
                    if (el) el.remove();
                    espnowBadgesBuilt.delete(id);
                }
            }
            for (const n of nodes) {
                let el = document.getElementById('badge_node_' + n.id);
                if (!el) {
                    el = document.createElement('span');
                    el.id = 'badge_node_' + n.id;
                    el.className = 'device-badge';
                    container.appendChild(el);
                    espnowBadgesBuilt.add(n.id);
                }
                el.textContent = nodeTypeName(n.type) || ('Node ' + n.id);
                el.style.background = n.online ? '#28a745' : '#dc3545';
            }
        }

        function setPwm(nodeId) {
            const ch1 = parseInt(document.getElementById('sl_ch1_' + nodeId).value) * 10;
            const ch2 = parseInt(document.getElementById('sl_ch2_' + nodeId).value) * 10;
            fetch('/api/espnow/pwm', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({node_id: nodeId, ch1: ch1, ch2: ch2})
            }).then(() => setTimeout(updateNodes, 600)).catch(() => {});
        }

        function setPwmOff(nodeId) {
            fetch('/api/espnow/pwm', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({node_id: nodeId, ch1: 0, ch2: 0})
            }).then(() => setTimeout(updateNodes, 600)).catch(() => {});
        }

        function setRelayMask(nodeId, mask) {
            fetch('/api/espnow/relay', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({node_id: nodeId, mask: mask})
            }).then(() => setTimeout(updateNodes, 600)).catch(() => {});
        }

        async function handleFwFile(nodeId, input) {
            const file = input.files[0];
            if (!file) return;
            const st = document.getElementById('ota_status_' + nodeId);
            st.style.color = '#888';
            st.textContent = t('otaUploading', (file.size/1024).toFixed(0));
            const fd = new FormData();
            fd.append('firmware', file, 'firmware.bin');
            try {
                const r1 = await fetch('/api/ota/upload?node_id=' + nodeId, {method:'POST', body:fd});
                if (!r1.ok) throw new Error('HTTP ' + r1.status);
                const d1 = await r1.json();
                st.textContent = t('otaFlashCmd', d1.size);
                const r2 = await fetch('/api/ota/flash', {
                    method: 'POST',
                    headers: {'Content-Type':'application/json'},
                    body: JSON.stringify({node_id: nodeId})
                });
                const d2 = await r2.json();
                if (d2.success) { st.style.color='#28a745'; st.textContent=t('otaStarted'); }
                else { st.style.color='#dc3545'; st.textContent=t('errorPrefix')+(d2.error||'?'); }
            } catch(e) {
                st.style.color = '#dc3545';
                st.textContent = t('errorPrefix') + e.message;
            }
            input.value = '';
        }

        // ===== Tank =====
        let tankKonfigGeladen = false;

        function loadTankKonfig() {
            if (tankKonfigGeladen) return;
            fetch('/api/tank').then(r => r.json()).then(d => {
                document.getElementById('tankHoehe').value       = d.hoehe_mm;
                document.getElementById('tankRadiusUnten').value = d.radius_unten_mm;
                document.getElementById('tankRadiusOben').value  = d.radius_oben_mm;
                document.getElementById('sensorLeer').value      = d.leer_abstand_mm;
                document.getElementById('sensorMin').value       = d.min_mm;
                document.getElementById('sensorMax').value       = d.max_mm;
                document.getElementById('sensorOffset').value    = d.offset_mm;
                tankKonfigGeladen = true;
            }).catch(() => {});
        }

        function saveTankKonfig() {
            const body = {
                hoehe_mm:        parseInt(document.getElementById('tankHoehe').value),
                radius_unten_mm: parseInt(document.getElementById('tankRadiusUnten').value),
                radius_oben_mm:  parseInt(document.getElementById('tankRadiusOben').value),
                leer_abstand_mm: parseInt(document.getElementById('sensorLeer').value),
                min_mm:          parseInt(document.getElementById('sensorMin').value),
                max_mm:          parseInt(document.getElementById('sensorMax').value),
                offset_mm:       parseInt(document.getElementById('sensorOffset').value)
            };
            fetch('/api/tank/config', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                if (d.success) {
                    const btn = document.querySelector('#tankConfigSection .button-on');
                    btn.textContent = '✅ ' + t('saved');
                    setTimeout(() => btn.textContent = '💾 ' + t('saveBtn'), 2000);
                }
            }).catch(() => {});
        }

        // ===== Zeitplan =====
        let schedLoaded = false;
        let schedDirtyFlag = false;
        let schedNowMin = 0;
        let schedAnchorMin = 0;
        let schedAnchorMs  = 0;

        function minToTime(m) {
            return String(Math.floor(m/60)).padStart(2,'0') + ':' + String(m%60).padStart(2,'0');
        }
        function timeToMin(t) {
            const p = t.split(':');
            return parseInt(p[0])*60 + parseInt(p[1]);
        }
        function schedDirty() { schedDirtyFlag = true; }

        // Spiegelt schedComputeRelayMask() aus scheduler.cpp: Kanaele schalten
        // nacheinander zu (Aufgang) bzw. nacheinander wieder ab (Untergang, umgekehrte Reihenfolge).
        // Muss exakt schedComputeRelayMask() in scheduler.cpp spiegeln (siehe dort fuer
        // die Begruendung: Auf-/Untergang brauchen getrennte Formeln, nicht dieselbe
        // "t"-Variable mit ceil() fuer beide — sonst dimmt der Untergang erst nach 1/n
        // des Fensters ab statt sofort).
        function schedComputeActive(nowMin, n, dawnStart, dawnEnd, duskStart, duskEnd) {
            if (n <= 0 || nowMin < dawnStart || nowMin >= duskEnd) return 0;
            if (nowMin >= dawnEnd && nowMin < duskStart) return n;
            if (nowMin < dawnEnd) {
                let e = (nowMin - dawnStart) / (dawnEnd - dawnStart);
                e = Math.max(0, Math.min(1, e));
                return Math.min(n, Math.ceil(e * n));
            }
            let e = (nowMin - duskStart) / (duskEnd - duskStart);
            e = Math.max(0, Math.min(1, e));
            const drop = Math.ceil(e * n);
            return (drop >= n) ? 0 : n - drop;
        }

        function drawSchedChart() {
            const canvas = document.getElementById('schedChart');
            if (!canvas) return;
            const ctx = canvas.getContext('2d');
            const W = canvas.width, H = canvas.height;
            ctx.clearRect(0, 0, W, H);

            // Hintergrund Nacht→Tag→Nacht
            const bg = ctx.createLinearGradient(0,0,W,0);
            bg.addColorStop(0,    '#0d1117');
            bg.addColorStop(0.22, '#1a2a4a');
            bg.addColorStop(0.35, '#2d5a8e');
            bg.addColorStop(0.5,  '#87CEEB');
            bg.addColorStop(0.65, '#2d5a8e');
            bg.addColorStop(0.78, '#1a2a4a');
            bg.addColorStop(1,    '#0d1117');
            ctx.fillStyle = bg;
            ctx.fillRect(0, 0, W, H);

            const mX = m => (m/1440)*W;
            const dawnStart = timeToMin(document.getElementById('schedDawnStart').value || '06:00');
            const dawnEnd   = timeToMin(document.getElementById('schedDawnEnd').value   || '08:00');
            const duskStart = timeToMin(document.getElementById('schedDuskStart').value || '18:00');
            const duskEnd   = timeToMin(document.getElementById('schedDuskEnd').value   || '20:00');
            const n = parseInt(document.getElementById('schedNumSsr').value) || 4;

            function stepY(m) {
                const active = schedComputeActive(m, n, dawnStart, dawnEnd, duskStart, duskEnd);
                return H - 22 - (active / n) * (H - 34);
            }

            ctx.beginPath();
            for (let m = 0; m <= 1440; m++) {
                const x = mX(m), y = stepY(m);
                if (m === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.lineTo(W, H-22); ctx.lineTo(0, H-22); ctx.closePath();
            ctx.fillStyle = 'rgba(243,156,18,0.30)';
            ctx.fill();
            ctx.beginPath();
            for (let m = 0; m <= 1440; m++) {
                const x = mX(m), y = stepY(m);
                if (m === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.strokeStyle = '#f5b041'; ctx.lineWidth = 2; ctx.stroke();

            // Stundengitter
            ctx.strokeStyle = 'rgba(255,255,255,0.12)'; ctx.lineWidth = 1;
            for (let h = 0; h <= 24; h += 2) {
                const x = mX(h*60);
                ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,H-22); ctx.stroke();
            }

            // Zeitbeschriftung
            ctx.fillStyle = 'rgba(255,255,255,0.6)';
            ctx.font = '10px sans-serif';
            for (let h = 0; h <= 24; h += 4)
                ctx.fillText(h+'h', mX(h*60)+2, H-6);

            // Aktuelle Uhrzeit
            if (schedNowMin > 0) {
                const nx = mX(schedNowMin);
                ctx.strokeStyle = '#2ecc71'; ctx.lineWidth = 1.5;
                ctx.setLineDash([4,3]);
                ctx.beginPath(); ctx.moveTo(nx,0); ctx.lineTo(nx,H-22); ctx.stroke();
                ctx.setLineDash([]);
                ctx.fillStyle = '#2ecc71';
                ctx.fillText(minToTime(schedNowMin), nx+3, 12);
            }
        }

        function updateSchedPhase() {
            const el = document.getElementById('schedPhase');
            if (!el) return;
            if (!document.getElementById('schedEnabled').checked) {
                el.style.background='#eee'; el.style.color='#888'; el.textContent=t('phaseOff'); return;
            }
            const dawnStart = timeToMin(document.getElementById('schedDawnStart').value || '06:00');
            const dawnEnd   = timeToMin(document.getElementById('schedDawnEnd').value   || '08:00');
            const duskStart = timeToMin(document.getElementById('schedDuskStart').value || '18:00');
            const duskEnd   = timeToMin(document.getElementById('schedDuskEnd').value   || '20:00');
            const n         = parseInt(document.getElementById('schedNumSsr').value) || 4;
            const active    = schedComputeActive(schedNowMin, n, dawnStart, dawnEnd, duskStart, duskEnd);

            let phaseKey = 'phaseNight', isTransition = false, isDay = false;
            if (schedNowMin >= dawnStart && schedNowMin < dawnEnd)   { phaseKey = 'phaseDawn'; isTransition = true; }
            else if (schedNowMin >= dawnEnd && schedNowMin < duskStart) { phaseKey = 'phaseDay'; isDay = true; }
            else if (schedNowMin >= duskStart && schedNowMin < duskEnd) { phaseKey = 'phaseDusk'; isTransition = true; }
            el.style.background = isDay ? '#fff3cd' : (isTransition ? '#d4edda' : '#e2e3e5');
            el.style.color = '#333';
            el.textContent = t(phaseKey) + '  ' + t('channelsLabel') + ' ' + active + '/' + n;
        }

        // Zaehlt schedNowMin lokal hoch (verankert an der zuletzt vom Server gelesenen
        // Uhrzeit) und zeichnet Chart + Phase neu — kein erneuter Fetch noetig, damit
        // unsaved Aenderungen im Formular nicht ueberschrieben werden.
        function schedTick() {
            if (!schedLoaded) return;
            const elapsedMin = (Date.now() - schedAnchorMs) / 60000;
            schedNowMin = Math.floor((schedAnchorMin + elapsedMin + 1440) % 1440);
            drawSchedChart();
            updateSchedPhase();
        }

        function loadZeitplan() {
            if (schedLoaded) { drawSchedChart(); return; }
            fetch('/api/scheduler').then(r => r.json()).then(d => {
                document.getElementById('schedEnabled').checked = d.enabled;
                document.getElementById('schedNumSsr').value     = d.num_ssr || 4;
                document.getElementById('schedDawnStart').value = minToTime(d.dawn_start);
                document.getElementById('schedDawnEnd').value   = minToTime(d.dawn_end);
                document.getElementById('schedDuskStart').value = minToTime(d.dusk_start);
                document.getElementById('schedDuskEnd').value   = minToTime(d.dusk_end);
                schedNowMin   = d.now_min || 0;
                schedAnchorMin = schedNowMin;
                schedAnchorMs  = Date.now();
                schedLoaded = true;
                schedDirtyFlag = false;
                updateSchedPhase();
                drawSchedChart();
            }).catch(() => {});
        }

        function saveZeitplan() {
            const body = {
                enabled:     document.getElementById('schedEnabled').checked,
                num_ssr:     parseInt(document.getElementById('schedNumSsr').value),
                dawn_start:  timeToMin(document.getElementById('schedDawnStart').value),
                dawn_end:    timeToMin(document.getElementById('schedDawnEnd').value),
                dusk_start:  timeToMin(document.getElementById('schedDuskStart').value),
                dusk_end:    timeToMin(document.getElementById('schedDuskEnd').value),
            };
            const st = document.getElementById('schedSaveStatus');
            fetch('/api/scheduler/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) { schedDirtyFlag = false; setTimeout(()=>st.textContent='',2000); }
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // ===== CO2-Steuerung =====
        let co2ConfigLoaded = false;
        let co2PendingNodeId = null;  // aus Config geladen, aber Dropdown evtl. noch leer

        // Wird aus updateNodes() mit der aktuellen Liste der Steckdosen-Nodes (type===3)
        // aufgerufen, damit das Dropdown immer die tatsaechlich verfuegbaren Ziele zeigt.
        function updateCo2NodeOptions(steckdosenNodes) {
            const sel = document.getElementById('co2Node');
            if (!sel) return;
            const prevValue = sel.value;
            let html = '<option value="">' + t('noneOption') + '</option>';
            for (const n of steckdosenNodes) {
                html += `<option value="${n.id}">Node ${n.id}${n.online ? '' : ' (' + t('offlineSuffix') + ')'}</option>`;
            }
            sel.innerHTML = html;
            if (co2PendingNodeId !== null) {
                sel.value = String(co2PendingNodeId);
                co2PendingNodeId = null;
            } else if (prevValue) {
                sel.value = prevValue;
            }
        }

        function loadCo2Config() {
            if (co2ConfigLoaded) return;
            fetch('/api/co2').then(r => r.json()).then(d => {
                document.getElementById('co2Enabled').checked = d.enabled;
                document.getElementById('co2RelayBit').value  = d.target_relay_bit;
                document.getElementById('co2Min').value       = d.co2_min;
                document.getElementById('co2Max').value       = d.co2_max;
                co2PendingNodeId = d.target_node_id || null;
                document.getElementById('co2Node').value = co2PendingNodeId !== null ? String(co2PendingNodeId) : '';
                co2ConfigLoaded = true;
            }).catch(() => {});
        }

        function saveCo2ConfigForm() {
            const body = {
                enabled:          document.getElementById('co2Enabled').checked,
                target_node_id:   parseInt(document.getElementById('co2Node').value) || 0,
                target_relay_bit: parseInt(document.getElementById('co2RelayBit').value),
                co2_min:          parseInt(document.getElementById('co2Min').value),
                co2_max:          parseInt(document.getElementById('co2Max').value),
            };
            const st = document.getElementById('co2SaveStatus');
            fetch('/api/co2/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // Startseite: Statuskarte, nur sichtbar wenn die Funktion aktiv ist
        function updateCo2Status() {
            fetch('/api/co2').then(r => r.json()).then(d => {
                const card = document.getElementById('co2StatusCard');
                if (!card) return;
                if (!d.enabled) { card.style.display = 'none'; return; }
                card.style.display = 'block';
                const badge = document.getElementById('co2OutputBadge');
                if (badge) {
                    if (!d.co2_online) { badge.textContent = t('noCo2Value'); badge.style.background = '#dc3545'; }
                    else { badge.textContent = d.output_on ? t('on') : t('off'); badge.style.background = d.output_on ? '#28a745' : '#6c757d'; }
                }
                const nowEl = document.getElementById('co2StatusNow');
                if (nowEl) nowEl.textContent = d.co2_online ? d.co2_now : '--';
                const rangeEl = document.getElementById('co2StatusRange');
                if (rangeEl) rangeEl.textContent = d.co2_min + ' / ' + d.co2_max;
            }).catch(() => {});
        }

        // ===== Abluftluefter-Steuerung (MARS Hydro, RS485 Adresse 6) =====
        let fanConfigLoaded = false;
        function fanModeName(mode) {
            const names = [t('fanModeManual'), t('fanModeAutoHum'), t('fanModeAutoTemp')];
            return names[mode];
        }

        function updateFanModeVisibility() {
            const mode = parseInt(document.getElementById('fanMode').value);
            document.getElementById('fanManualSection').style.display = (mode === 0) ? 'block' : 'none';
            document.getElementById('fanHumSection').style.display    = (mode === 1) ? 'grid'  : 'none';
            document.getElementById('fanTempSection').style.display   = (mode === 2) ? 'grid'  : 'none';
            if (mode === 2) updateFanTempModeVisibility();
        }

        function updateFanTempModeVisibility() {
            const isDiff = parseInt(document.getElementById('fanTempMode').value) === 1;
            document.getElementById('fanTempMinLabel').textContent = isDiff ? t('diffMinLabel') : t('tempMinLabel');
            document.getElementById('fanTempMaxLabel').textContent = isDiff ? t('diffMaxLabel') : t('tempMaxLabel');
            document.getElementById('fanTempCapSection').style.display = isDiff ? 'none' : 'block';
        }

        function loadFanConfig() {
            if (fanConfigLoaded) return;
            fetch('/api/fan').then(r => r.json()).then(d => {
                document.getElementById('fanEnabled').checked      = d.enabled;
                document.getElementById('fanMode').value           = d.mode;
                document.getElementById('fanManualPercent').value  = d.manual_percent;
                document.getElementById('fanHumMin').value         = d.hum_min;
                document.getElementById('fanHumMax').value         = d.hum_max;
                document.getElementById('fanTempMin').value        = d.temp_min;
                document.getElementById('fanTempMax').value        = d.temp_max;
                document.getElementById('fanTempMode').value       = d.temp_mode;
                document.getElementById('fanTempCapDiff').value    = d.temp_cap_diff;
                document.getElementById('fanMinSpeed').value       = d.min_speed;
                updateFanModeVisibility();
                fanConfigLoaded = true;
            }).catch(() => {});
        }

        function saveFanConfigForm() {
            const body = {
                enabled:        document.getElementById('fanEnabled').checked,
                mode:           parseInt(document.getElementById('fanMode').value),
                manual_percent: parseInt(document.getElementById('fanManualPercent').value),
                hum_min:        parseInt(document.getElementById('fanHumMin').value),
                hum_max:        parseInt(document.getElementById('fanHumMax').value),
                temp_min:       parseInt(document.getElementById('fanTempMin').value),
                temp_max:       parseInt(document.getElementById('fanTempMax').value),
                temp_mode:      parseInt(document.getElementById('fanTempMode').value),
                temp_cap_diff:  parseInt(document.getElementById('fanTempCapDiff').value),
                min_speed:      parseInt(document.getElementById('fanMinSpeed').value),
            };
            const st = document.getElementById('fanSaveStatus');
            fetch('/api/fan/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // WLAN-Konfiguration
        function loadWifiConfigForm() {
            fetch('/api/wifi').then(r => r.json()).then(d => {
                document.getElementById('wifiSsid').value = d.ssid || '';
                document.getElementById('wifiApSsidHint').textContent = d.ap_ssid || '—';
                const hintEn = document.getElementById('wifiApSsidHintEn');
                if (hintEn) hintEn.textContent = d.ap_ssid || '—';
                const st = document.getElementById('wifiCurrentStatus');
                if (d.connected) {
                    st.textContent = t('wifiConnectedTo', d.ssid, d.ip);
                    st.style.color = '#27ae60';
                } else if (d.ap_active) {
                    st.textContent = t('fallbackApActive', d.ap_ssid, d.ap_ip);
                    st.style.color = '#e67e22';
                } else {
                    st.textContent = t('notConnected');
                    st.style.color = '#dc3545';
                }
            }).catch(() => {
                document.getElementById('wifiCurrentStatus').textContent = t('loadError');
            });
        }

        function saveWifiConfigForm() {
            const ssid = document.getElementById('wifiSsid').value.trim();
            const password = document.getElementById('wifiPassword').value;
            if (!ssid) { alert(t('ssidEmptyAlert')); return; }
            const st = document.getElementById('wifiSaveStatus');
            fetch('/api/wifi/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({ssid: ssid, password: password})
            }).then(r => r.json()).then(d => {
                if (d.success) {
                    st.style.color = '#27ae60';
                    st.textContent = t('savedRestarting');
                } else {
                    st.style.color = '#dc3545';
                    st.textContent = t('error');
                }
            }).catch(() => { st.style.color = '#dc3545'; st.textContent = t('connectionError'); });
        }

        // Daten-Log (SD-Karte): Dateiliste laden + Download anstossen
        function loadLogFileList() {
            const sel = document.getElementById('logFileSelect');
            if (!sel) return;
            fetch('/api/logs').then(r => r.json()).then(d => {
                const files = d.files || [];
                if (files.length === 0) { sel.innerHTML = '<option value="">' + t('noFilesFound') + '</option>'; return; }
                files.sort((a, b) => b.name.localeCompare(a.name));
                sel.innerHTML = files.map(f => `<option value="${f.name}">${f.name} (${(f.size/1024).toFixed(1)} KB)</option>`).join('');
            }).catch(() => { sel.innerHTML = '<option value="">' + t('loadError') + '</option>'; });
        }

        function downloadLogFile() {
            const sel = document.getElementById('logFileSelect');
            if (!sel || !sel.value) return;
            window.location.href = '/api/logs/download?file=' + encodeURIComponent(sel.value);
        }

        function deleteLogFile() {
            const sel = document.getElementById('logFileSelect');
            if (!sel || !sel.value) return;
            if (!confirm(t('confirmDeleteFile', sel.value))) return;
            fetch('/api/logs/delete', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({file: sel.value})
            }).then(r => r.json()).then(d => {
                if (d.success) loadLogFileList();
                else alert(t('deleteFailed'));
            }).catch(() => alert(t('connectionError')));
        }

        // Startseite: Drehzahl/Modus, sichtbar sobald der Luefter (RS485) online ist
        function updateFanStatus() {
            fetch('/api/fan').then(r => r.json()).then(d => {
                const badgeFan = document.getElementById('badge_fan');
                if (badgeFan) badgeFan.style.background = d.online ? '#28a745' : '#dc3545';

                const card = document.getElementById('fanStatusCard');
                if (!card) return;
                if (!d.online) { card.style.display = 'none'; return; }
                card.style.display = 'block';
                const badge = document.getElementById('fanModeBadge');
                if (badge) {
                    if (d.enabled) { badge.textContent = fanModeName(d.mode) || '--'; badge.style.background = '#28a745'; }
                    else { badge.textContent = t('externalControl'); badge.style.background = '#6c757d'; }
                }
                const pctEl = document.getElementById('fanPercent');
                if (pctEl) pctEl.textContent = d.percent + '%';
                const rpmEl = document.getElementById('fanRpm');
                if (rpmEl) rpmEl.textContent = d.rpm;
            }).catch(() => {});
        }

        // ===== Ventile =====
        let ventilData = null;
        let ventilKonfigGeladen = false;

        function updateVentile() {
            fetch('/api/ventile').then(r => r.json()).then(d => {
                ventilData = d;
                // Keys entsprechen den Roh-Zustandsnamen aus der API (nicht uebersetzen!),
                // nur die Anzeige-Labels sind sprachabhaengig.
                const farben = { 'Pause': '#6c757d', 'Vorlauf': '#ffc107', 'Aktiv': '#28a745' };
                const zustandLabel = { 'Pause': t('zustandPause'), 'Vorlauf': t('zustandVorlauf'), 'Aktiv': t('zustandAktiv') };
                const namen = [t('behaelter') + ' 1', t('behaelter') + ' 2', t('behaelter') + ' 3'];

                // Status (Startseite) — eine Karte, jeden Poll neu befuellt, nur aktive Behälter
                const statusContainer = document.getElementById('behaelterStatusCards');
                if (statusContainer) {
                    let statusHtml = '';
                    for (let i = 0; i < 3; i++) {
                        const b = d.behaelter[i];
                        if (!b.aktiv) continue;
                        const farbe = farben[b.zustand] || '#6c757d';
                        statusHtml += `<div style="display:flex;justify-content:space-between;align-items:center;margin-top:8px;">
                            <span style="font-size:0.9em;">${namen[i]}</span>
                            <span style="background:${farbe};color:white;padding:2px 10px;border-radius:10px;font-size:0.85em;">${zustandLabel[b.zustand] || b.zustand}</span>
                        </div>
                        <div style="font-size:0.78em;color:#888;">${t('timerLabel')}: ${b.timer_s} s</div>`;
                    }
                    statusContainer.innerHTML = statusHtml || '<div style="color:#aaa;font-size:0.85em;margin-top:8px;">' + t('noActiveReservoirs') + '</div>';
                }

                if (!ventilKonfigGeladen) {
                    // Einmalig: Konfig-Karten mit Eingabefeldern aufbauen (Konfiguration-Seite)
                    document.getElementById('vorlaufAktiv').checked = d.vorlauf_aktiv;
                    document.getElementById('vorlaufzeit').value    = d.vorlaufzeit_s;
                    let html = '';
                    for (let i = 0; i < 3; i++) {
                        const b = d.behaelter[i];
                        html += `<div class="status-box-info">
                            <strong>${namen[i]}</strong>
                            <div style="margin-top:10px;">
                                <label><input type="checkbox" id="aktiv${i}" ${b.aktiv ? 'checked' : ''}> ${t('active')}</label>
                            </div>
                            <div style="margin-top:8px; display:grid; grid-template-columns:1fr 1fr; gap:8px;">
                                <label style="font-size:0.9em;">${t('openingLabel')}<br>
                                    <input type="number" id="oeffnung${i}" min="1" max="60" value="${b.oeffnungszeit_s}" style="width:100%;padding:4px;">
                                </label>
                                <label style="font-size:0.9em;">${t('pauseLabel')}<br>
                                    <input type="number" id="pause${i}" min="1" max="60" value="${b.pausenzeit_min}" style="width:100%;padding:4px;">
                                </label>
                            </div>
                        </div>`;
                    }
                    document.getElementById('behaelterCards').innerHTML = html;
                    ventilKonfigGeladen = true;
                }
            }).catch(() => {});
        }

        function saveVentilConfig() {
            const body = {
                vorlauf_aktiv: document.getElementById('vorlaufAktiv').checked,
                vorlaufzeit_s: parseInt(document.getElementById('vorlaufzeit').value),
                behaelter: [0,1,2].map(i => ({
                    aktiv:           document.getElementById('aktiv' + i)?.checked ?? false,
                    oeffnungszeit_s: parseInt(document.getElementById('oeffnung' + i)?.value ?? 5),
                    pausenzeit_min:  parseInt(document.getElementById('pause' + i)?.value ?? 5)
                }))
            };
            fetch('/api/ventile/config', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(() => updateVentile());
        }

        // ===== Lichtsensor / Spektrum =====

        const WL_NM = [415, 445, 480, 515, 555, 590, 630, 680];
        const WL_COLORS = ['#9b59b6','#3498db','#00bcd4','#4caf50','#cddc39','#ff9800','#f44336','#b71c1c'];

        function drawSpektrumChart(canvasId, channels) {
            const canvas = document.getElementById(canvasId);
            if (!canvas) return;
            const ctx = canvas.getContext('2d');
            const W = canvas.width, H = canvas.height;
            ctx.clearRect(0, 0, W, H);
            ctx.fillStyle = '#111';
            ctx.fillRect(0, 0, W, H);

            const maxVal = Math.max(...channels, 1);
            const labelH = 30, topPad = 18;
            const chartH = H - labelH - topPad;
            const barW = W / WL_NM.length;
            const gap  = barW * 0.15;

            for (let i = 0; i < WL_NM.length; i++) {
                const x = i * barW;
                const frac = channels[i] / maxVal;
                const barH = Math.round(frac * chartH);
                const y = topPad + chartH - barH;
                const cx = x + barW / 2;

                const grad = ctx.createLinearGradient(0, y, 0, topPad + chartH);
                grad.addColorStop(0, WL_COLORS[i] + 'ff');
                grad.addColorStop(1, WL_COLORS[i] + '33');
                ctx.fillStyle = grad;
                ctx.fillRect(x + gap, y, barW - gap * 2, barH);

                const pct = Math.round(frac * 100);
                ctx.fillStyle = WL_COLORS[i];
                ctx.font = 'bold 10px sans-serif';
                ctx.textAlign = 'center';
                if (barH > 14) ctx.fillText(pct + '%', cx, y - 3);

                ctx.fillStyle = 'rgba(255,255,255,0.55)';
                ctx.font = '10px sans-serif';
                ctx.fillText(WL_NM[i], cx, H - 14);
                ctx.fillStyle = 'rgba(255,255,255,0.30)';
                ctx.font = '9px sans-serif';
                ctx.fillText('nm', cx, H - 4);
            }
        }

        function updateLicht() {
            fetch('/api/licht').then(r => r.json()).then(d => {
                // Dedizierter Spektrum-Node (reiches Format). Wenn keiner da ist, bleibt
                // lichtHasEspNowData massgeblich fuer die Sichtbarkeit (siehe updateNodes).
                lichtHasDedicatedData = !!(d.nodes && d.nodes.length > 0);
                updateLichtVisibility();
                if (!lichtHasDedicatedData) return;

                const n   = d.nodes[0];
                const ppfd = parseFloat(n.p);
                const el = id => document.getElementById(id);

                if (el('lichtPpfd')) el('lichtPpfd').textContent = ppfd.toFixed(1);
                const satBox = el('lichtSatBox');
                if (satBox) satBox.style.display = n.s ? '' : 'none';
                if (el('lichtAge'))  el('lichtAge').textContent  = n.age_s !== undefined ? t('agoSeconds', n.age_s) : '--';

                if (n.f && n.f.length === 8) drawSpektrumChart('spektrumChart', n.f);
            }).catch(() => {});
        }

        // ===== PCF8574 Test =====
        let pcfTestActive = false;
        let pcfState = 0;

        async function togglePcfTest() {
            pcfTestActive = !pcfTestActive;
            const r = await fetch('/api/pcf/test', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({testmode: pcfTestActive})
            });
            const d = await r.json();
            pcfState = d.state || 0;
            updatePcfUi();
        }

        async function pcfToggle(pin) {
            const on = !((pcfState >> pin) & 1);
            const r = await fetch('/api/pcf/test', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({pin: pin, on: on})
            });
            const d = await r.json();
            pcfState = d.state;
            updatePcfUi();
        }

        function updatePcfUi() {
            const toggleBtn = document.getElementById('pcfTestToggle');
            const warning   = document.getElementById('pcfTestWarning');
            const buttons   = document.getElementById('pcfTestButtons');
            if (!toggleBtn) return;
            if (pcfTestActive) {
                toggleBtn.textContent = t('stopTest');
                toggleBtn.style.background = '#28a745';
                warning.style.display = 'block';
                buttons.style.opacity = '1';
                buttons.style.pointerEvents = 'auto';
            } else {
                toggleBtn.textContent = t('startTest');
                toggleBtn.style.background = '#e74c3c';
                warning.style.display = 'none';
                buttons.style.opacity = '0.4';
                buttons.style.pointerEvents = 'none';
            }
            for (let i = 0; i < 8; i++) {
                const b = document.getElementById('pcf_btn_' + i);
                if (!b) continue;
                const on = (pcfState >> i) & 1;
                b.style.background = on ? '#28a745' : '#6c757d';
                b.querySelector('span').textContent = on ? t('on') : t('off');
            }
        }

        // ===== RS485 Sensoren =====

        function updateRs485() {
            fetch('/api/rs485').then(r => r.json()).then(d => {
                const ms2 = d.ms2;
                if (!ms2) return;
                // Eigener Messwerte-Block in der "Messwerte"-Karte, kein PPFD (siehe upsertMesswerteBlock)
                upsertMesswerteBlock('ms2', t('tentValues'),
                                     ms2.online, ms2.online ? ms2.temp : '--',
                                     ms2.online ? ms2.hum : '--', ms2.online ? ms2.co2 : '--');

                // PPFD/Spektrum vom RS485-Multisensor (MS2) — bei diesem Aufbau die
                // tatsaechliche Quelle des Lichtsensors (nicht ESP-NOW, nicht /api/licht).
                lichtHasMs2Data = !!(ms2.online && (ms2.ppfd !== undefined || (ms2.channels && ms2.channels.length === 8)));
                if (lichtHasMs2Data) {
                    const ppfdEl = document.getElementById('lichtPpfd');
                    if (ppfdEl && ms2.ppfd !== undefined) ppfdEl.textContent = parseFloat(ms2.ppfd).toFixed(1);
                    if (ms2.channels && ms2.channels.length === 8)
                        requestAnimationFrame(() => drawSpektrumChart('spektrumChart', ms2.channels));
                    const ageEl = document.getElementById('lichtAge');
                    if (ageEl && ms2.age_s !== undefined) ageEl.textContent = 'vor ' + ms2.age_s + ' s';
                }
                updateLichtVisibility();

                // Geraetestatus-Leiste (Fussbereich)
                const badgeMs2 = document.getElementById('badge_ms2');
                if (badgeMs2) badgeMs2.style.background = ms2.online ? '#28a745' : '#dc3545';
                const badgeLicht = document.getElementById('badge_licht');
                if (badgeLicht && d.licht) badgeLicht.style.background = d.licht.online ? '#28a745' : '#dc3545';
            }).catch(() => {});
        }

        setInterval(() => {
            if (!updateInProgress) {
                updateData();
                updateVentile();
                updateNodes();
                updateRs485();
                updateLicht();
                updateCo2Status();
                updateFanStatus();
                schedTick();
            }
        }, 2000);
        applyLangButtons();
        applyOptionTranslations();
        updateData();
        updateVentile();
        updateNodes();
        updateRs485();
        updateLicht();
        updateCo2Status();
        updateFanStatus();
        loadZeitplan();
    </script>
</body>
</html>
)rawliteral";
