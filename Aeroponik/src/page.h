// page.h - Web Interface für ESP32 Aeroponik
// HTML-Inhalt pro Tab liegt in eigenen Dateien (page_home.h, page_system.h, page_alarme.h,
// page_aeroponik.h, page_klima.h) und wird hier per #include als fortlaufendes Raw-String-Literal
// zusammengesetzt -- Laufzeitverhalten (eine einzige Seite, kein Nachladen) bleibt unveraendert.
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
                    🌡️ <span id="ahtTemp">--</span>°C &nbsp; 💧 <span id="ahtHum">--</span>%<span id="ens160Co2Wrap" style="display:none;"> &nbsp; 🫧 <span id="ens160Co2">--</span> ppm CO2</span>
                </div>
                <div id="netzRow" style="font-size:1em;white-space:nowrap;padding:3px 10px;border-radius:12px;background:#d4edda;color:#155724;">⚡ <span id="netzText"><span class="de">Netz OK</span><span class="en">Mains OK</span></span></div>
                <div style="font-size:1.1em;color:#555;white-space:nowrap;">🕐 <span id="datetime">--</span></div>
                <div class="lang-toggle">
                    <button class="lang-btn" data-lang-btn="de" onclick="setLang('de')">DE</button>
                    <button class="lang-btn" data-lang-btn="en" onclick="setLang('en')">EN</button>
                </div>
            </div>
        </div>

        <div class="tabs">
            <button class="tab active" onclick="showTab('startseite', this)">🏠 <span class="de">Startseite</span><span class="en">Home</span></button>
            <button class="tab"        onclick="showTab('system',     this)">⚙️ <span class="de">System</span><span class="en">System</span></button>
            <button class="tab"        onclick="showTab('alarme',     this)">🔔 <span class="de">Alarme</span><span class="en">Alarms</span></button>
            <button class="tab"        onclick="showTab('aeroponik',  this)">🚿 <span class="de">Aeroponik</span><span class="en">Aeroponics</span></button>
            <button class="tab"        onclick="showTab('klima',      this)">🌡️ <span class="de">Klima</span><span class="en">Climate</span></button>
        </div>

)rawliteral"
#include "page_home.h"
#include "page_system.h"
#include "page_alarme.h"
#include "page_aeroponik.h"
#include "page_klima.h"
R"rawliteral(

        <!-- Geraetestatus (immer sichtbar, unabhaengig vom aktiven Tab) -->
        <div id="deviceStatusBar" style="margin-top:20px;padding:10px 14px;background:#f8f9fa;border-radius:8px;font-size:0.82em;display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:center;color:#555;">
            <span id="wifiMode"><span class="de">Lade Verbindungsinfo...</span><span class="en">Loading connection info...</span></span>
            <span id="badge_ms2"   class="device-badge" style="background:#6c757d;">Multisensor-R</span>
            <span id="badge_licht" class="device-badge" style="background:#6c757d;"><span class="de">Licht</span><span class="en">Light</span><span id="badge_licht_suffix"></span></span>
            <span id="badge_fan"   class="device-badge" style="background:#6c757d;"><span class="de">Lüfter</span><span class="en">Fan</span><span id="badge_fan_suffix"></span></span>
            <span id="espnowBadges" style="display:contents;"></span>
        </div>
    </div>
)rawliteral"
#include "page_script.h"
R"rawliteral(
</body>
</html>
)rawliteral";
