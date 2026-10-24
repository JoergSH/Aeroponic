R"rawliteral(
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
                doseOnce: 'Einmal auslösen',
                doseRunning: (s) => 'Läuft... ' + s + 's',
                openingLabel: 'Öffnung (s)',
                pauseLabel: 'Pause (min)',
                tentValues: 'Zeltwerte',
                startTest: 'Test starten',
                stopTest: 'Test beenden',
                noneOption: '— keine —',
                offlineSuffix: 'offline',
                dsVorrat: 'Vorrat',
                dsPflanze: 'Pflanze',
                dsInnen: 'Innentemperatur',
                noSensorsFound: 'Keine Sensoren gefunden',
                phaseLicht: 'Lichtphase',
                phaseDunkel: 'Dunkelphase',
                noCo2Value: 'Kein CO2-Wert',
                noHumidValue: 'Kein Feuchte-Wert',
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
                flowcalInvalidVolume: 'Bitte eine gültige Menge in ml eingeben',
                flowmonResetConfirm: 'Alle Sollwerte wirklich löschen?',
                confirmRestart: 'Steuerung wirklich neu starten? Laufende Zyklen werden unterbrochen.',
                debugCaptureRunning: (s) => 'Zeichnet auf … noch ' + s + ' s',
                debugCaptureDone: 'Aufzeichnung fertig, siehe Dateiliste',
                savedRestarting: 'Gespeichert, Master startet neu…',
                notifyMs2Offline: 'RS485-Multisensor offline — Alarm greift nicht',
                co2SensorMissing: 'CO2-Sensor nicht gefunden/defekt',
                notifyTestSent: 'Testnachricht gesendet',
                noFilesFound: 'Keine Dateien gefunden',
                confirmDeleteFile: (name) => 'Datei "' + name + '" wirklich löschen?',
                confirmDeleteAllLogs: 'Wirklich ALLE Dateien auf der SD-Karte löschen? Das kann nicht rückgängig gemacht werden!',
                deleteAllDone: (n) => n + ' Datei(en) gelöscht',
                deleteFailed: 'Löschen fehlgeschlagen',
                externalControl: 'Extern (iControl)',
                saveBtn: 'Speichern',
                offlineLabel: 'Offline',
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
                doseOnce: 'Run once',
                doseRunning: (s) => 'Running... ' + s + 's',
                openingLabel: 'Opening (s)',
                pauseLabel: 'Pause (min)',
                tentValues: 'Tent Values',
                startTest: 'Start Test',
                stopTest: 'Stop Test',
                noneOption: '— none —',
                offlineSuffix: 'offline',
                dsVorrat: 'Reservoir',
                dsPflanze: 'Plant',
                dsInnen: 'Internal temperature',
                noSensorsFound: 'No sensors found',
                phaseLicht: 'Light phase',
                phaseDunkel: 'Dark phase',
                noCo2Value: 'No CO2 value',
                noHumidValue: 'No humidity value',
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
                flowcalInvalidVolume: 'Please enter a valid amount in ml',
                flowmonResetConfirm: 'Really clear all target values?',
                confirmRestart: 'Really restart the controller? Running cycles will be interrupted.',
                debugCaptureRunning: (s) => 'Recording… ' + s + ' s left',
                debugCaptureDone: 'Capture done, see file list',
                savedRestarting: 'Saved, master is restarting…',
                notifyMs2Offline: 'RS485 multisensor offline — alert will not trigger',
                co2SensorMissing: 'CO2 sensor missing/faulty',
                notifyTestSent: 'Test message sent',
                noFilesFound: 'No files found',
                confirmDeleteFile: (name) => 'Really delete file "' + name + '"?',
                confirmDeleteAllLogs: 'Really delete ALL files on the SD card? This cannot be undone!',
                deleteAllDone: (n) => n + ' file(s) deleted',
                deleteFailed: 'Delete failed',
                externalControl: 'External (iControl)',
                saveBtn: 'Save',
                offlineLabel: 'Offline',
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
            updateData(); updateVentile(); updateNodes(); updateRs485();
            updateCo2Status(); updateHumidStatus(); updateFanStatus(); updateDwcStatus(); updateRaumlichtStatus(); updateGehaeuseFanStatus(); updateLueftmodulStatus();
            loadTankKonfig(); loadZeitplan(); loadCo2Config(); loadHumidConfig(); loadDwcConfig(); loadRaumlichtConfig(); loadGehaeuseFanConfig(); loadUmwaelzConfig(); loadDruckSensorConfigForm(); loadFanConfig(); loadLueftmodulConfig(); rs485OtaStartPolling();
            loadWifiConfigForm(); loadLogFileList(); loadOutputConfig(); loadNotifyConfigForm();
            updateSchedPhase();
            updatePcfUi();
            updateFanModeVisibility();
        }

        function showTab(tabName, btn) {
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.getElementById(tabName + 'Tab').classList.add('active');
            btn.classList.add('active');
            if (tabName === 'system') {
                loadWifiConfigForm();
                loadLogFileList();
                loadGehaeuseFanConfig();
            } else if (tabName === 'alarme') {
                loadNotifyConfigForm();
            } else if (tabName === 'aeroponik') {
                loadTankKonfig();
                loadUmwaelzConfig();
                loadDruckSensorConfigForm();
            } else if (tabName === 'klima') {
                loadOutputConfig();
                loadZeitplan();
                loadCo2Config();
                loadHumidConfig();
                loadDwcConfig();
                loadRaumlichtConfig();
                loadFanConfig();
                loadLueftmodulConfig();
            }
        }

        function updateData() {
            if (updateInProgress) return;

            fetch('/api/data')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('datetime').textContent    = d.datetime || '--';
                    const netzRow = document.getElementById('netzRow');
                    if (netzRow && d.netzOk !== undefined) {
                        if (d.netzOk) {
                            netzRow.style.background = '#d4edda';
                            netzRow.style.color      = '#155724';
                            netzRow.innerHTML = '⚡ <span class="de">Netz OK</span><span class="en">Mains OK</span>';
                        } else {
                            netzRow.style.background = '#f8d7da';
                            netzRow.style.color      = '#721c24';
                            netzRow.innerHTML = '🔋 <span class="de">USV-Betrieb!</span><span class="en">On UPS!</span>';
                        }
                    }
                    document.getElementById('druckBar').textContent   = d.druckBar;
                    const dsb = document.getElementById('druckSensorBox');
                    if (dsb) dsb.style.display = d.druckSensorEnabled ? '' : 'none';
                    document.getElementById('tempVorrat').textContent  = d.tempVorrat.toFixed(1);
                    document.getElementById('tempPflanze').textContent = d.tempPflanze.toFixed(1);
                    document.getElementById('volumenLiter').textContent = d.volumenLiter;
                    const tsl = document.getElementById('tankSensorLive');
                    if (tsl && d.distanzMM !== undefined) tsl.textContent = d.distanzMM + ' mm';
                    const tdl = document.getElementById('tankDruckLive');
                    if (tdl && d.tankDruckRoh !== undefined) tdl.textContent = d.tankDruckRoh;

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
                    const co2Wrap = document.getElementById('ens160Co2Wrap');
                    if (co2Wrap) {
                        co2Wrap.style.display = d.ens160Ok ? 'inline' : 'none';
                        if (d.ens160Ok) document.getElementById('ens160Co2').textContent = d.ens160Co2;
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
                        document.getElementById('fwVersion').textContent  = d.fwVersion || '--';
                        document.getElementById('chipModel').textContent   = d.chipModel;
                        document.getElementById('chipRevision').textContent = d.chipRevision;
                        document.getElementById('flashSize').textContent   = Math.round(d.flashSize / (1024 * 1024)) + ' MB';
                        document.getElementById('sketchSize').textContent  = Math.round(d.sketchSize / 1024) + ' KB';
                        const rr = document.getElementById('lastResetReason');
                        if (rr && d.lastResetReason) rr.textContent = d.lastResetReason;
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

        // PPFD-Karte hat zwei moegliche Quellen: RS485-Multisensor (MS2, /api/rs485 —
        // bei diesem Aufbau die tatsaechliche Quelle), oder ein ESP-NOW-Sensor-Node (/api/espnow).
        // Beide laufen unabhaengig/asynchron — nur ausblenden wenn WIRKLICH beide leer sind.
        let lichtHasMs2Data       = false;
        let lichtHasEspNowData    = false;
        function updateLichtVisibility() {
            const dataEl = document.getElementById('lichtsensorData');
            if (dataEl) dataEl.style.display = (lichtHasMs2Data || lichtHasEspNowData) ? 'block' : 'none';
        }

        // ===== Messwerte-Bloecke (Startseite): Temp/Hum/VPD/CO2, kein PPFD =====
        // Ein Block pro Quelle (ESP-NOW-Sensor-Node oder RS485-MS2), alle in
        // derselben "Messwerte"-Karte statt eigener Karten pro Quelle.
        const messwerteBuilt = new Set();

        // Zelt-Multisensor: EIN gemeinsamer "Zeltwerte"-Block unabhaengig von der Anbindung
        // (RS485-Multisensor2 ODER ESP-NOW-Multisensor-Node) — beide Quellen laufen
        // unabhaengig/asynchron und schreiben hierher, renderZeltCard() entscheidet welche
        // gerade angezeigt wird (online bevorzugt, sonst die zuletzt gesehene).
        // co2Ok: nur bei RS485 belastbar (eigenes Status-Byte vom Multisensor, siehe
        // /api/rs485 co2_sensor_ok) — ueber ESP-NOW gibt es dafuer aktuell kein Signal,
        // bleibt dort also immer true (keine Fehlwarnung).
        let zeltRs485  = { online: false, seen: false, temp: '--', hum: '--', co2: '--', co2Ok: true };
        let zeltEspNow = { online: false, seen: false, temp: '--', hum: '--', co2: '--', co2Ok: true };
        function renderZeltCard() {
            let src = zeltRs485.online ? zeltRs485
                    : zeltEspNow.online ? zeltEspNow
                    : zeltRs485.seen    ? zeltRs485
                    : zeltEspNow.seen   ? zeltEspNow
                    : zeltRs485;
            const warn = (src.online && !src.co2Ok) ? t('co2SensorMissing') : '';
            upsertMesswerteBlock('zelt', t('tentValues'), src.online, src.temp, src.hum, src.co2, warn);
        }

        // VPD (Vapor Pressure Deficit) aus Temperatur+Feuchte — Tetens-Formel,
        // ohne Blatt-Temperatur-Offset (einfache/haeufigste Variante).
        function computeVpdDisplay(temp, hum) {
            const t = parseFloat(temp), h = parseFloat(hum);
            if (isNaN(t) || isNaN(h)) return '--';
            const svp = 0.6108 * Math.exp((17.27 * t) / (t + 237.3));  // kPa
            const vpd = svp * (1 - h / 100);
            return vpd.toFixed(2);
        }

        function messwerteBlockHtml(key, label, online, temp, hum, co2, vpd, warn) {
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
                <div id="msw_warn_${key}" style="display:${warn?'block':'none'};margin-top:6px;font-size:0.75em;color:#dc3545;">⚠ ${warn||''}</div>
            </div>`;
        }

        function upsertMesswerteBlock(key, label, online, temp, hum, co2, warn) {
            const vpd = computeVpdDisplay(temp, hum);
            const container = document.getElementById('messwerteExtra');
            if (!container) return;
            let block = document.getElementById('msw_block_' + key);
            if (!block) {
                const wrapper = document.createElement('div');
                wrapper.innerHTML = messwerteBlockHtml(key, label, online, temp, hum, co2, vpd, warn);
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
            const warnEl = document.getElementById('msw_warn_' + key);
            if (warnEl) {
                warnEl.style.display = warn ? 'block' : 'none';
                warnEl.textContent = warn ? ('⚠ ' + warn) : '';
            }
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
                // (der gemeinsame "Zeltwerte"-Block bleibt davon unabhaengig immer bestehen,
                // siehe renderZeltCard/updateRs485)
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
                    // Messwerte (Startseite) — nur Temp/Hum/CO2, kein PPFD/Spektrum. Multisensor-
                    // Nodes (type 2) landen im gemeinsamen "Zeltwerte"-Block (siehe renderZeltCard),
                    // nicht in einem eigenen Node-Block — sonst erscheint der Multisensor doppelt,
                    // je nachdem ob er gerade per RS485 oder ESP-NOW angebunden ist.
                    if (n.type === 2 && n.temperature !== undefined) {
                        zeltEspNow.online = !!n.online;
                        zeltEspNow.seen   = true;
                        zeltEspNow.temp   = n.online ? n.temperature : '--';
                        zeltEspNow.hum    = n.online ? n.humidity    : '--';
                        zeltEspNow.co2    = n.online ? n.co2         : '--';
                        renderZeltCard();
                    } else if (n.temperature !== undefined) {
                        const typName = nodeTypeName(n.type) || 'Node';
                        upsertMesswerteBlock('node' + n.id, typName + ' (Node ' + n.id + ')',
                                             n.online, n.temperature, n.humidity, n.co2);
                    }
                    // PPFD/Spektrum — bei den meisten Aufbauten steckt der Lichtsensor
                    // im Multisensor (NODE_TYPE_SENSOR, einfaches Format).
                    if (n.ppfd !== undefined || (n.spektrum && n.spektrum.length === 8)) {
                        lichtHasEspNowData = true;
                        const ppfdEl = document.getElementById('lichtPpfd');
                        if (ppfdEl && n.ppfd !== undefined) ppfdEl.textContent = parseFloat(n.ppfd).toFixed(1);
                        if (n.spektrum && n.spektrum.length === 8)
                            requestAnimationFrame(() => drawSpektrumChart('spektrumChart', n.spektrum));
                        const ageEl = document.getElementById('lichtAge');
                        if (ageEl && n.age_s !== undefined) ageEl.textContent = t('agoSeconds', n.age_s);
                        const missBox = document.getElementById('lichtMissingBox');
                        if (missBox) missBox.style.display = (n.light_sensor_ok === false) ? '' : 'none';
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
                updateHumidNodeOptions((d.nodes || []).filter(n => n.type === 3));
                updateDwcNodeOptions((d.nodes || []).filter(n => n.type === 3));
                updateRaumlichtNodeOptions((d.nodes || []).filter(n => n.type === 3));
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
                el.textContent = (nodeTypeName(n.type) || ('Node ' + n.id)) + '-E';
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

        // ===== Multimodul-Update ueber RS485 =====
        // Upload -> Master puffert im PSRAM -> loopRS485() uebertraegt blockweise; Fortschritt per
        // /api/rs485/ota/status pollen, bis "done"/"error"/"idle".
        let rs485OtaTimer = null;
        const otaL = (de, en) => currentLang === 'en' ? en : de;

        function rs485OtaShow(busy) {
            document.getElementById('rs485OtaBtn').disabled = busy;
            document.getElementById('rs485OtaAbortBtn').style.display = busy ? 'inline-block' : 'none';
        }

        function rs485OtaSetStatus(text, color) {
            const el = document.getElementById('rs485OtaStatus');
            el.style.color = color || '';
            el.textContent = text;
        }

        async function rs485OtaPoll() {
            let d;
            try {
                d = await (await fetch('/api/rs485/ota/status')).json();
            } catch (e) {
                return;   // Master kurz nicht erreichbar -- naechster Versuch
            }
            if (d.version) document.getElementById('rs485OtaVersion').textContent = d.version;
            const fill = document.getElementById('rs485OtaFill');
            const pct = d.total ? Math.floor(d.sent * 100 / d.total) : 0;
            fill.style.width = pct + '%';
            fill.textContent = pct + '%';
            const phase = {
                info:   otaL('Verbinde mit Multimodul...', 'Connecting to Multimodul...'),
                begin:  otaL('Update wird vorbereitet...', 'Preparing update...'),
                data:   otaL('Übertrage ', 'Transferring ') + Math.round(d.sent / 1024) + ' / ' + Math.round(d.total / 1024) + ' KB',
                end:    otaL('Prüfe Firmware (MD5)...', 'Verifying firmware (MD5)...'),
                reboot: otaL('Multimodul startet neu...', 'Multimodul is restarting...'),
                version: otaL('Frage Version ab...', 'Querying version...')
            };
            if (d.busy) {
                rs485OtaShow(true);
                rs485OtaSetStatus(phase[d.state] || d.state);
                return;
            }
            clearInterval(rs485OtaTimer);
            rs485OtaTimer = null;
            rs485OtaShow(false);
            if (d.state === 'done') {
                fill.style.width = '100%';
                fill.textContent = '100%';
                rs485OtaSetStatus(otaL('✅ Update erfolgreich – Version ', '✅ Update successful – version ') + (d.version || '?') +
                                  (d.version_before ? otaL(' (vorher ', ' (before ') + d.version_before + ')' : '') +
                                  (d.error ? ' – ' + d.error : ''), '#28a745');
            } else if (d.state === 'error') {
                rs485OtaSetStatus('❌ ' + (d.error || otaL('Fehler', 'Error')), '#dc3545');
            } else {
                rs485OtaSetStatus(d.version ? '' : (d.error || ''), d.error ? '#dc3545' : '');
            }
        }

        function rs485OtaStartPolling() {
            if (!rs485OtaTimer) rs485OtaTimer = setInterval(rs485OtaPoll, 1000);
            rs485OtaPoll();
        }

        async function rs485OtaQueryVersion() {
            document.getElementById('rs485OtaVersion').textContent = '…';
            await fetch('/api/rs485/ota/version', {method: 'POST'});
            rs485OtaStartPolling();
        }

        async function rs485OtaUpload() {
            const input = document.getElementById('rs485OtaFile');
            const file = input.files[0];
            if (!file) { rs485OtaSetStatus(otaL('Bitte zuerst eine .bin-Datei wählen', 'Please choose a .bin file first'), '#dc3545'); return; }
            if (!confirm(otaL('Multimodul-Firmware jetzt über RS485 aktualisieren?', 'Update the Multimodul firmware via RS485 now?'))) return;
            rs485OtaShow(true);
            document.getElementById('rs485OtaProgress').style.display = 'block';
            rs485OtaSetStatus(otaL('Lade Datei auf den Master hoch...', 'Uploading file to the master...'));
            const fd = new FormData();
            fd.append('firmware', file, 'firmware.bin');
            try {
                const r = await fetch('/api/rs485/ota/upload', {method: 'POST', body: fd});
                const d = await r.json();
                if (!d.ok) throw new Error(d.error || ('HTTP ' + r.status));
                rs485OtaStartPolling();
            } catch (e) {
                rs485OtaShow(false);
                rs485OtaSetStatus('❌ ' + e.message, '#dc3545');
            }
            input.value = '';
        }

        async function rs485OtaAbort() {
            await fetch('/api/rs485/ota/abort', {method: 'POST'});
            rs485OtaPoll();
        }

        // ===== Tank =====
        let tankKonfigGeladen = false;

        function toggleTankSensorUI() {
            const istDruck = document.getElementById('tankSensorTyp').value === '1';
            document.getElementById('ultraschallKalibrierung').hidden = istDruck;
            document.getElementById('druckKalibrierung').hidden       = !istDruck;
        }

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
                document.getElementById('tankSensorTyp').value   = d.sensor_type;
                document.getElementById('druckNull').value       = d.druck_null_raw;
                document.getElementById('druckProMm').value      = d.druck_counts_pro_mm;
                toggleTankSensorUI();
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
                offset_mm:       parseInt(document.getElementById('sensorOffset').value),
                sensor_type:         parseInt(document.getElementById('tankSensorTyp').value),
                druck_null_raw:      parseInt(document.getElementById('druckNull').value),
                druck_counts_pro_mm: parseFloat(document.getElementById('druckProMm').value)
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
        // Aktueller Licht-Ausgang (0=Lichtx4 gestuft, 1=Analogmodul stufenlos), von
        // loadOutputConfig() aktuell gehalten — bestimmt ob der Chart Treppen oder Kurve zeigt.
        let outputLightMode = 0;
        let outputFanMode   = 0;

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

        // Spiegelt schedComputeBrightnessPercent() aus scheduler.cpp: stufenlose Rampe
        // 0-100%, genutzt fuer den Chart wenn Licht-Ausgang = Analogmodul (kein n-Kanal-Stufen).
        function schedComputeBrightnessPercent(nowMin, dawnStart, dawnEnd, duskStart, duskEnd) {
            if (nowMin < dawnStart || nowMin >= duskEnd) return 0;
            if (nowMin >= dawnEnd && nowMin < duskStart) return 100;
            let frac;
            if (nowMin < dawnEnd) frac = (nowMin - dawnStart) / (dawnEnd - dawnStart);
            else frac = 1 - (nowMin - duskStart) / (duskEnd - duskStart);
            frac = Math.max(0, Math.min(1, frac));
            return Math.round(frac * 100);
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
                if (outputLightMode >= 1) {
                    const pct = schedComputeBrightnessPercent(m, dawnStart, dawnEnd, duskStart, duskEnd);
                    return H - 22 - (pct / 100) * (H - 34);
                }
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

            let phaseKey = 'phaseNight', isTransition = false, isDay = false;
            if (schedNowMin >= dawnStart && schedNowMin < dawnEnd)   { phaseKey = 'phaseDawn'; isTransition = true; }
            else if (schedNowMin >= dawnEnd && schedNowMin < duskStart) { phaseKey = 'phaseDay'; isDay = true; }
            else if (schedNowMin >= duskStart && schedNowMin < duskEnd) { phaseKey = 'phaseDusk'; isTransition = true; }
            el.style.background = isDay ? '#fff3cd' : (isTransition ? '#d4edda' : '#e2e3e5');
            el.style.color = '#333';
            if (outputLightMode == 1) {
                const pct = schedComputeBrightnessPercent(schedNowMin, dawnStart, dawnEnd, duskStart, duskEnd);
                el.textContent = t(phaseKey) + '  ' + pct + '%';
            } else {
                const active = schedComputeActive(schedNowMin, n, dawnStart, dawnEnd, duskStart, duskEnd);
                el.textContent = t(phaseKey) + '  ' + t('channelsLabel') + ' ' + active + '/' + n;
            }
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

        // Startseite: manuelle Helligkeit statt Zeitrampe, wenn der Zeitplan deaktiviert ist
        // (gleiches Muster wie die manuelle Luefter-Leistung).
        let schedManualDragging = false;

        function onSchedManualSliderInput() {
            schedManualDragging = true;
            const v = document.getElementById('schedManualSlider').value;
            document.getElementById('schedManualValue').textContent = v + '%';
        }

        function onSchedManualSliderChange() {
            const v = parseInt(document.getElementById('schedManualSlider').value);
            fetch('/api/scheduler/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({manual_percent: v})
            }).then(() => { schedManualDragging = false; })
              .catch(() => { schedManualDragging = false; });
        }

        // Periodischer Status-Poll: entscheidet, ob Chart+Phase oder die manuelle
        // Helligkeit gezeigt wird. Bewusst getrennt von loadZeitplan()/schedLoaded
        // (dort werden nur die Formularfelder auf dem Parameter-Tab einmalig geladen,
        // damit unbestaetigte Eingaben dort nicht ueberschrieben werden).
        function updateSchedManualStatus() {
            fetch('/api/scheduler').then(r => r.json()).then(d => {
                const manualRow = document.getElementById('schedManualRow');
                const phaseRow  = document.getElementById('schedPhaseRow');
                const chart     = document.getElementById('schedChart');
                if (manualRow) manualRow.style.display = d.enabled ? 'none' : 'block';
                if (phaseRow)  phaseRow.style.display  = d.enabled ? 'flex' : 'none';
                if (chart)     chart.style.display     = d.enabled ? 'block' : 'none';
                if (!d.enabled && !schedManualDragging) {
                    const sl = document.getElementById('schedManualSlider');
                    if (sl) sl.value = d.manual_percent;
                    const valEl = document.getElementById('schedManualValue');
                    if (valEl) valEl.textContent = d.manual_percent + '%';
                }
            }).catch(() => {});
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
        // CO2- und Luefter-Abschnitt teilen sich eine Box (Nutzerwunsch) — jeder Abschnitt
        // blendet sich unabhaengig ein/aus, die Box selbst ist sichtbar sobald mindestens
        // einer der beiden etwas zu zeigen hat. Der Luefter-Abschnitt bekommt nur dann eine
        // sichtbare Trennlinie, wenn der CO2-Abschnitt gerade auch angezeigt wird.
        function refreshCo2FanCardVisibility() {
            const card = document.getElementById('co2FanStatusCard');
            if (!card) return;
            const co2Section = document.getElementById('co2StatusSection');
            const fanSection = document.getElementById('fanStatusSection');
            const co2Visible = co2Section && co2Section.style.display !== 'none';
            const fanVisible = fanSection && fanSection.style.display !== 'none';
            card.style.display = (co2Visible || fanVisible) ? 'block' : 'none';
            if (fanSection) fanSection.style.borderTop = co2Visible ? '1px solid #e0e0e0' : 'none';
        }

        function updateCo2Status() {
            fetch('/api/co2').then(r => r.json()).then(d => {
                const section = document.getElementById('co2StatusSection');
                if (!section) return;
                section.style.display = d.enabled ? 'block' : 'none';
                refreshCo2FanCardVisibility();
                if (!d.enabled) return;
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

        // ===== Luftbefeuchtung-Steuerung — 1:1 Kopie der CO2-Steuerung =====
        let humidConfigLoaded = false;
        let humidPendingNodeId = null;  // aus Config geladen, aber Dropdown evtl. noch leer

        // Wird aus updateNodes() mit der aktuellen Liste der Steckdosen-Nodes (type===3)
        // aufgerufen, damit das Dropdown immer die tatsaechlich verfuegbaren Ziele zeigt.
        function updateHumidNodeOptions(steckdosenNodes) {
            const sel = document.getElementById('humidNode');
            if (!sel) return;
            const prevValue = sel.value;
            let html = '<option value="">' + t('noneOption') + '</option>';
            for (const n of steckdosenNodes) {
                html += `<option value="${n.id}">Node ${n.id}${n.online ? '' : ' (' + t('offlineSuffix') + ')'}</option>`;
            }
            sel.innerHTML = html;
            if (humidPendingNodeId !== null) {
                sel.value = String(humidPendingNodeId);
                humidPendingNodeId = null;
            } else if (prevValue) {
                sel.value = prevValue;
            }
        }

        // humid_min/humid_max kommen vom Geraet als %*10 (Ganzzahl ohne Float im EEPROM) --
        // im UI wird mit einer Nachkommastelle in echten Prozent gearbeitet.
        function loadHumidConfig() {
            if (humidConfigLoaded) return;
            fetch('/api/humid').then(r => r.json()).then(d => {
                document.getElementById('humidEnabled').checked = d.enabled;
                document.getElementById('humidRelayBit').value  = d.target_relay_bit;
                document.getElementById('humidMin').value       = (d.humid_min / 10).toFixed(1);
                document.getElementById('humidMax').value       = (d.humid_max / 10).toFixed(1);
                humidPendingNodeId = d.target_node_id || null;
                document.getElementById('humidNode').value = humidPendingNodeId !== null ? String(humidPendingNodeId) : '';
                humidConfigLoaded = true;
            }).catch(() => {});
        }

        function saveHumidConfigForm() {
            const body = {
                enabled:          document.getElementById('humidEnabled').checked,
                target_node_id:   parseInt(document.getElementById('humidNode').value) || 0,
                target_relay_bit: parseInt(document.getElementById('humidRelayBit').value),
                humid_min:        Math.round(parseFloat(document.getElementById('humidMin').value) * 10),
                humid_max:        Math.round(parseFloat(document.getElementById('humidMax').value) * 10),
            };
            const st = document.getElementById('humidSaveStatus');
            fetch('/api/humid/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        function updateHumidStatus() {
            fetch('/api/humid').then(r => r.json()).then(d => {
                const card = document.getElementById('humidStatusCard');
                if (!card) return;
                card.style.display = d.enabled ? 'block' : 'none';
                if (!d.enabled) return;
                const badge = document.getElementById('humidOutputBadge');
                if (badge) {
                    if (!d.humid_online) { badge.textContent = t('noHumidValue'); badge.style.background = '#dc3545'; }
                    else { badge.textContent = d.output_on ? t('on') : t('off'); badge.style.background = d.output_on ? '#28a745' : '#6c757d'; }
                }
                const nowEl = document.getElementById('humidStatusNow');
                if (nowEl) nowEl.textContent = d.humid_online ? d.humid_now : '--';
                const rangeEl = document.getElementById('humidStatusRange');
                if (rangeEl) rangeEl.textContent = (d.humid_min / 10).toFixed(1) + ' / ' + (d.humid_max / 10).toFixed(1);
            }).catch(() => {});
        }

        // ===== DWC-Beleuchtungstimer =====
        let dwcConfigLoaded  = false;
        let dwcPendingNodeId = null;  // aus Config geladen, aber Dropdown evtl. noch leer

        // Wird aus updateNodes() mit der aktuellen Liste der Steckdosen-Nodes (type===3)
        // aufgerufen, damit das Dropdown immer die tatsaechlich verfuegbaren Ziele zeigt.
        function updateDwcNodeOptions(steckdosenNodes) {
            const sel = document.getElementById('dwcNode');
            if (!sel) return;
            const prevValue = sel.value;
            let html = '<option value="">' + t('noneOption') + '</option>';
            for (const n of steckdosenNodes) {
                html += `<option value="${n.id}">Node ${n.id}${n.online ? '' : ' (' + t('offlineSuffix') + ')'}</option>`;
            }
            sel.innerHTML = html;
            if (dwcPendingNodeId !== null) {
                sel.value = String(dwcPendingNodeId);
                dwcPendingNodeId = null;
            } else if (prevValue) {
                sel.value = prevValue;
            }
        }

        function loadDwcConfig() {
            if (dwcConfigLoaded) return;
            fetch('/api/dwc').then(r => r.json()).then(d => {
                document.getElementById('dwcEnabled').checked  = d.enabled;
                document.getElementById('dwcRelayBit').value   = d.target_relay_bit;
                document.getElementById('dwcOnTime').value     = minToTime(d.on_min);
                document.getElementById('dwcOffTime').value    = minToTime(d.off_min);
                dwcPendingNodeId = d.target_node_id || null;
                document.getElementById('dwcNode').value = dwcPendingNodeId !== null ? String(dwcPendingNodeId) : '';
                dwcConfigLoaded = true;
            }).catch(() => {});
        }

        function saveDwcConfigForm() {
            const body = {
                enabled:          document.getElementById('dwcEnabled').checked,
                target_node_id:   parseInt(document.getElementById('dwcNode').value) || 0,
                target_relay_bit: parseInt(document.getElementById('dwcRelayBit').value),
                on_min:           timeToMin(document.getElementById('dwcOnTime').value),
                off_min:          timeToMin(document.getElementById('dwcOffTime').value),
            };
            const st = document.getElementById('dwcSaveStatus');
            fetch('/api/dwc/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // Startseite: eigene Statuskarte (separates System von der Zelt-Klimasteuerung),
        // nur sichtbar wenn der Timer aktiv ist.
        function updateDwcStatus() {
            fetch('/api/dwc').then(r => r.json()).then(d => {
                const card = document.getElementById('dwcStatusCard');
                if (!card) return;
                card.style.display = d.enabled ? 'block' : 'none';
                if (!d.enabled) return;
                const badge = document.getElementById('dwcOutputBadge');
                if (badge) {
                    badge.textContent = d.output_on ? t('on') : t('off');
                    badge.style.background = d.output_on ? '#28a745' : '#6c757d';
                }
                const info = document.getElementById('dwcTimerInfo');
                if (info) info.textContent = minToTime(d.on_min) + ' – ' + minToTime(d.off_min);
            }).catch(() => {});
        }

        // ===== Raumlicht (manueller Schalter) =====
        let raumlichtConfigLoaded  = false;
        let raumlichtPendingNodeId = null;  // aus Config geladen, aber Dropdown evtl. noch leer

        function updateRaumlichtNodeOptions(steckdosenNodes) {
            const sel = document.getElementById('raumlichtNode');
            if (!sel) return;
            const prevValue = sel.value;
            let html = '<option value="">' + t('noneOption') + '</option>';
            for (const n of steckdosenNodes) {
                html += `<option value="${n.id}">Node ${n.id}${n.online ? '' : ' (' + t('offlineSuffix') + ')'}</option>`;
            }
            sel.innerHTML = html;
            if (raumlichtPendingNodeId !== null) {
                sel.value = String(raumlichtPendingNodeId);
                raumlichtPendingNodeId = null;
            } else if (prevValue) {
                sel.value = prevValue;
            }
        }

        function loadRaumlichtConfig() {
            if (raumlichtConfigLoaded) return;
            fetch('/api/raumlicht').then(r => r.json()).then(d => {
                document.getElementById('raumlichtRelayBit').value = d.target_relay_bit;
                raumlichtPendingNodeId = d.target_node_id || null;
                document.getElementById('raumlichtNode').value = raumlichtPendingNodeId !== null ? String(raumlichtPendingNodeId) : '';
                raumlichtConfigLoaded = true;
            }).catch(() => {});
        }

        function saveRaumlichtConfigForm() {
            const body = {
                target_node_id:   parseInt(document.getElementById('raumlichtNode').value) || 0,
                target_relay_bit: parseInt(document.getElementById('raumlichtRelayBit').value),
            };
            const st = document.getElementById('raumlichtSaveStatus');
            fetch('/api/raumlicht/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // Startseite: Karte nur sichtbar, sobald ein Ausgang ausgewaehlt wurde (target_node_id != 0).
        function updateRaumlichtStatus() {
            fetch('/api/raumlicht').then(r => r.json()).then(d => {
                const card = document.getElementById('raumlichtStatusCard');
                if (!card) return;
                card.style.display = d.target_node_id ? 'block' : 'none';
                if (!d.target_node_id) return;

                const offlineBadge = document.getElementById('raumlichtOfflineBadge');
                if (offlineBadge) offlineBadge.style.display = d.online ? 'none' : 'inline-block';

                const btn = document.getElementById('raumlichtToggleBtn');
                if (btn) {
                    btn.disabled = !d.online;
                    btn.style.opacity = d.online ? '1' : '0.5';
                    btn.style.background = d.output_on ? '#28a745' : '#6c757d';
                    btn.style.color = 'white';
                    btn.textContent = d.output_on ? t('on') : t('off');
                }
            }).catch(() => {});
        }

        function toggleRaumlicht() {
            fetch('/api/raumlicht/toggle', { method: 'POST' }).then(updateRaumlichtStatus);
        }

        // ===== Gehäuselüfter =====
        // sensor_available: Innentemperatur-DS18B20 zugewiesen (Homepage-Zeile).
        // fan_available: PWM-Lüfterausgang vorhanden (Config-Abschnitt im Parameter-Tab).
        let gehaeuseFanConfigLoaded = false;

        function updateGehaeuseFanStatus() {
            fetch('/api/gehaeuse').then(r => r.json()).then(d => {
                const sensorAvail = !!d.sensor_available;
                const fanAvail    = !!d.fan_available;
                const row = document.getElementById('gehaeuseFanRow');
                const section = document.getElementById('gehaeuseFanSection');
                if (row) row.style.display = (sensorAvail || fanAvail) ? 'flex' : 'none';
                if (section) section.style.display = fanAvail ? 'block' : 'none';

                const tVal = (d.temp_innen !== undefined) ? d.temp_innen : '--';
                const ti = document.getElementById('gehTempInnen');  if (ti) ti.textContent = tVal;

                const badge = document.getElementById('gehaeuseFanBadge');
                if (badge) {
                    if (fanAvail) {
                        badge.style.display = '';
                        badge.textContent = d.on ? t('on') : t('off');
                        badge.style.background = d.on ? '#28a745' : '#6c757d';
                    } else {
                        badge.style.display = 'none';
                    }
                }
                if (fanAvail) {
                    const go = document.getElementById('gehOnOff'); if (go) go.textContent = d.on ? t('on') : t('off');
                }
            }).catch(() => {});
        }

        function loadGehaeuseFanConfig() {
            if (gehaeuseFanConfigLoaded) return;
            fetch('/api/gehaeuse').then(r => r.json()).then(d => {
                if (!d.fan_available) return;
                document.getElementById('gehEnabled').checked = d.enabled;
                document.getElementById('gehTempMin').value   = d.temp_min;
                document.getElementById('gehTempMax').value   = d.temp_max;
                gehaeuseFanConfigLoaded = true;
            }).catch(() => {});
        }

        function saveGehaeuseFanForm() {
            const body = {
                enabled:   document.getElementById('gehEnabled').checked,
                temp_min:  parseInt(document.getElementById('gehTempMin').value),
                temp_max:  parseInt(document.getElementById('gehTempMax').value),
            };
            const st = document.getElementById('gehSaveStatus');
            fetch('/api/gehaeuse/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // ===== Zeltlüfter-Steuerung (Lueftermodul, RS485 0x51) =====
        let lueftmodulConfigLoaded = false;
        const lueftPercent = [0, 0, 0, 0];

        function updateLueftSliderVisibility() {
            const same = document.getElementById('lueftGleicheWerte').checked;
            document.getElementById('lueftSliderSingleRow').style.display = same ? 'block' : 'none';
            document.getElementById('lueftSlidersMulti').style.display   = same ? 'none'  : 'grid';
        }

        function onLueftSliderInput(idx) {
            if (idx === -1) {
                const v = parseInt(document.getElementById('lueftSliderSingle').value);
                document.getElementById('lueftSliderSingleValue').textContent = v + '%';
                for (let i = 0; i < 4; i++) {
                    lueftPercent[i] = v;
                    const s = document.getElementById('lueftSlider' + i);
                    const l = document.getElementById('lueftSlider' + i + 'Value');
                    if (s) s.value = v;
                    if (l) l.textContent = v + '%';
                }
            } else {
                const v = parseInt(document.getElementById('lueftSlider' + idx).value);
                document.getElementById('lueftSlider' + idx + 'Value').textContent = v + '%';
                lueftPercent[idx] = v;
            }
        }
        function onLueftSliderChange() { /* Uebernahme erst beim Speichern-Klick */ }

        // Leichtgewicht-Statuspoll (Online/RPM) — fasst KEINE Formularfelder an, damit eine
        // unbestaetigte Nutzereingabe nicht durch den naechsten Poll ueberschrieben wird.
        function updateLueftmodulStatus() {
            fetch('/api/lueftmodul').then(r => r.json()).then(d => {
                const badge = document.getElementById('lueftOnlineBadge');
                if (badge) {
                    badge.textContent = d.online ? t('on') : t('off');
                    badge.style.background = d.online ? '#28a745' : '#6c757d';
                }
                for (let i = 0; i < 4; i++) {
                    const el = document.getElementById('lueftRpm' + i);
                    if (el) el.textContent = (d.online && d.rpm) ? d.rpm[i] : '--';
                }
            }).catch(() => {});
        }

        function loadLueftmodulConfig() {
            if (lueftmodulConfigLoaded) return;
            fetch('/api/lueftmodul').then(r => r.json()).then(d => {
                document.getElementById('lueftEnabled').checked        = d.enabled;
                document.getElementById('lueftGleicheWerte').checked   = d.gleiche_werte;
                document.getElementById('lueftMonitorEnabled').checked = d.monitor_enabled;
                document.getElementById('lueftMonitorTol').value       = d.monitor_tol_pct;
                if (d.percent && d.percent.length === 4) {
                    for (let i = 0; i < 4; i++) {
                        lueftPercent[i] = d.percent[i];
                        document.getElementById('lueftSlider' + i).value = d.percent[i];
                        document.getElementById('lueftSlider' + i + 'Value').textContent = d.percent[i] + '%';
                    }
                    document.getElementById('lueftSliderSingle').value = d.percent[0];
                    document.getElementById('lueftSliderSingleValue').textContent = d.percent[0] + '%';
                }
                updateLueftSliderVisibility();
                lueftmodulConfigLoaded = true;
            }).catch(() => {});
        }

        function saveLueftmodulForm() {
            const body = {
                enabled:         document.getElementById('lueftEnabled').checked,
                gleiche_werte:   document.getElementById('lueftGleicheWerte').checked,
                percent:         lueftPercent,
                monitor_enabled: document.getElementById('lueftMonitorEnabled').checked,
                monitor_tol_pct: parseInt(document.getElementById('lueftMonitorTol').value),
            };
            const st = document.getElementById('lueftSaveStatus');
            fetch('/api/lueftmodul/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // ===== Umwälzpumpe (Vorratsbehälter-Zirkulation) =====
        let umwaelzConfigLoaded = false;

        function loadUmwaelzConfig() {
            if (umwaelzConfigLoaded) return;
            fetch('/api/umwaelz').then(r => r.json()).then(d => {
                document.getElementById('umwEnabled').checked = d.aktiv;
                document.getElementById('umwLaufzeit').value  = d.laufzeit_s;
                document.getElementById('umwPause').value     = d.pausenzeit_min;
                umwaelzConfigLoaded = true;
            }).catch(() => {});
        }

        function saveUmwaelzForm() {
            const body = {
                aktiv:          document.getElementById('umwEnabled').checked,
                laufzeit_s:     parseInt(document.getElementById('umwLaufzeit').value),
                pausenzeit_min: parseInt(document.getElementById('umwPause').value),
            };
            const st = document.getElementById('umwSaveStatus');
            fetch('/api/umwaelz/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // ===== Drucksensor (Pumpendruck) An/Aus =====
        let druckSensorConfigLoaded = false;

        function loadDruckSensorConfigForm() {
            if (druckSensorConfigLoaded) return;
            fetch('/api/drucksensor').then(r => r.json()).then(d => {
                document.getElementById('druckSensorEnabled').checked = d.enabled;
                druckSensorConfigLoaded = true;
            }).catch(() => {});
        }

        function saveDruckSensorConfig() {
            const st = document.getElementById('druckSensorSaveStatus');
            fetch('/api/drucksensor/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({ enabled: document.getElementById('druckSensorEnabled').checked })
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(()=>st.textContent='',2000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // ===== DS18B20-Sensoren (Rollenzuordnung ueber ROM-Adresse) =====
        const DS18B20_ROLE_LABEL = { vorrat: () => t('dsVorrat'), pflanze: () => t('dsPflanze'), innen: () => t('dsInnen') };

        function scanDs18b20() {
            const container = document.getElementById('ds18b20List');
            if (container) container.innerHTML = '<div style="color:#aaa;font-size:0.85em;">' + t('loading') + '</div>';
            fetch('/api/ds18b20/scan').then(r => r.json()).then(d => {
                if (!container) return;
                const devices = d.devices || [];
                if (devices.length === 0) {
                    container.innerHTML = '<div style="color:#aaa;font-size:0.85em;">' + t('noSensorsFound') + '</div>';
                    return;
                }
                let html = '';
                devices.forEach(dev => {
                    const roleOpts = ['vorrat', 'pflanze', 'innen'].map(r =>
                        `<option value="${r}" ${dev.role === r ? 'selected' : ''}>${DS18B20_ROLE_LABEL[r]()}</option>`
                    ).join('');
                    html += `<div style="display:flex;justify-content:space-between;align-items:center;gap:8px;margin-top:8px;padding:8px;background:#f8f9fa;border-radius:6px;flex-wrap:wrap;">
                        <span style="font-family:monospace;font-size:0.8em;">${dev.addr}</span>
                        <span style="font-size:0.9em;min-width:60px;">${dev.temp !== undefined ? dev.temp + ' °C' : '--'}</span>
                        <select onchange="onDs18b20RoleChange('${dev.addr}', '${dev.role || ''}', this.value)" style="padding:4px;border:1px solid #ddd;border-radius:4px;">
                            <option value="">${t('noneOption')}</option>
                            ${roleOpts}
                        </select>
                    </div>`;
                });
                container.innerHTML = html;
            }).catch(() => {
                if (container) { container.style.color = '#dc3545'; container.textContent = t('connectionError'); }
            });
        }

        function onDs18b20RoleChange(addr, oldRole, newRole) {
            const calls = [];
            if (oldRole && oldRole !== newRole) {
                calls.push(fetch('/api/ds18b20/assign', {
                    method: 'POST', headers: {'Content-Type':'application/json'},
                    body: JSON.stringify({role: oldRole, clear: true})
                }));
            }
            if (newRole) {
                calls.push(fetch('/api/ds18b20/assign', {
                    method: 'POST', headers: {'Content-Type':'application/json'},
                    body: JSON.stringify({role: newRole, addr: addr})
                }));
            }
            Promise.all(calls).then(() => scanDs18b20()).catch(() => scanDs18b20());
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

        // ===== RS485-Adress-Diagnose (MARS-Hydro-Luefter) =====
        function scanRs485Addresses() {
            const from = parseInt(document.getElementById('rs485ScanFrom').value) || 1;
            const to   = parseInt(document.getElementById('rs485ScanTo').value)   || 20;
            const btn  = document.getElementById('rs485ScanBtn');
            const result = document.getElementById('rs485ScanResult');
            btn.disabled = true;
            btn.style.opacity = '0.5';
            result.innerHTML = '<span class="de">Scanne ' + from + '–' + to + ' …</span><span class="en">Scanning ' + from + '–' + to + ' …</span>';
            fetch('/api/rs485/scanfan', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({from: from, to: to})
            }).then(r => r.json()).then(d => {
                btn.disabled = false;
                btn.style.opacity = '1';
                if (!d.found || d.found.length === 0) {
                    result.innerHTML = '<span style="color:#dc3545;">⚠ <span class="de">Keine Antwort im gesamten Bereich.</span><span class="en">No response anywhere in the range.</span></span>';
                    return;
                }
                let html = '<div style="color:#28a745;margin-bottom:6px;">✓ <span class="de">Antwort gefunden bei:</span><span class="en">Response found at:</span></div>';
                for (const f of d.found) {
                    html += `<div style="font-family:monospace;padding:4px 0;">Adresse ${f.addr} (0x${f.addr.toString(16).padStart(2,'0').toUpperCase()}) — ${f.bytes} Byte: ${f.hex}</div>`;
                }
                result.innerHTML = html;
            }).catch(() => {
                btn.disabled = false;
                btn.style.opacity = '1';
                result.innerHTML = '<span style="color:#dc3545;">' + t('connectionError') + '</span>';
            });
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

        // Geraete-Auswahl: Licht-/Luefter-Ausgang (Lichtx4/MARS vs. Analogmodul)
        function updateSchedNumSsrVisibility(lightOutput) {
            const row = document.getElementById('schedNumSsrRow');
            if (row) row.style.display = (lightOutput == 0) ? 'grid' : 'none';
        }

        // Gemeinsame Auswertung von /api/output: Status-Badges, Analog-Statuszeile und den
        // fuer den Zeitplan-Chart massgeblichen outputLightMode aktuell halten. Fasst NICHT
        // die <select>-Formularfelder an, damit eine unbestaetigte Auswahl des Nutzers beim
        // naechsten 2s-Poll nicht ueberschrieben wird (analog zum schedLoaded-Muster).
        function applyOutputStatus(d) {
            const st = document.getElementById('analogStatus');
            if (st) {
                st.textContent = d.analog_online
                    ? 'Online — Ch1=' + d.analog_ch1_raw + '  Ch2=' + d.analog_ch2_raw
                    : t('offlineLabel');
            }

            // Geraetestatus-Leiste: bei 0-10V-Ausgang (RS485-Analog=1 oder GP8403=2) ersetzt
            // dessen Online-Status den des dann nicht mehr gepollten Lichtx4/MARS-Geraets.
            // Fan-Badge bei GP8403 setzt updateFanStatus() selbst (aus /api/fan online).
            const badgeLicht = document.getElementById('badge_licht');
            if (badgeLicht) {
                if (d.light_output == 1) badgeLicht.style.background = d.analog_online ? '#28a745' : '#dc3545';
                else if (d.light_output == 2) badgeLicht.style.background = d.gp8403_online ? '#28a745' : '#dc3545';
            }
            const badgeFan = document.getElementById('badge_fan');
            if (badgeFan && d.fan_output == 1) badgeFan.style.background = d.analog_online ? '#28a745' : '#dc3545';

            // Verbindungsart-Suffix: -R fuer RS485 (Lichtx4/MARS/Analogmodul), kein Suffix
            // fuer GP8403 (lokaler I2C-DAC, weder RS485 noch ESP-NOW).
            const lichtSuffix = document.getElementById('badge_licht_suffix');
            if (lichtSuffix) lichtSuffix.textContent = (d.light_output == 0 || d.light_output == 1) ? '-R' : '';
            const fanSuffix = document.getElementById('badge_fan_suffix');
            if (fanSuffix) fanSuffix.textContent = (d.fan_output == 0 || d.fan_output == 1) ? '-R' : '';

            const modeChanged = (outputLightMode != d.light_output);
            outputLightMode = d.light_output;
            outputFanMode   = d.fan_output;
            if (modeChanged && schedLoaded) { drawSchedChart(); updateSchedPhase(); }
        }

        // Periodischer Leichtgewicht-Poll (2s-Intervall): nur Status, keine Formularfelder.
        function updateOutputStatus() {
            fetch('/api/output').then(r => r.json()).then(applyOutputStatus).catch(() => {});
        }

        // Einmaliges Laden beim Tab-Wechsel/Initial-Load: fuellt zusaetzlich die Dropdowns.
        function loadOutputConfig() {
            fetch('/api/output').then(r => r.json()).then(d => {
                const lightEl = document.getElementById('outputLight');
                const fanEl   = document.getElementById('outputFan');
                if (lightEl) lightEl.value = d.light_output;
                if (fanEl)   fanEl.value   = d.fan_output;
                updateSchedNumSsrVisibility(d.light_output);
                applyOutputStatus(d);
            }).catch(() => {});
        }

        function saveOutputConfigForm() {
            const lightOutput = parseInt(document.getElementById('outputLight').value);
            const body = {
                light_output: lightOutput,
                fan_output:   parseInt(document.getElementById('outputFan').value),
            };
            const st = document.getElementById('outputSaveStatus');
            fetch('/api/output/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) { updateSchedNumSsrVisibility(lightOutput); setTimeout(()=>st.textContent='',2000); }
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        // System-Neustart
        function systemRestart() {
            if (!confirm(t('confirmRestart'))) return;
            const st = document.getElementById('systemRestartStatus');
            fetch('/api/system/restart', { method: 'POST' }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('savedRestarting') : t('error');
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

        // Benachrichtigungen: WhatsApp (CallMeBot) / Telegram / E-Mail (SMTP)
        function loadNotifyConfigForm() {
            fetch('/api/notify').then(r => r.json()).then(d => {
                document.getElementById('notifyGlobalEnabled').checked = d.global_enabled;
                document.getElementById('notifyNetzEnabled').checked      = d.netzausfall_enabled;
                document.getElementById('notifySensorEnabled').checked    = d.sensorausfall_enabled;
                document.getElementById('notifyFeuchteEnabled').checked   = d.feuchte_enabled;
                document.getElementById('notifyFeuchteMin').value         = d.feuchte_min;
                document.getElementById('notifyFeuchteMax').value         = d.feuchte_max;
                document.getElementById('notifyTemperaturEnabled').checked = d.temperatur_enabled;
                document.getElementById('notifyTemperaturMin').value      = d.temperatur_min;
                document.getElementById('notifyTemperaturMax').value      = d.temperatur_max;
                document.getElementById('notifyTankEnabled').checked      = d.tank_enabled;
                document.getElementById('notifyTankMin').value            = d.tank_min_prozent;
                document.getElementById('notifyLueftmodulEnabled').checked = d.lueftmodul_enabled;
                const hint = document.getElementById('notifyMs2Hint');
                if (hint) hint.textContent = d.ms2_online ? '' : t('notifyMs2Offline');

                // WhatsApp
                document.getElementById('notifyWhatsappEnabled').checked = d.whatsapp_enabled;
                document.getElementById('notifyPhone').value = d.phone || '';

                // Telegram
                document.getElementById('notifyTelegramEnabled').checked = d.telegram_enabled;
                document.getElementById('notifyTelegramChatId').value    = d.telegram_chat_id || '';
                document.getElementById('notifyTelegramToken').placeholder =
                    d.has_telegram_token ? 'unverändert lassen' : '123456789:AAExxx...';

                // E-Mail (SMTP)
                document.getElementById('notifyEmailEnabled').checked = d.email_enabled;
                document.getElementById('notifySmtpHost').value = d.smtp_host || '';
                document.getElementById('notifySmtpPort').value = d.smtp_port || 465;
                document.getElementById('notifySmtpUser').value = d.smtp_user || '';
                document.getElementById('notifySmtpTo').value   = d.email_to || '';
                document.getElementById('notifySmtpPass').placeholder =
                    d.has_smtp_pass ? 'unverändert lassen' : 'App-Passwort';
            }).catch(() => {});
        }

        function saveNotifyForm() {
            const body = {
                global_enabled:      document.getElementById('notifyGlobalEnabled').checked,
                netzausfall_enabled: document.getElementById('notifyNetzEnabled').checked,
                sensorausfall_enabled: document.getElementById('notifySensorEnabled').checked,
                feuchte_enabled:     document.getElementById('notifyFeuchteEnabled').checked,
                feuchte_min:         parseInt(document.getElementById('notifyFeuchteMin').value),
                feuchte_max:         parseInt(document.getElementById('notifyFeuchteMax').value),
                temperatur_enabled:  document.getElementById('notifyTemperaturEnabled').checked,
                temperatur_min:      parseInt(document.getElementById('notifyTemperaturMin').value),
                temperatur_max:      parseInt(document.getElementById('notifyTemperaturMax').value),
                tank_enabled:        document.getElementById('notifyTankEnabled').checked,
                tank_min_prozent:    parseInt(document.getElementById('notifyTankMin').value),
                lueftmodul_enabled:  document.getElementById('notifyLueftmodulEnabled').checked,

                whatsapp_enabled:    document.getElementById('notifyWhatsappEnabled').checked,
                phone:               document.getElementById('notifyPhone').value.trim(),
                apikey:              document.getElementById('notifyApikey').value.trim(),

                telegram_enabled:    document.getElementById('notifyTelegramEnabled').checked,
                telegram_chat_id:    document.getElementById('notifyTelegramChatId').value.trim(),
                telegram_token:      document.getElementById('notifyTelegramToken').value.trim(),

                email_enabled:       document.getElementById('notifyEmailEnabled').checked,
                smtp_host:           document.getElementById('notifySmtpHost').value.trim(),
                smtp_port:           parseInt(document.getElementById('notifySmtpPort').value) || 465,
                smtp_user:           document.getElementById('notifySmtpUser').value.trim(),
                smtp_pass:           document.getElementById('notifySmtpPass').value.trim(),
                email_to:            document.getElementById('notifySmtpTo').value.trim(),
            };
            const st = document.getElementById('notifySaveStatus');
            fetch('/api/notify/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                document.getElementById('notifyApikey').value = '';
                document.getElementById('notifyTelegramToken').value = '';
                document.getElementById('notifySmtpPass').value = '';
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) { setTimeout(()=>st.textContent='',2000); loadNotifyConfigForm(); }
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }

        function sendNotifyTestChannel(url) {
            const st = document.getElementById('notifySaveStatus');
            fetch(url, { method: 'POST' }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('notifyTestSent') : t('error');
                setTimeout(()=>st.textContent='',3000);
            }).catch(() => { st.style.color='#dc3545'; st.textContent=t('connectionError'); });
        }
        function sendNotifyTest()         { sendNotifyTestChannel('/api/notify/test'); }
        function sendNotifyTestTelegram() { sendNotifyTestChannel('/api/notify/test-telegram'); }
        function sendNotifyTestEmail()    { sendNotifyTestChannel('/api/notify/test-email'); }

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

        function deleteAllLogFiles() {
            if (!confirm(t('confirmDeleteAllLogs'))) return;
            const status = document.getElementById('deleteAllLogStatus');
            fetch('/api/logs/delete-all', { method: 'POST' })
                .then(r => r.json())
                .then(d => {
                    if (status) status.textContent = t('deleteAllDone', d.deleted);
                    loadLogFileList();
                })
                .catch(() => alert(t('connectionError')));
        }

        function startDebugCapture() {
            const btn = document.getElementById('debugCaptureBtn');
            const st  = document.getElementById('debugCaptureStatus');
            fetch('/api/debug/capture', { method: 'POST' }).then(r => r.json()).then(d => {
                if (!d.success) { st.style.color = '#dc3545'; st.textContent = d.error || t('error'); return; }
                st.style.color = '#888';
                btn.disabled = true;
                let remaining = 60;
                st.textContent = t('debugCaptureRunning', remaining);
                const iv = setInterval(() => {
                    remaining--;
                    if (remaining <= 0) {
                        clearInterval(iv);
                        btn.disabled = false;
                        st.style.color = '#27ae60';
                        st.textContent = t('debugCaptureDone');
                        loadLogFileList();
                        setTimeout(() => { st.textContent = ''; }, 4000);
                    } else {
                        st.textContent = t('debugCaptureRunning', remaining);
                    }
                }, 1000);
            }).catch(() => { st.style.color = '#dc3545'; st.textContent = t('connectionError'); });
        }

        // Manuelle Leistungs-Schieberegler auf der Startseite (nur sichtbar bei Modus=Manuell).
        // fanSliderDragging haelt den Server-Poll waehrend des Ziehens davon ab, den Wert
        // unter der Maus des Nutzers zurueckzusetzen (gleiches Muster wie schedDirtyFlag).
        let fanSliderDragging = false;

        function onFanManualSliderInput() {
            fanSliderDragging = true;
            const v = document.getElementById('fanManualSlider').value;
            document.getElementById('fanManualSliderValue').textContent = v + '%';
        }

        function onFanManualSliderChange() {
            const v = parseInt(document.getElementById('fanManualSlider').value);
            fetch('/api/fan/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({manual_percent: v})
            }).then(() => { fanSliderDragging = false; })
              .catch(() => { fanSliderDragging = false; });
        }

        // Startseite: Drehzahl/Modus, sichtbar sobald der aktive Luefter-Ausgang online ist
        function updateFanStatus() {
            fetch('/api/fan').then(r => r.json()).then(d => {
                const badgeFan = document.getElementById('badge_fan');
                // Bei Analog-Ausgang haelt updateOutputStatus() diese Badge aktuell (MARS-Luefter
                // wird dann gar nicht mehr gepollt und waere sonst faelschlich immer offline).
                if (badgeFan && outputFanMode != 1) badgeFan.style.background = d.online ? '#28a745' : '#dc3545';

                const section = document.getElementById('fanStatusSection');
                if (!section) return;
                section.style.display = d.online ? 'block' : 'none';
                refreshCo2FanCardVisibility();
                if (!d.online) return;
                const badge = document.getElementById('fanModeBadge');
                if (badge) {
                    if (d.enabled) { badge.textContent = fanModeName(d.mode) || '--'; badge.style.background = '#28a745'; }
                    else { badge.textContent = t('externalControl'); badge.style.background = '#6c757d'; }
                }
                const pctEl = document.getElementById('fanPercent');
                if (pctEl) pctEl.textContent = d.percent + '%';
                // RPM-Rueckmeldung gibt es nur vom MARS-Luefter (Tacho), das Analogmodul hat
                // keinen Ruecklesewert dafuer.
                const rpmBox = document.getElementById('fanRpmBox');
                if (rpmBox) rpmBox.style.display = (d.rpm !== undefined) ? '' : 'none';
                const rpmEl = document.getElementById('fanRpm');
                if (rpmEl && d.rpm !== undefined) rpmEl.textContent = d.rpm;

                const sliderRow = document.getElementById('fanManualSliderRow');
                if (sliderRow) {
                    const showSlider = d.enabled && d.mode === 0;
                    sliderRow.style.display = showSlider ? 'block' : 'none';
                    if (showSlider && !fanSliderDragging) {
                        const sl = document.getElementById('fanManualSlider');
                        if (sl) sl.value = d.manual_percent;
                        const valEl = document.getElementById('fanManualSliderValue');
                        if (valEl) valEl.textContent = d.manual_percent + '%';
                    }
                }
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

                // Status (Startseite) — eine Karte, jeden Poll neu befuellt, nur aktive Behälter.
                // Die Box selbst ist nur sichtbar, wenn mindestens ein Behälter aktiv ist.
                const statusCard = document.getElementById('behaelterStatusCard');
                const anyActive  = d.behaelter.some(b => b.aktiv);
                if (statusCard) statusCard.style.display = anyActive ? 'block' : 'none';
                const pumpeBadge = document.getElementById('pumpeBadge');
                if (pumpeBadge) {
                    pumpeBadge.textContent = d.pumpe_aktiv ? t('on') : t('off');
                    pumpeBadge.style.background = d.pumpe_aktiv ? '#28a745' : '#6c757d';
                }
                const umwaelzBadge = document.getElementById('umwaelzBadge');
                if (umwaelzBadge) {
                    umwaelzBadge.textContent = d.umwaelzpumpe_aktiv ? t('on') : t('off');
                    umwaelzBadge.style.background = d.umwaelzpumpe_aktiv ? '#28a745' : '#6c757d';
                }
                const lichtBadge = document.getElementById('lichtStatusBadge');
                if (lichtBadge) {
                    lichtBadge.textContent = (d.licht_an ? '☀ ' : '🌙 ') + (d.licht_an ? t('phaseLicht') : t('phaseDunkel'));
                    lichtBadge.style.background = d.licht_an ? '#f39c12' : '#34495e';
                }

                const statusContainer = document.getElementById('behaelterStatusCards');
                if (statusContainer && anyActive) {
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
                    statusContainer.innerHTML = statusHtml;
                }

                if (!ventilKonfigGeladen) {
                    // Einmalig: Konfig-Karten mit Eingabefeldern aufbauen (Konfiguration-Seite)
                    document.getElementById('vorlaufAktiv').checked  = d.vorlauf_aktiv;
                    document.getElementById('vorlaufzeit').value     = d.vorlaufzeit_s;
                    document.getElementById('gleicheZeiten').checked = d.gleiche_zeiten;
                    let html = '';
                    for (let i = 0; i < 3; i++) {
                        const b = d.behaelter[i];
                        html += `<div class="status-box-info">
                            <strong>${namen[i]}</strong>
                            <div style="margin-top:10px;display:flex;justify-content:space-between;align-items:center;">
                                <label><input type="checkbox" id="aktiv${i}" ${b.aktiv ? 'checked' : ''}> ${t('active')}</label>
                                <button id="doseBtn${i}" onclick="triggerManualDose(${i})" style="background:#17a2b8;color:white;border:none;padding:6px 12px;border-radius:4px;cursor:pointer;font-size:0.82em;">💧 ${t('doseOnce')}</button>
                            </div>
                            <div class="behaelter-zeiten-individual" style="margin-top:8px;">
                                <div style="font-size:0.78em;color:#888;margin-bottom:3px;">☀ <span class="de">Lichtphase</span><span class="en">Light phase</span></div>
                                <div style="display:grid; grid-template-columns:1fr 1fr; gap:8px;margin-bottom:8px;">
                                    <label style="font-size:0.9em;">${t('openingLabel')}<br>
                                        <input type="number" id="oeffnung${i}" min="1" max="60" value="${b.oeffnungszeit_s}" style="width:100%;padding:4px;">
                                    </label>
                                    <label style="font-size:0.9em;">${t('pauseLabel')}<br>
                                        <input type="number" id="pause${i}" min="1" max="60" value="${b.pausenzeit_min}" style="width:100%;padding:4px;">
                                    </label>
                                </div>
                                <div style="font-size:0.78em;color:#888;margin-bottom:3px;">🌙 <span class="de">Dunkelphase</span><span class="en">Dark phase</span></div>
                                <div style="display:grid; grid-template-columns:1fr 1fr; gap:8px;">
                                    <label style="font-size:0.9em;">${t('openingLabel')}<br>
                                        <input type="number" id="oeffnung${i}Dunkel" min="1" max="60" value="${b.oeffnungszeit_s_dunkel}" style="width:100%;padding:4px;">
                                    </label>
                                    <label style="font-size:0.9em;">${t('pauseLabel')}<br>
                                        <input type="number" id="pause${i}Dunkel" min="1" max="60" value="${b.pausenzeit_min_dunkel}" style="width:100%;padding:4px;">
                                    </label>
                                </div>
                            </div>
                        </div>`;
                    }
                    document.getElementById('behaelterCards').innerHTML = html;
                    if (d.gleiche_zeiten) {
                        document.getElementById('oeffnungGemeinsam').value       = d.behaelter[0].oeffnungszeit_s;
                        document.getElementById('pauseGemeinsam').value          = d.behaelter[0].pausenzeit_min;
                        document.getElementById('oeffnungGemeinsamDunkel').value = d.behaelter[0].oeffnungszeit_s_dunkel;
                        document.getElementById('pauseGemeinsamDunkel').value    = d.behaelter[0].pausenzeit_min_dunkel;
                    }
                    updateGleicheZeitenVisibility();
                    ventilKonfigGeladen = true;
                }
            }).catch(() => {});
        }

        // ===== Manueller Einzelzyklus pro Behaelter (Button auf jeder Behaelter-Karte) =====
        function triggerManualDose(index) {
            fetch('/api/ventile/dose/start', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({index: index})
            }).then(r => r.json()).then(d => {
                if (!d.success) alert(t('error'));
                updateManualDoseStatus();
            }).catch(() => alert(t('connectionError')));
        }

        function updateManualDoseStatus() {
            fetch('/api/ventile/dose/status').then(r => r.json()).then(d => {
                for (let i = 0; i < 3; i++) {
                    const btn = document.getElementById('doseBtn' + i);
                    if (!btn) continue;
                    const runningHere = d.active && d.index === i;
                    btn.disabled = d.active;
                    btn.style.opacity = d.active ? '0.5' : '1';
                    btn.style.pointerEvents = d.active ? 'none' : 'auto';
                    btn.textContent = runningHere ? '💧 ' + t('doseRunning', d.remaining_s) : '💧 ' + t('doseOnce');
                }
            }).catch(() => {});
        }

        // "Gleiche Zeiten fuer alle": Checkbox schaltet zwischen 3 individuellen Zeitfeldern
        // (pro Behaelter-Karte) und einem gemeinsamen Feldpaar um. Die einzelnen "Aktiv"-
        // Haken bleiben in beiden Modi nutzbar, nur die Zeiten werden im gemeinsamen Modus
        // synchron gehalten (auch serverseitig, siehe handleApiVentilConfig()).
        function updateGleicheZeitenVisibility() {
            const checked = document.getElementById('gleicheZeiten').checked;
            const gf = document.getElementById('gleicheZeitenFields');
            if (gf) gf.style.display = checked ? 'block' : 'none';
            document.querySelectorAll('.behaelter-zeiten-individual').forEach(el => {
                el.style.display = checked ? 'none' : 'block';
            });
        }

        function applyGleicheZeiten() {
            const o  = document.getElementById('oeffnungGemeinsam').value;
            const p  = document.getElementById('pauseGemeinsam').value;
            const od = document.getElementById('oeffnungGemeinsamDunkel').value;
            const pd = document.getElementById('pauseGemeinsamDunkel').value;
            for (let i = 0; i < 3; i++) {
                const oEl  = document.getElementById('oeffnung' + i);
                const pEl  = document.getElementById('pause' + i);
                const odEl = document.getElementById('oeffnung' + i + 'Dunkel');
                const pdEl = document.getElementById('pause' + i + 'Dunkel');
                if (oEl)  oEl.value  = o;
                if (pEl)  pEl.value  = p;
                if (odEl) odEl.value = od;
                if (pdEl) pdEl.value = pd;
            }
            saveVentilConfig();
        }

        function onGleicheZeitenChange() {
            updateGleicheZeitenVisibility();
            if (document.getElementById('gleicheZeiten').checked) {
                // Gemeinsame Felder mit den aktuellen Werten von Behälter 1 vorbelegen
                const o  = document.getElementById('oeffnung0')?.value;
                const p  = document.getElementById('pause0')?.value;
                const od = document.getElementById('oeffnung0Dunkel')?.value;
                const pd = document.getElementById('pause0Dunkel')?.value;
                if (o)  document.getElementById('oeffnungGemeinsam').value       = o;
                if (p)  document.getElementById('pauseGemeinsam').value          = p;
                if (od) document.getElementById('oeffnungGemeinsamDunkel').value = od;
                if (pd) document.getElementById('pauseGemeinsamDunkel').value    = pd;
                applyGleicheZeiten();
            } else {
                saveVentilConfig();
            }
        }

        // Fertige Licht-/Dunkelphase-Profile aus dem LPA-Betriebsprotokoll (Punkt 2/3) zum
        // Vorbefuellen aller Zeitfelder — egal ob "Gleiche Zeiten" an oder aus ist, damit der
        // Nutzer danach nur noch den gewuenschten Modus waehlen und speichern muss.
        const SPRAY_SCHEMES = {
            phase1: { oeffnung: 5, pause: 3,  oeffnungDunkel: 4, pauseDunkel: 10 },  // Junge Pflanzen
            phase2: { oeffnung: 4, pause: 5,  oeffnungDunkel: 3, pauseDunkel: 15 },  // Große Pflanzen
        };

        function applySprayScheme() {
            const key = document.getElementById('sprayScheme').value;
            const s = SPRAY_SCHEMES[key];
            if (!s) return;
            document.getElementById('oeffnungGemeinsam').value       = s.oeffnung;
            document.getElementById('pauseGemeinsam').value          = s.pause;
            document.getElementById('oeffnungGemeinsamDunkel').value = s.oeffnungDunkel;
            document.getElementById('pauseGemeinsamDunkel').value    = s.pauseDunkel;
            for (let i = 0; i < 3; i++) {
                const oEl  = document.getElementById('oeffnung' + i);
                const pEl  = document.getElementById('pause' + i);
                const odEl = document.getElementById('oeffnung' + i + 'Dunkel');
                const pdEl = document.getElementById('pause' + i + 'Dunkel');
                if (oEl)  oEl.value  = s.oeffnung;
                if (pEl)  pEl.value  = s.pause;
                if (odEl) odEl.value = s.oeffnungDunkel;
                if (pdEl) pdEl.value = s.pauseDunkel;
            }
        }

        function saveVentilConfig() {
            const body = {
                vorlauf_aktiv:  document.getElementById('vorlaufAktiv').checked,
                vorlaufzeit_s:  parseInt(document.getElementById('vorlaufzeit').value),
                gleiche_zeiten: document.getElementById('gleicheZeiten').checked,
                behaelter: [0,1,2].map(i => ({
                    aktiv:                  document.getElementById('aktiv' + i)?.checked ?? false,
                    oeffnungszeit_s:        parseInt(document.getElementById('oeffnung' + i)?.value ?? 5),
                    pausenzeit_min:         parseInt(document.getElementById('pause' + i)?.value ?? 5),
                    oeffnungszeit_s_dunkel: parseInt(document.getElementById('oeffnung' + i + 'Dunkel')?.value ?? 5),
                    pausenzeit_min_dunkel:  parseInt(document.getElementById('pause' + i + 'Dunkel')?.value ?? 5)
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

        // ===== Ausgangstest (Ventile 1-4, Umwälzpumpe, Pumpe — direkte GPIO) =====
        let pcfTestActive  = false;
        let pcfTestPending = false;
        let pcfState = 0;
        const OUTPUT_TEST_COUNT = 6;

        async function togglePcfTest() {
            const wantActive = !(pcfTestActive || pcfTestPending);
            const r = await fetch('/api/output/test', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({testmode: wantActive})
            });
            const d = await r.json();
            pcfTestActive  = !!d.testmode;
            pcfTestPending = !!d.pending;
            pcfState = d.state || 0;
            updatePcfUi();
        }

        // Periodisches Polling (unabhaengig von Klicks) -- damit die UI mitbekommt, wenn ein
        // angeforderter Testmodus von selbst aktiv wird, sobald der laufende Zyklus endet.
        function updatePcfStatus() {
            fetch('/api/output/test').then(r => r.json()).then(d => {
                pcfTestActive  = !!d.testmode;
                pcfTestPending = !!d.pending;
                pcfState = d.state || 0;
                updatePcfUi();
            }).catch(() => {});
        }

        async function pcfToggle(index) {
            const on = !((pcfState >> index) & 1);
            const r = await fetch('/api/output/test', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({index: index, on: on})
            });
            const d = await r.json();
            pcfState = d.state;
            updatePcfUi();
        }

        function updatePcfUi() {
            const toggleBtn = document.getElementById('pcfTestToggle');
            const warning   = document.getElementById('pcfTestWarning');
            const pendingInfo = document.getElementById('pcfTestPendingInfo');
            const buttons   = document.getElementById('pcfTestButtons');
            if (!toggleBtn) return;
            if (pcfTestActive) {
                toggleBtn.textContent = t('stopTest');
                toggleBtn.style.background = '#28a745';
                warning.style.display = 'block';
                if (pendingInfo) pendingInfo.style.display = 'none';
                buttons.style.opacity = '1';
                buttons.style.pointerEvents = 'auto';
            } else if (pcfTestPending) {
                toggleBtn.textContent = t('stopTest');  // Anfrage laesst sich wieder zuruecknehmen
                toggleBtn.style.background = '#e0a800';
                warning.style.display = 'none';
                if (pendingInfo) pendingInfo.style.display = 'block';
                buttons.style.opacity = '0.4';
                buttons.style.pointerEvents = 'none';
            } else {
                toggleBtn.textContent = t('startTest');
                toggleBtn.style.background = '#e74c3c';
                warning.style.display = 'none';
                if (pendingInfo) pendingInfo.style.display = 'none';
                buttons.style.opacity = '0.4';
                buttons.style.pointerEvents = 'none';
            }
            for (let i = 0; i < OUTPUT_TEST_COUNT; i++) {
                const b = document.getElementById('pcf_btn_' + i);
                if (!b) continue;
                const on = (pcfState >> i) & 1;
                b.style.background = on ? '#28a745' : '#6c757d';
                b.querySelector('span').textContent = on ? t('on') : t('off');
            }
        }

        // ===== Durchflussmesser-Kalibrierung =====
        // state: 0=idle 1=Pumpe an (vor Start) 2=zaehlt (Ventil offen) 3=gestoppt (Menge eingeben)
        function updateFlowCalStatus() {
            fetch('/api/flowcal/status').then(r => r.json()).then(d => {
                // Zustaende: 0=idle 1=Pumpe an 2=Entlueften 3=zaehlt 4=gestoppt
                document.getElementById('flowcalIdle').style.display        = d.state === 0 ? 'block' : 'none';
                document.getElementById('flowcalRunning').style.display     = (d.state === 1 || d.state === 2 || d.state === 3) ? 'block' : 'none';

                const purgeBtn = document.getElementById('flowcalPurgeBtn');
                purgeBtn.style.display = (d.state === 1 || d.state === 2) ? 'inline-block' : 'none';
                if (d.state === 2) {
                    purgeBtn.style.background = '#dc3545';
                    purgeBtn.innerHTML = '<span class="de">Stop</span><span class="en">Stop</span>';
                } else {
                    purgeBtn.style.background = '#f39c12';
                    purgeBtn.innerHTML = '<span class="de">Entlüften (Ventil öffnen)</span><span class="en">Purge air (open valve)</span>';
                }

                const beginBtn = document.getElementById('flowcalBeginBtn');
                beginBtn.style.display      = d.state === 1 ? 'inline-block' : 'none';
                beginBtn.disabled           = !d.purged;
                beginBtn.style.opacity      = d.purged ? '1' : '0.4';
                beginBtn.style.pointerEvents = d.purged ? 'auto' : 'none';

                document.getElementById('flowcalStopBtn').style.display     = d.state === 3 ? 'inline-block' : 'none';
                document.getElementById('flowcalVolumeInput').style.display = d.state === 4 ? 'block' : 'none';

                document.getElementById('flowcalDruck').textContent  = d.druckBar;
                document.getElementById('flowcalPulses').textContent = d.pulses_live;
                if (d.state === 4) document.getElementById('flowcalPulsesResult').textContent = d.pulses_result;

                const cur = document.getElementById('flowcalCurrent');
                if (d.pulses_per_liter > 0 && d.state === 0) {
                    cur.style.display = 'block';
                    document.getElementById('flowcalCurrentValue').textContent = d.pulses_per_liter;
                } else {
                    cur.style.display = 'none';
                }
            }).catch(() => {});
        }

        function flowcalStart() {
            fetch('/api/flowcal/start', { method: 'POST' }).then(updateFlowCalStatus);
        }
        function flowcalPurge() {
            fetch('/api/flowcal/purge', { method: 'POST' }).then(updateFlowCalStatus);
        }
        function flowcalBegin() {
            fetch('/api/flowcal/begin', { method: 'POST' }).then(updateFlowCalStatus);
        }
        function flowcalStop() {
            fetch('/api/flowcal/stop', { method: 'POST' }).then(updateFlowCalStatus);
        }
        function flowcalCalc() {
            const ml = parseFloat(document.getElementById('flowcalMl').value);
            if (!ml || ml <= 0) { alert(t('flowcalInvalidVolume')); return; }
            fetch('/api/flowcal/calc', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({volume_ml: ml})
            }).then(r => r.json()).then(d => {
                document.getElementById('flowcalMl').value = '';
                const resultDiv = document.getElementById('flowcalResult');
                if (d.success) {
                    resultDiv.style.display = 'block';
                    document.getElementById('flowcalResultValue').textContent = d.pulses_per_liter;
                    setTimeout(() => resultDiv.style.display = 'none', 8000);
                }
                updateFlowCalStatus();
            });
        }
        function flowcalCancel() {
            fetch('/api/flowcal/cancel', { method: 'POST' }).then(updateFlowCalStatus);
        }

        // ===== Duesen-Ueberwachung =====
        let flowmonConfigLoaded = false;

        function updateFlowMonStatus() {
            fetch('/api/flowmon').then(r => r.json()).then(d => {
                document.getElementById('flowmonLast1').textContent  = d.last_cycle_ml[0];
                document.getElementById('flowmonLast2').textContent  = d.last_cycle_ml[1];
                document.getElementById('flowmonLast3').textContent  = d.last_cycle_ml[2];
                document.getElementById('flowmonTotal1').textContent = d.total_ml[0];
                document.getElementById('flowmonTotal2').textContent = d.total_ml[1];
                document.getElementById('flowmonTotal3').textContent = d.total_ml[2];
                document.getElementById('flowmonTotalAll').textContent = d.total_ml_all;

                // Config nur einmal uebernehmen (nicht bei jedem Poll ueberschreiben, sonst
                // tippt man gegen staendig zurueckgesetzte Eingabefelder)
                if (!flowmonConfigLoaded) {
                    document.getElementById('flowmonEnabled').checked = d.enabled;
                    document.getElementById('flowmonSoll1').value     = d.erwarteter_durchfluss_ml_s[0];
                    document.getElementById('flowmonSoll2').value     = d.erwarteter_durchfluss_ml_s[1];
                    document.getElementById('flowmonSoll3').value     = d.erwarteter_durchfluss_ml_s[2];
                    document.getElementById('flowmonToleranz').value  = d.toleranz_prozent;
                    flowmonConfigLoaded = true;
                }
            }).catch(() => {});
        }

        function saveFlowMonConfig() {
            const body = {
                enabled: document.getElementById('flowmonEnabled').checked,
                toleranz_prozent: parseInt(document.getElementById('flowmonToleranz').value) || 30,
                erwarteter_durchfluss_ml_s: [
                    parseFloat(document.getElementById('flowmonSoll1').value) || 0,
                    parseFloat(document.getElementById('flowmonSoll2').value) || 0,
                    parseFloat(document.getElementById('flowmonSoll3').value) || 0,
                ]
            };
            const st = document.getElementById('flowmonSaveStatus');
            fetch('/api/flowmon/save', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(d => {
                st.style.color = d.success ? '#27ae60' : '#dc3545';
                st.textContent = d.success ? t('saved') : t('error');
                if (d.success) setTimeout(() => st.textContent = '', 2000);
            }).catch(() => { st.style.color = '#dc3545'; st.textContent = t('connectionError'); });
        }

        // Setzt alle drei Sollwerte auf 0 (= Vergleich fuer den jeweiligen Behaelter deaktiviert)
        // und speichert direkt -- praktisch nach dem Duesenreinigen, um spaeter mit frisch
        // ermittelten Werten neu zu kalibrieren statt die alten (jetzt wieder gueltigen) Werte
        // faelschlich als Abweichung zu werten.
        function resetFlowMonSollwerte() {
            if (!confirm(t('flowmonResetConfirm'))) return;
            document.getElementById('flowmonSoll1').value = 0;
            document.getElementById('flowmonSoll2').value = 0;
            document.getElementById('flowmonSoll3').value = 0;
            saveFlowMonConfig();
        }

        // ===== RS485 Sensoren =====

        function updateRs485() {
            fetch('/api/rs485').then(r => r.json()).then(d => {
                const ms2 = d.ms2;
                if (!ms2) return;
                // Gemeinsamer "Zeltwerte"-Block (siehe renderZeltCard) statt eigenem RS485-Block,
                // kein PPFD hier (das laeuft separat unten weiter).
                zeltRs485.online = !!ms2.online;
                zeltRs485.seen   = true;
                zeltRs485.temp   = ms2.online ? ms2.temp : '--';
                zeltRs485.hum    = ms2.online ? ms2.hum  : '--';
                zeltRs485.co2    = ms2.online ? ms2.co2  : '--';
                zeltRs485.co2Ok  = ms2.online ? (ms2.co2_sensor_ok !== false) : true;
                renderZeltCard();

                // PPFD/Spektrum vom RS485-Multisensor (MS2) — bei diesem Aufbau die
                // tatsaechliche Quelle des Lichtsensors (nicht ESP-NOW).
                lichtHasMs2Data = !!(ms2.online && (ms2.ppfd !== undefined || (ms2.channels && ms2.channels.length === 8)));
                if (lichtHasMs2Data) {
                    const ppfdEl = document.getElementById('lichtPpfd');
                    if (ppfdEl && ms2.ppfd !== undefined) ppfdEl.textContent = parseFloat(ms2.ppfd).toFixed(1);
                    if (ms2.channels && ms2.channels.length === 8)
                        requestAnimationFrame(() => drawSpektrumChart('spektrumChart', ms2.channels));
                    const ageEl = document.getElementById('lichtAge');
                    if (ageEl && ms2.age_s !== undefined) ageEl.textContent = 'vor ' + ms2.age_s + ' s';
                    const missBox = document.getElementById('lichtMissingBox');
                    if (missBox) missBox.style.display = (ms2.light_sensor_ok === false) ? '' : 'none';
                }
                updateLichtVisibility();

                // Geraetestatus-Leiste (Fussbereich)
                const badgeMs2 = document.getElementById('badge_ms2');
                if (badgeMs2) badgeMs2.style.background = ms2.online ? '#28a745' : '#dc3545';
                const badgeLicht = document.getElementById('badge_licht');
                // Nur im Lichtx4-Modus (0) setzt updateLicht() die Badge; bei 0-10V-Ausgang
                // (Analog=1 / GP8403=2) macht das updateOutputStatus(), da Lichtx4 dann nicht
                // mehr gepollt wird und sonst faelschlich immer offline waere.
                if (badgeLicht && d.licht && outputLightMode == 0) badgeLicht.style.background = d.licht.online ? '#28a745' : '#dc3545';
            }).catch(() => {});
        }

        setInterval(() => {
            if (!updateInProgress) {
                updateData();
                updateVentile();
                updateNodes();
                updateRs485();
                updateCo2Status();
                updateHumidStatus();
                updateFanStatus();
                updateFlowCalStatus();
                updateFlowMonStatus();
                updateOutputStatus();
                updateSchedManualStatus();
                updateDwcStatus();
                updateRaumlichtStatus();
                updateGehaeuseFanStatus();
                updateLueftmodulStatus();
                updateManualDoseStatus();
                updatePcfStatus();
                schedTick();
            }
        }, 2000);
        applyLangButtons();
        applyOptionTranslations();
        updateData();
        updateVentile();
        updateNodes();
        updateRs485();
        updateCo2Status();
        updateHumidStatus();
        updateFanStatus();
        updateFlowCalStatus();
        updateFlowMonStatus();
        updateManualDoseStatus();
        updatePcfStatus();
        loadZeitplan();
        loadOutputConfig();
        updateSchedManualStatus();
        updateDwcStatus();
        updateRaumlichtStatus();
        updateGehaeuseFanStatus();
        updateLueftmodulStatus();
    </script>
)rawliteral"
