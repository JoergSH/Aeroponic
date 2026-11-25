R"rawliteral(
        <!-- ============================================================ -->
        <!-- AUSWERTUNG — Diagramme aus den SD-Tageslogs (rein im Browser)  -->
        <!-- ============================================================ -->
        <!-- Liest die CSV-Tagesdateien ueber /api/logs + /api/logs/download und zeichnet alles
             selbst auf <canvas> (keine externe Bibliothek, funktioniert auch ohne Internet).
             Spalten werden ueber den Header-Namen gesucht, aeltere Dateien mit anderem Format
             (z.B. *_alt.csv, nur 3 Behaelter) funktionieren daher ebenso. -->
        <div id="chartsTab" class="tab-content">
            <style>
                .ch-toolbar{display:flex;flex-wrap:wrap;gap:10px 16px;align-items:center;font-size:0.9em}
                .ch-toolbar select{padding:5px;border:1px solid #ddd;border-radius:4px;margin-left:4px}
                .ch-btn{background:#17a2b8;color:#fff;border:none;padding:6px 12px;border-radius:4px;cursor:pointer;font-size:0.9em}
                .ch-btn:hover{opacity:.85}
                .ch-status{color:#888;font-size:0.85em}
                .ch-toggles{display:flex;flex-wrap:wrap;gap:8px 12px;margin:12px 0 4px;font-size:0.85em}
                .ch-tgroup{display:flex;flex-wrap:wrap;gap:4px 10px;align-items:center;background:#f8f9fa;border-radius:6px;padding:5px 9px}
                .ch-tg-title{font-weight:bold;color:#555;margin-right:2px}
                .ch-tg{white-space:nowrap;cursor:pointer}
                .ch-tg input{vertical-align:middle;margin:0 3px 0 0}
                .ch-sw{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:4px;vertical-align:middle}
                .ch-hoverbar{display:flex;justify-content:space-between;flex-wrap:wrap;gap:6px;font-size:0.82em;color:#666;margin-top:8px;min-height:1.2em}
                .ch-panel{border:1px solid #e3e6ea;border-radius:8px;padding:6px 6px 2px;margin-top:8px;background:#fff}
                .ch-head{display:flex;flex-wrap:wrap;gap:3px 14px;align-items:baseline;font-size:0.82em;padding:0 4px 4px}
                .ch-head .ch-title{font-weight:bold;font-size:1.05em;color:#2c3e50;margin-right:6px}
                .ch-li{white-space:nowrap;color:#555}
                .ch-cv{width:100%;display:block;touch-action:pan-y;cursor:crosshair}
                .ch-scroll{display:flex;gap:8px;align-items:center;margin-top:10px}
                .ch-scroll input[type=range]{flex:1;min-width:0}
                .ch-hint{font-size:0.78em;color:#999;margin-top:6px}
                .ch-cards{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:10px;margin-top:12px}
                .ch-card{background:#f8f9fa;border-radius:8px;border-left:4px solid #27ae60;padding:10px 12px;font-size:0.85em}
                .ch-c-title{color:#555;font-weight:bold;margin-bottom:4px}
                .ch-c-main{font-size:1.5em;font-weight:bold;color:#2c3e50}
                .ch-c-main small{font-size:0.55em;color:#888;font-weight:normal}
                .ch-c-sub{color:#777;margin-top:3px}
                .ch-bar{height:6px;background:#e9ecef;border-radius:3px;margin:6px 0 4px;overflow:hidden}
                .ch-bar span{display:block;height:100%}
                table.ch-days{width:100%;border-collapse:collapse;font-size:0.82em;margin-top:6px}
                .ch-days th{background:#f1f3f5;color:#555;padding:6px;text-align:right;white-space:nowrap}
                .ch-days td{padding:5px 6px;text-align:right;border-bottom:1px solid #eee;white-space:nowrap}
                .ch-days th:first-child,.ch-days td:first-child{text-align:left}
                .ch-days tr[data-day]{cursor:pointer}
                .ch-days tr[data-day]:hover td{background:#eef7f1}
            </style>

            <h3 class="section-heading">📈 <span class="de">Klima-Verlauf</span><span class="en">Climate history</span></h3>
            <div class="status-box-info ch-toolbar">
                <label><span class="de">Zeitraum</span><span class="en">Period</span>
                    <select id="chRange">
                        <option class="i18n-opt" value="1" data-de="1 Tag" data-en="1 day">1 Tag</option>
                        <option class="i18n-opt" value="7" data-de="1 Woche" data-en="1 week">1 Woche</option>
                    </select></label>
                <label><span class="de">bis</span><span class="en">until</span> <select id="chEndDay"></select></label>
                <label><span class="de">Ausschnitt</span><span class="en">Window</span> <select id="chWindow"></select></label>
                <button id="chReloadBtn" class="ch-btn">🔄 <span class="de">Neu laden</span><span class="en">Reload</span></button>
                <span id="chStatus" class="ch-status"></span>
            </div>
            <div id="chToggles" class="ch-toggles"></div>
            <div class="ch-hoverbar"><span id="chViewLabel"></span><span id="chHoverTime"></span></div>
            <div id="chPanels"></div>
            <div class="ch-scroll">
                <button class="ch-btn" id="chPrev" title="zurück / back">◀</button>
                <input type="range" id="chScroll" min="0" max="1000" value="1000">
                <button class="ch-btn" id="chNext" title="vor / forward">▶</button>
                <button class="ch-btn" id="chAll">⤢ <span class="de">Alles</span><span class="en">All</span></button>
            </div>
            <div class="ch-hint">
                <span class="de">Mausrad = zoomen · Ziehen = verschieben · Doppelklick = alles · Handy: wischen bzw. mit zwei Fingern zoomen. Grau hinterlegt = Licht aus, rot gestrichelt ↻ = Neustart, orange ⚡ = Netzaussetzer, graues ▲ = Zelt-Sensor nicht erreichbar (kurze Lücken werden in der Linie überbrückt), grünes Band = VPD-Zielbereich (mit aktiver VPD-Regelung deren Sollbereich, sonst 0,8–1,2 kPa).</span>
                <span class="en">Mouse wheel = zoom · drag = pan · double-click = all · phone: swipe or pinch. Grey background = lights off, red dashed ↻ = restart, orange ⚡ = mains dropout, grey ▲ = tent sensor unreachable (short gaps are bridged in the line), green band = VPD target range (the controller's target when VPD control is active, otherwise 0.8–1.2 kPa).</span>
            </div>
            <div id="chStats" class="ch-cards"></div>

            <h3 class="section-heading">📅 <span class="de">Tagesübersicht</span><span class="en">Daily summary</span></h3>
            <div style="overflow-x:auto;"><table id="chDays" class="ch-days"></table></div>
            <div class="ch-hint"><span class="de">DLI = Tageslichtsumme aus PPFD. Zeile anklicken springt zu diesem Tag.</span><span class="en">DLI = daily light integral from PPFD. Click a row to jump to that day.</span></div>

            <h3 class="section-heading">🚿 <span class="de">Düsen-Auswertung</span><span class="en">Nozzle analysis</span></h3>
            <div class="status-box-info ch-toolbar">
                <span id="chNozToggles" style="display:flex;flex-wrap:wrap;gap:4px 14px;"></span>
                <label><span class="de">Anzeige</span><span class="en">Show</span>
                    <select id="chNozMode">
                        <option class="i18n-opt" value="dur" data-de="Laufzeit je Zyklus (s)" data-en="Run time per cycle (s)">Laufzeit je Zyklus (s)</option>
                        <option class="i18n-opt" value="rate" data-de="Impulse pro Sekunde" data-en="Pulses per second">Impulse pro Sekunde</option>
                        <option class="i18n-opt" value="cyc" data-de="Impulse je Zyklus" data-en="Pulses per cycle">Impulse je Zyklus</option>
                    </select></label>
            </div>
            <div id="chNozzleWrap"></div>
            <div id="chNozCards" class="ch-cards"></div>
            <div class="ch-hint">
                <span class="de">Punkte = einzelne Gießzyklen, Linie = gleitender Median über 9 Zyklen, roter Ring = Ausreißer (&gt;15 % vom Median). „Laufzeit je Zyklus“ passt zur Dosierung nach Menge: Das Ventil schließt nach der eingestellten Impulszahl, eine Düse, die zusetzt, braucht dafür länger (Trend steigt). „Impulse pro Sekunde“ bleibt vergleichbar, auch wenn Öffnungszeit oder Menge geändert wurden. Die Bewertung bezieht sich auf den Ausschnitt oben: Trend = letztes gegen erstes Drittel der Zyklen, Streuung = Standardabweichung in %.</span>
                <span class="en">Dots = single watering cycles, line = rolling median over 9 cycles, red ring = outlier (&gt;15 % from the median). "Run time per cycle" fits dosing by amount: the valve closes after the set pulse count, a clogging nozzle needs longer (trend rising). "Pulses per second" stays comparable even if the open time or amount was changed. The rating refers to the window above: trend = last vs. first third of cycles, spread = standard deviation in %.</span>
            </div>

            <script>
            (function () {
                'use strict';
                const GAP = 16 * 60000;           // Luecke > 16 min (Log alle 5 min) -> Linie unterbrechen
                const PL = 50, PR = 50, PT = 8, PB = 22;
                const HOUR = 3600000, DAY = 86400000;
                const VPD_BAND = [0.8, 1.2];
                const NOZ_COLORS = ['#e74c3c', '#2e86de', '#27ae60', '#8e44ad'];
                const OUTLIER = 0.15;
                const PREF_KEY = 'aeroponik-charts-v1';

                function L(de, en) { return (typeof currentLang !== 'undefined' && currentLang === 'en') ? en : de; }

                const SERIES = [
                    { id: 'zeltT',  col: 'ZeltTemp_C',        de: 'Zelt',           en: 'Tent',              panel: 'temp',  color: '#e74c3c', on: true,  unit: '°C',  dec: 1 },
                    { id: 'raumT',  col: 'RaumTemp_C',        de: 'Raum (draußen)', en: 'Room (outside)',    panel: 'temp',  color: '#3498db', on: true,  unit: '°C',  dec: 1 },
                    { id: 'vorT',   col: 'TempVorrat_C',      de: 'Nährlösung',     en: 'Nutrient solution', panel: 'temp',  color: '#16a085', on: false, unit: '°C',  dec: 1 },
                    { id: 'pflT',   col: 'TempPflanze_C',     de: 'Wurzelraum',     en: 'Root zone',         panel: 'temp',  color: '#8e44ad', on: false, unit: '°C',  dec: 1 },
                    { id: 'zeltH',  col: 'ZeltFeuchte_%',     de: 'Zelt',           en: 'Tent',              panel: 'hum',   color: '#e67e22', on: true,  unit: '%',   dec: 0 },
                    { id: 'raumH',  col: 'RaumFeuchte_%',     de: 'Raum (draußen)', en: 'Room (outside)',    panel: 'hum',   color: '#5dade2', on: true,  unit: '%',   dec: 0 },
                    { id: 'vpd',    col: 'ZeltVPD_kPa',       de: 'VPD Luft',       en: 'Air VPD',           panel: 'hum',   color: '#27ae60', on: true,  unit: 'kPa', dec: 2, right: true },
                    { id: 'vpdLeaf', col: 'VPD_Blatt_kPa',    de: 'VPD Blatt',      en: 'Leaf VPD',          panel: 'hum',   color: '#145a32', on: true,  unit: 'kPa', dec: 2, right: true },
                    { id: 'ppfd',   col: 'ZeltPPFD_umol',     de: 'PPFD',           en: 'PPFD',              panel: 'light', color: '#f39c12', on: true,  unit: 'µmol/m²s', dec: 0, fill: true },
                    { id: 'co2',    col: 'ZeltCO2_ppm',       de: 'CO₂',            en: 'CO₂',               panel: 'co2',   color: '#7f8c8d', on: false, unit: 'ppm', dec: 0 },
                    { id: 'fuell',  col: 'Fuellstand_%',      de: 'Füllstand',      en: 'Tank level',        panel: 'tank',  color: '#2980b9', on: false, unit: '%',   dec: 0 },
                    { id: 'fan',    col: 'LuefterLeistung_%', de: 'Lüfter',         en: 'Fan',               panel: 'act',   color: '#95a5a6', on: false, unit: '%',   dec: 0 },
                    { id: 'humOut', col: 'HumidAusgang',      de: 'Befeuchter',     en: 'Humidifier',        panel: 'act',   color: '#3498db', on: false, state: true },
                    { id: 'co2Out', col: 'CO2Ausgang',        de: 'CO₂-Ventil',     en: 'CO₂ valve',         panel: 'act',   color: '#7f8c8d', on: false, state: true },
                    { id: 'wlan',   col: 'WLAN_dBm',          de: 'WLAN',           en: 'WiFi',              panel: 'diag',  color: '#2c3e50', on: false, unit: 'dBm', dec: 0 },
                    { id: 'ms2to',  col: 'MS2_Timeouts',      de: 'Zelt-Sensor Timeouts', en: 'Tent sensor timeouts', panel: 'diag', color: '#c0392b', on: false, unit: '', dec: 0, right: true, min0: true }
                ];
                const PANELS = [
                    { id: 'temp',  de: '🌡️ Temperatur',          en: '🌡️ Temperature',      h: 180, minSpan: 2 },
                    { id: 'hum',   de: '💧 Luftfeuchte & VPD',   en: '💧 Humidity & VPD',   h: 180, minSpan: 10, band: VPD_BAND },
                    { id: 'light', de: '☀️ Licht',               en: '☀️ Light',            h: 120, min0: true },
                    { id: 'co2',   de: '🫧 CO₂',                 en: '🫧 CO₂',              h: 120, minSpan: 50 },
                    { id: 'tank',  de: '🛢️ Füllstand',           en: '🛢️ Tank level',       h: 110, min0: true },
                    { id: 'act',   de: '⚙️ Aktoren',             en: '⚙️ Actuators',        h: 120, min0: true },
                    { id: 'diag',  de: '📡 Verbindung',          en: '📡 Connection',       h: 120, minSpan: 10 },
                    { id: 'noz',   de: '🚿 Durchfluss je Behälter', en: '🚿 Flow per reservoir', h: 220 }
                ];

                const S = {
                    inited: false, files: [], cache: {}, t: [], cols: {}, marks: [], nights: [], noz: [],
                    d0: 0, d1: 0, v0: 0, v1: 0, hoverT: null, range: 1, on: {},
                    nozOn: [true, true, true, true], nozMode: 'dur',
                    loading: false, statsKey: '', drag: null, pinch: null, raf: false
                };

                // ---------- Hilfsfunktionen ----------
                const $ = id => document.getElementById(id);
                const pad2 = n => (n < 10 ? '0' : '') + n;
                function fmt(v, dec) { return (v === undefined || v === null || !isFinite(v)) ? '–' : v.toFixed(dec); }
                function fmtTime(t) { const d = new Date(t); return pad2(d.getHours()) + ':' + pad2(d.getMinutes()); }
                function fmtDate(t) { const d = new Date(t); return pad2(d.getDate()) + '.' + pad2(d.getMonth() + 1) + '.'; }
                function fmtDT(t) { return fmtDate(t) + ' ' + fmtTime(t); }
                function dayStart(t) { const d = new Date(t); d.setHours(0, 0, 0, 0); return d.getTime(); }
                function addDays(t, n) { const d = new Date(t); d.setDate(d.getDate() + n); return d.getTime(); }  // sommerzeitsicher
                function parseDayName(s) { const m = /^(\d{4})-(\d{2})-(\d{2})/.exec(s); return m ? new Date(+m[1], +m[2] - 1, +m[3]).getTime() : NaN; }
                function median(a) { if (!a.length) return NaN; const b = a.slice().sort((x, y) => x - y); const m = b.length >> 1; return b.length % 2 ? b[m] : (b[m - 1] + b[m]) / 2; }
                function lowerBound(arr, x) { let lo = 0, hi = arr.length; while (lo < hi) { const m = (lo + hi) >> 1; if (arr[m] < x) lo = m + 1; else hi = m; } return lo; }
                function lowerBoundT(a, x) { let lo = 0, hi = a.length; while (lo < hi) { const m = (lo + hi) >> 1; if (a[m].t < x) lo = m + 1; else hi = m; } return lo; }
                function signed(v, dec) { return isFinite(v) ? (v > 0 ? '+' : '') + v.toFixed(dec) : '–'; }

                function loadPrefs() {
                    SERIES.forEach(s => { S.on[s.id] = s.on; });
                    try {
                        const p = JSON.parse(localStorage.getItem(PREF_KEY) || '{}');
                        if (p.on) Object.keys(p.on).forEach(k => { if (k in S.on) S.on[k] = !!p.on[k]; });
                        if (Array.isArray(p.nozOn)) for (let k = 0; k < 4; k++) if (typeof p.nozOn[k] === 'boolean') S.nozOn[k] = p.nozOn[k];
                        // nozV 2: "Laufzeit je Zyklus" ist neu und Standard -- aeltere gespeicherte Auswahl nicht uebernehmen
                        if (p.nozV === 2 && (p.nozMode === 'cyc' || p.nozMode === 'rate' || p.nozMode === 'dur')) S.nozMode = p.nozMode;
                        if (p.range === 7) S.range = 7;
                    } catch (e) {}
                }
                function savePrefs() {
                    try { localStorage.setItem(PREF_KEY, JSON.stringify({ on: S.on, nozOn: S.nozOn, nozMode: S.nozMode, nozV: 2, range: S.range })); } catch (e) {}
                }

                // ---------- DOM ----------
                function buildDom() {
                    $('chPanels').innerHTML = '';
                    $('chNozzleWrap').innerHTML = '';
                    PANELS.forEach(p => {
                        const host = p.id === 'noz' ? $('chNozzleWrap') : $('chPanels');
                        const div = document.createElement('div');
                        div.className = 'ch-panel';
                        div.innerHTML = '<div class="ch-head"><span class="ch-title"></span><span class="ch-leg" style="display:contents;"></span></div><canvas class="ch-cv"></canvas>';
                        host.appendChild(div);
                        p.div = div;
                        p.cv = div.querySelector('canvas');
                        p.cv.style.height = p.h + 'px';
                        p.title = div.querySelector('.ch-title');
                        p.leg = div.querySelector('.ch-leg');
                        attachEvents(p.cv);
                    });
                }

                function buildToggles() {
                    let h = '';
                    PANELS.forEach(p => {
                        if (p.id === 'noz') return;
                        h += '<div class="ch-tgroup"><span class="ch-tg-title">' + L(p.de, p.en) + '</span>';
                        SERIES.filter(s => s.panel === p.id).forEach(s => {
                            h += '<label class="ch-tg"><input type="checkbox" data-sid="' + s.id + '"' + (S.on[s.id] ? ' checked' : '') +
                                 '><span class="ch-sw" style="background:' + s.color + '"></span>' + L(s.de, s.en) + '</label>';
                        });
                        h += '</div>';
                    });
                    $('chToggles').innerHTML = h;
                    $('chToggles').querySelectorAll('input[data-sid]').forEach(cb => cb.addEventListener('change', () => {
                        S.on[cb.dataset.sid] = cb.checked; savePrefs(); S.statsKey = ''; redraw();
                    }));
                    let n = '';
                    for (let k = 0; k < 4; k++) {
                        n += '<label class="ch-tg"><input type="checkbox" data-noz="' + k + '"' + (S.nozOn[k] ? ' checked' : '') +
                             '><span class="ch-sw" style="background:' + NOZ_COLORS[k] + '"></span>' + L('Behälter ', 'Reservoir ') + (k + 1) + '</label>';
                    }
                    $('chNozToggles').innerHTML = n;
                    $('chNozToggles').querySelectorAll('input[data-noz]').forEach(cb => cb.addEventListener('change', () => {
                        S.nozOn[+cb.dataset.noz] = cb.checked; savePrefs(); S.statsKey = ''; redraw();
                    }));
                    PANELS.forEach(p => { if (p.title) p.title.textContent = L(p.de, p.en); });
                }

                function buildWindowSelect() {
                    const hs = S.range === 1 ? [1, 3, 6, 12, 24] : [6, 12, 24, 48, 72, 168];
                    $('chWindow').innerHTML = '<option value="">' + L('frei', 'custom') + '</option>' + hs.map(h =>
                        '<option value="' + h + '">' + (h < 24 ? h + ' h' : (h / 24) + ' ' + (h === 24 ? L('Tag', 'day') : L('Tage', 'days'))) + '</option>').join('');
                    syncWindowSelect();
                }
                function syncWindowSelect() {
                    const sel = $('chWindow');
                    const h = Math.round((S.v1 - S.v0) / HOUR * 100) / 100;
                    let found = '';
                    for (const o of sel.options) if (o.value && Math.abs(+o.value - h) < 0.02) found = o.value;
                    sel.value = found;
                }

                function wireControls() {
                    $('chRange').value = String(S.range);
                    $('chRange').addEventListener('change', e => { S.range = +e.target.value; savePrefs(); loadData(false); });
                    $('chEndDay').addEventListener('change', () => loadData(false));
                    $('chWindow').addEventListener('change', e => {
                        const h = +e.target.value; if (!h) return;
                        S.v0 = S.v1 - h * HOUR; clampView(); redraw();
                    });
                    $('chReloadBtn').addEventListener('click', () => { loadFileList().then(() => loadData(true)); });
                    $('chScroll').addEventListener('input', e => {
                        const full = S.d1 - S.d0, w = S.v1 - S.v0;
                        S.v0 = S.d0 + (full - w) * (+e.target.value / 1000); S.v1 = S.v0 + w; redraw();
                    });
                    const shift = f => { const w = S.v1 - S.v0; S.v0 += w * f; S.v1 += w * f; clampView(); redraw(); };
                    $('chPrev').addEventListener('click', () => shift(-0.5));
                    $('chNext').addEventListener('click', () => shift(0.5));
                    $('chAll').addEventListener('click', () => { S.v0 = S.d0; S.v1 = S.d1; redraw(); });
                    $('chNozMode').value = S.nozMode;
                    $('chNozMode').addEventListener('change', e => { S.nozMode = e.target.value; savePrefs(); S.statsKey = ''; redraw(); });
                    window.addEventListener('resize', () => redraw());
                    window.addEventListener('mouseup', () => { S.drag = null; });
                }

                // ---------- Interaktion ----------
                function tAtX(cv, x) { return S.v0 + (x - PL) / (cv.clientWidth - PL - PR) * (S.v1 - S.v0); }
                function clampView() {
                    const full = S.d1 - S.d0;
                    let w = S.v1 - S.v0;
                    if (!(w > 0)) w = full;
                    w = Math.min(Math.max(w, 15 * 60000), full);
                    if (S.v0 < S.d0) S.v0 = S.d0;
                    S.v1 = S.v0 + w;
                    if (S.v1 > S.d1) { S.v1 = S.d1; S.v0 = S.v1 - w; }
                }
                function zoomAt(tc, f) {
                    const w = S.v1 - S.v0;
                    const nw = Math.min(Math.max(w * f, 15 * 60000), S.d1 - S.d0);
                    S.v0 = tc - (tc - S.v0) * nw / w; S.v1 = S.v0 + nw;
                    clampView(); redraw();
                }
                function attachEvents(cv) {
                    cv.addEventListener('mousemove', e => {
                        const r = cv.getBoundingClientRect();
                        if (S.drag) {
                            const dt = (S.drag.x - e.clientX) / (cv.clientWidth - PL - PR) * (S.drag.v1 - S.drag.v0);
                            S.v0 = S.drag.v0 + dt; S.v1 = S.drag.v1 + dt; clampView();
                        }
                        S.hoverT = tAtX(cv, e.clientX - r.left); redraw();
                    });
                    cv.addEventListener('mouseleave', () => { S.hoverT = null; redraw(); });
                    cv.addEventListener('mousedown', e => { S.drag = { x: e.clientX, v0: S.v0, v1: S.v1 }; e.preventDefault(); });
                    cv.addEventListener('wheel', e => {
                        e.preventDefault();
                        const r = cv.getBoundingClientRect();
                        zoomAt(tAtX(cv, e.clientX - r.left), e.deltaY > 0 ? 1.25 : 0.8);
                    }, { passive: false });
                    cv.addEventListener('dblclick', () => { S.v0 = S.d0; S.v1 = S.d1; redraw(); });
                    cv.addEventListener('touchstart', e => {
                        const r = cv.getBoundingClientRect();
                        if (e.touches.length === 1) {
                            S.drag = { x: e.touches[0].clientX, v0: S.v0, v1: S.v1 }; S.pinch = null;
                            S.hoverT = tAtX(cv, e.touches[0].clientX - r.left); redraw();
                        } else if (e.touches.length === 2) {
                            const a = e.touches[0].clientX, b = e.touches[1].clientX;
                            S.pinch = { d: Math.abs(a - b) || 1, t: tAtX(cv, (a + b) / 2 - r.left), v0: S.v0, v1: S.v1 }; S.drag = null;
                        }
                    }, { passive: true });
                    cv.addEventListener('touchmove', e => {
                        if (S.drag && e.touches.length === 1) {
                            const dx = S.drag.x - e.touches[0].clientX;
                            const dt = dx / (cv.clientWidth - PL - PR) * (S.drag.v1 - S.drag.v0);
                            S.v0 = S.drag.v0 + dt; S.v1 = S.drag.v1 + dt; clampView();
                            const r = cv.getBoundingClientRect();
                            S.hoverT = tAtX(cv, e.touches[0].clientX - r.left); redraw();
                        } else if (S.pinch && e.touches.length === 2) {
                            e.preventDefault();
                            const p = S.pinch;
                            const f = p.d / (Math.abs(e.touches[0].clientX - e.touches[1].clientX) || 1);
                            const nw = Math.min(Math.max((p.v1 - p.v0) * f, 15 * 60000), S.d1 - S.d0);
                            S.v0 = p.t - (p.t - p.v0) * nw / (p.v1 - p.v0); S.v1 = S.v0 + nw; clampView(); redraw();
                        }
                    }, { passive: false });
                    cv.addEventListener('touchend', () => { S.drag = null; S.pinch = null; });
                }

                // ---------- Laden & Aufbereiten ----------
                function loadFileList() {
                    return fetch('/api/logs').then(r => r.json()).then(d => {
                        S.files = (d.files || []).map(f => String(f.name).replace(/^\//, ''))
                                                 .filter(n => /^\d{4}-\d{2}-\d{2}(_alt\d*)?\.csv$/.test(n));
                        const days = [...new Set(S.files.map(n => n.slice(0, 10)))].sort().reverse();
                        const sel = $('chEndDay'), prev = sel.value;
                        sel.innerHTML = days.map(d => '<option value="' + d + '">' + d.slice(8, 10) + '.' + d.slice(5, 7) + '.' + d.slice(0, 4) + '</option>').join('');
                        if (prev && days.includes(prev)) sel.value = prev;
                    });
                }

                async function loadData(keepView) {
                    if (S.loading) return;
                    S.loading = true;
                    const status = $('chStatus');
                    try {
                        const endDay = $('chEndDay').value;
                        if (!endDay) { status.textContent = L('Keine Log-Dateien auf der SD-Karte.', 'No log files on the SD card.'); return; }
                        const endT = parseDayName(endDay), startT = addDays(endT, -(S.range - 1));
                        const want = S.files.filter(n => { const t = parseDayName(n); return t >= startT && t <= endT; })
                            .sort((a, b) => a.localeCompare(b));   // build() sortiert die Zeilen ohnehin nach Zeit
                        const newest = S.files.map(n => n.slice(0, 10)).sort().pop();
                        let i = 0;
                        for (const n of want) {
                            i++;
                            const live = n.slice(0, 10) === newest && !n.includes('_alt');   // laufender Tag waechst noch
                            if (S.cache[n] === undefined || live) {
                                status.textContent = L('Lade ', 'Loading ') + n + ' (' + i + '/' + want.length + ') …';
                                const r = await fetch('/api/logs/download?file=' + encodeURIComponent(n));
                                // Fehlgeschlagenen Download nicht merken, sonst bleibt der Tag bis zum
                                // Neuladen der Seite leer (z.B. Abruf waehrend Master-Neustart/Update)
                                if (r.ok) S.cache[n] = await r.text(); else delete S.cache[n];
                            }
                        }
                        build(want, startT, addDays(endT, 1));
                        if (!keepView) { S.v0 = S.d0; S.v1 = S.d1; }
                        clampView();
                        buildWindowSelect();
                        renderDays();
                        status.textContent = want.length
                            ? want.length + L(' Datei(en), ', ' file(s), ') + S.t.length + L(' Messpunkte', ' data points')
                            : L('Keine Daten im Zeitraum.', 'No data in this period.');
                        S.statsKey = '';
                        redraw();
                    } catch (e) {
                        status.textContent = L('Fehler beim Laden: ', 'Load error: ') + e;
                    } finally {
                        S.loading = false;
                    }
                }

                function build(names, d0, d1) {
                    const rows = [];
                    names.forEach(n => {
                        const txt = S.cache[n]; if (!txt) return;
                        const bd = new Date(parseDayName(n));
                        const lines = txt.split(/\r?\n/);
                        const hdr = lines[0].split(',');
                        const zi = hdr.indexOf('Zeit'); if (zi < 0) return;
                        for (let li = 1; li < lines.length; li++) {
                            if (!lines[li]) continue;
                            const c = lines[li].split(',');
                            const p = (c[zi] || '').split(':'); if (p.length < 2) continue;
                            const t = new Date(bd.getFullYear(), bd.getMonth(), bd.getDate(), +p[0], +p[1], +(p[2] || 0)).getTime();
                            if (isFinite(t)) rows.push({ t, hdr, c });
                        }
                    });
                    rows.sort((a, b) => a.t - b.t);
                    const N = rows.length;
                    const need = SERIES.map(s => s.col).concat(['LichtMaske', 'Laufzeit_min', 'Netzaussetzer', 'NetzaussetzerMax_ms', 'MS2_Abfragen', 'VPD_Soll_min', 'VPD_Soll_max']);
                    for (let k = 1; k <= 4; k++) need.push('ImpulseBehaelter' + k, 'DauerBehaelter' + k + '_s');
                    S.t = new Array(N); S.cols = {};
                    need.forEach(col => { S.cols[col] = new Array(N).fill(NaN); });
                    const idx = new Map();   // Spaltenindex je Header (= je Datei)
                    rows.forEach((r, i) => {
                        S.t[i] = r.t;
                        let im = idx.get(r.hdr);
                        if (!im) { im = need.map(col => r.hdr.indexOf(col)); idx.set(r.hdr, im); }
                        for (let j = 0; j < need.length; j++) {
                            const ci = im[j]; if (ci < 0) continue;
                            const s = r.c[ci]; if (s === undefined || s === '') continue;
                            const v = parseFloat(s); if (isFinite(v)) S.cols[need[j]][i] = v;
                        }
                    });

                    // Typischer Zeilenabstand (Median): 5 min bis FW 1.20.1, danach 1 min -- wird fuer
                    // die Dauer der letzten Zeile, Aktor-Balken und Aussetzer-Dauern gebraucht.
                    const diffs = [];
                    for (let i = 1; i < N && diffs.length < 400; i++) { const d = S.t[i] - S.t[i - 1]; if (d > 0 && d <= GAP) diffs.push(d); }
                    S.rowDt = diffs.length ? median(diffs) : 5 * 60000;

                    // Ereignisse: Neustart (Laufzeit springt zurueck), Netzaussetzer
                    S.marks = [];
                    const lz = S.cols['Laufzeit_min'], na = S.cols['Netzaussetzer'], nm = S.cols['NetzaussetzerMax_ms'];
                    let prevL = NaN;
                    for (let i = 0; i < N; i++) {
                        const l = lz[i];
                        if (isFinite(l)) {
                            if ((isFinite(prevL) && l < prevL) || (i === 0 && l <= 10)) S.marks.push({ t: S.t[i] - l * 60000, type: 'restart' });
                            prevL = l;
                        }
                        if (na[i] > 0) S.marks.push({ t: S.t[i], type: 'mains', n: na[i], ms: nm[i] });
                    }
                    // Zelt-Sensor-Aussetzer: Zeile vorhanden, aber Zelt-Temperatur UND -Feuchte leer
                    // (Master hat den RS485-Sensor nicht erreicht). Aufeinanderfolgende Zeilen = ein Aussetzer.
                    const zt = S.cols['ZeltTemp_C'], zh = S.cols['ZeltFeuchte_%'];
                    let run = null;
                    for (let i = 0; i < N; i++) {
                        const miss = !isFinite(zt[i]) && !isFinite(zh[i]);
                        if (miss) {
                            if (run && S.t[i] - S.t[i - 1] <= GAP) run.n++;
                            else { run = { t: S.t[i], type: 'sensor', n: 1 }; S.marks.push(run); }
                        } else run = null;
                    }

                    // Licht aus (LichtMaske == 0) -> grau hinterlegte Zeitraeume
                    S.nights = [];
                    const lm = S.cols['LichtMaske'];
                    let ns = null;
                    for (let i = 0; i < N; i++) {
                        const v = lm[i];
                        if (v === 0) {
                            if (ns === null) ns = S.t[i];
                            else if (S.t[i] - S.t[i - 1] > GAP) { S.nights.push([ns, S.t[i - 1]]); ns = S.t[i]; }
                        } else if (isFinite(v) && ns !== null) {
                            // Datenluecke nicht als "Licht aus" grau fuellen
                            S.nights.push([ns, S.t[i] - S.t[i - 1] > GAP ? S.t[i - 1] : S.t[i]]); ns = null;
                        }
                    }
                    if (ns !== null && N) S.nights.push([ns, S.t[N - 1]]);

                    // Duesen: je Behaelter die einzelnen Giesszyklen + gleitender Median (9 Zyklen, zentriert)
                    S.noz = [];
                    for (let k = 1; k <= 4; k++) {
                        const P = S.cols['ImpulseBehaelter' + k], D = S.cols['DauerBehaelter' + k + '_s'];
                        const a = [];
                        for (let i = 0; i < N; i++) {
                            if (isFinite(P[i]) && isFinite(D[i]) && D[i] >= 0.5) a.push({ t: S.t[i], p: P[i], d: D[i], r: P[i] / D[i] });
                        }
                        for (let i = 0; i < a.length; i++) {
                            const w = a.slice(Math.max(0, i - 4), Math.min(a.length, i + 5));
                            a[i].mr = median(w.map(x => x.r));
                            a[i].mp = median(w.map(x => x.p));
                            a[i].md = median(w.map(x => x.d));
                        }
                        S.noz.push(a);
                    }
                    S.d0 = d0; S.d1 = d1;
                }

                // ---------- Zeichnen ----------
                function redraw() {
                    if (S.raf) return;
                    S.raf = true;
                    requestAnimationFrame(() => { S.raf = false; drawAll(); });
                }

                function drawAll() {
                    const tab = $('chartsTab');
                    if (!tab || !tab.classList.contains('active') || !S.d1) return;
                    PANELS.forEach(p => {
                        const vis = p.id === 'noz' ? S.nozOn.some(x => x) : SERIES.some(s => s.panel === p.id && S.on[s.id]);
                        p.div.style.display = vis ? '' : 'none';
                        if (vis) { if (p.id === 'noz') drawNozzle(p); else drawPanel(p); }
                    });
                    const full = S.d1 - S.d0, w = S.v1 - S.v0, sc = $('chScroll');
                    sc.disabled = w >= full - 1000;
                    sc.value = full - w > 0 ? Math.round((S.v0 - S.d0) / (full - w) * 1000) : 1000;
                    $('chViewLabel').textContent = fmtDT(S.v0) + ' – ' + fmtDT(S.v1);
                    $('chHoverTime').textContent = S.hoverT !== null ? '🕐 ' + fmtDT(S.hoverT) : '';
                    const key = [S.v0, S.v1, JSON.stringify(S.on), S.nozOn.join(), S.nozMode, L('de', 'en')].join('|');
                    if (key !== S.statsKey) { S.statsKey = key; syncWindowSelect(); renderStats(); renderNozCards(); }
                }

                function prepCanvas(p) {
                    const cv = p.cv, w = cv.clientWidth, h = p.h;
                    if (!w) return null;
                    const dpr = window.devicePixelRatio || 1;
                    const W = Math.round(w * dpr), H = Math.round(h * dpr);
                    if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
                    const ctx = cv.getContext('2d');
                    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
                    ctx.clearRect(0, 0, w, h);
                    return { ctx, w, h, pw: w - PL - PR };
                }

                const TSTEPS = [10, 15, 30, 60, 120, 180, 360, 720, 1440, 2880].map(m => m * 60000);
                function timeTicks(pw) {
                    const span = S.v1 - S.v0, want = Math.max(2, Math.floor(pw / 80));
                    let step = TSTEPS[TSTEPS.length - 1];
                    for (const s of TSTEPS) if (span / s <= want) { step = s; break; }
                    const out = [];
                    if (step >= DAY) {
                        let t = dayStart(S.v0); if (t < S.v0) t = addDays(t, 1);
                        for (; t <= S.v1; t = addDays(t, step / DAY)) out.push({ t, label: fmtDate(t), major: true });
                        return out;
                    }
                    const off = new Date(S.v0).getTimezoneOffset() * 60000;   // auf lokale Uhrzeit ausrichten
                    for (let t = Math.ceil((S.v0 - off) / step) * step + off; t <= S.v1; t += step) {
                        const d = new Date(t), mid = d.getHours() === 0 && d.getMinutes() === 0;
                        out.push({ t, label: mid ? fmtDate(t) : fmtTime(t), major: mid });
                    }
                    return out;
                }

                function niceTicks(lo, hi, n) {
                    if (!(hi > lo)) hi = lo + 1;
                    const raw = (hi - lo) / n, p = Math.pow(10, Math.floor(Math.log10(raw))), f = raw / p;
                    const step = (f < 1.5 ? 1 : f < 3 ? 2 : f < 7 ? 5 : 10) * p;
                    const a = Math.floor(lo / step) * step, b = Math.ceil(hi / step) * step;
                    const ticks = [];
                    for (let v = a; v <= b + step * 1e-6; v += step) ticks.push(+v.toFixed(6));
                    return { lo: a, hi: b, step, ticks };
                }
                const tickDec = step => step < 0.1 ? 2 : step < 1 ? 1 : 0;

                function drawFrame(g, X) {
                    const { ctx, w, h } = g;
                    ctx.fillStyle = '#eef1f6';
                    S.nights.forEach(([a, b]) => {
                        if (b < S.v0 || a > S.v1) return;
                        const x0 = Math.max(PL, X(a)), x1 = Math.min(w - PR, X(b));
                        if (x1 > x0) ctx.fillRect(x0, PT, x1 - x0, h - PT - PB);
                    });
                    ctx.font = '10px Arial'; ctx.textAlign = 'center'; ctx.textBaseline = 'top'; ctx.lineWidth = 1;
                    timeTicks(g.pw).forEach(o => {
                        const x = Math.round(X(o.t)) + 0.5;
                        ctx.strokeStyle = o.major ? '#c5ccd4' : '#ececec';
                        ctx.beginPath(); ctx.moveTo(x, PT); ctx.lineTo(x, h - PB); ctx.stroke();
                        ctx.fillStyle = o.major ? '#555' : '#888';
                        ctx.fillText(o.label, x, h - PB + 5);
                    });
                    ctx.strokeStyle = '#d0d4d9';
                    ctx.strokeRect(PL + 0.5, PT + 0.5, g.pw - 1, h - PT - PB - 1);
                }

                function drawMarks(g, X) {
                    const { ctx, h } = g;
                    ctx.font = '11px Arial'; ctx.textAlign = 'center'; ctx.textBaseline = 'top';
                    S.marks.forEach(m => {
                        if (m.t < S.v0 || m.t > S.v1) return;
                        if (m.type === 'sensor') {   // kleines graues Dreieck auf der Grundlinie
                            const x = X(m.t), y = h - PB;
                            ctx.fillStyle = '#7f8c8d';
                            ctx.beginPath(); ctx.moveTo(x, y - 7); ctx.lineTo(x - 4, y); ctx.lineTo(x + 4, y); ctx.closePath(); ctx.fill();
                            return;
                        }
                        const x = Math.round(X(m.t)) + 0.5, col = m.type === 'restart' ? '#c0392b' : '#e67e22';
                        ctx.save();
                        ctx.strokeStyle = col; ctx.setLineDash([4, 3]);
                        ctx.beginPath(); ctx.moveTo(x, PT); ctx.lineTo(x, h - PB); ctx.stroke();
                        ctx.restore();
                        ctx.fillStyle = col;
                        ctx.fillText(m.type === 'restart' ? '↻' : '⚡', x, PT + 1);
                    });
                }

                function drawHover(g, X) {
                    if (S.hoverT === null || S.hoverT < S.v0 || S.hoverT > S.v1) return;
                    const x = Math.round(X(S.hoverT)) + 0.5, ctx = g.ctx;
                    ctx.strokeStyle = 'rgba(0,0,0,0.45)'; ctx.lineWidth = 1;
                    ctx.beginPath(); ctx.moveTo(x, PT); ctx.lineTo(x, g.h - PB); ctx.stroke();
                }

                function nearestIdx(t) {
                    const N = S.t.length; if (!N) return -1;
                    let i = lowerBound(S.t, t); if (i >= N) i = N - 1;
                    if (i > 0 && Math.abs(S.t[i - 1] - t) < Math.abs(S.t[i] - t)) i--;
                    return Math.abs(S.t[i] - t) <= 10 * 60000 ? i : -1;
                }

                function drawSeries(ctx, X, Y, arr, i0, i1, s, base) {
                    const segs = []; let cur = null, lastT = 0;
                    for (let i = i0; i <= i1; i++) {
                        const v = arr[i];
                        // Einzelne fehlende Werte ueberbruecken; ob wirklich unterbrochen wird, entscheidet
                        // nur der Zeitabstand (GAP). Sensor-Aussetzer bleiben ueber die grauen ▲-Marken sichtbar.
                        if (!isFinite(v)) continue;
                        const t = S.t[i];
                        if (cur && t - lastT > GAP) cur = null;
                        if (!cur) { cur = []; segs.push(cur); }
                        cur.push(X(t), Y(v)); lastT = t;
                    }
                    ctx.strokeStyle = s.color; ctx.fillStyle = s.color; ctx.lineWidth = 1.6; ctx.lineJoin = 'round';
                    segs.forEach(sg => {
                        if (sg.length === 2) { ctx.beginPath(); ctx.arc(sg[0], sg[1], 1.6, 0, 7); ctx.fill(); return; }
                        ctx.beginPath(); ctx.moveTo(sg[0], sg[1]);
                        for (let j = 2; j < sg.length; j += 2) ctx.lineTo(sg[j], sg[j + 1]);
                        ctx.stroke();
                        if (s.fill) {
                            ctx.lineTo(sg[sg.length - 2], base); ctx.lineTo(sg[0], base); ctx.closePath();
                            ctx.globalAlpha = 0.18; ctx.fill(); ctx.globalAlpha = 1;
                        }
                    });
                }

                function drawPanel(p) {
                    const g = prepCanvas(p); if (!g) return;
                    const { ctx, w, h, pw } = g;
                    const X = t => PL + (t - S.v0) / (S.v1 - S.v0) * pw;
                    drawFrame(g, X);
                    const N = S.t.length;
                    const i0 = Math.max(0, lowerBound(S.t, S.v0) - 1), i1 = Math.min(N - 1, lowerBound(S.t, S.v1));
                    const act = SERIES.filter(s => s.panel === p.id && S.on[s.id]);
                    const lines = act.filter(s => !s.state), states = act.filter(s => s.state);
                    const bottom = h - PB - states.length * 9;
                    let any = false;

                    const axis = (arr, isRight) => {
                        let lo = Infinity, hi = -Infinity;
                        arr.forEach(s => { const a = S.cols[s.col]; for (let i = i0; i <= i1; i++) { const v = a[i]; if (isFinite(v)) { if (v < lo) lo = v; if (v > hi) hi = v; } } });
                        if (lo === Infinity) return null;
                        const m0 = p.min0 || arr.some(s => s.min0);
                        if (isRight && p.band) { lo = 0; hi = Math.max(hi * 1.1, p.band[1] * 1.3); }
                        else if (arr.some(s => s.min0)) { lo = 0; hi = Math.max(hi * 1.15, 3); }   // Zaehler: ab 0, mind. 0..3
                        else {
                            if (m0) lo = Math.min(lo, 0);
                            const ms = p.minSpan || 1;
                            if (hi - lo < ms) { const c = (hi + lo) / 2; lo = c - ms / 2; hi = c + ms / 2; if (m0 && lo < 0) { hi -= lo; lo = 0; } }
                            const padv = (hi - lo) * 0.06; if (!m0 || lo > 0) lo -= padv; hi += padv;
                        }
                        const nt = niceTicks(lo, hi, Math.max(2, Math.floor((bottom - PT) / 34)));
                        return { nt, Y: v => bottom - (v - nt.lo) / (nt.hi - nt.lo) * (bottom - PT) };
                    };
                    const LA = axis(lines.filter(s => !s.right), false);
                    const RA = axis(lines.filter(s => s.right), true);
                    const rightCol = (lines.find(s => s.right) || {}).color || '#777';
                    ctx.font = '10px Arial'; ctx.textBaseline = 'middle';
                    if (RA && p.band) {
                        // Zielband: aus der VPD-Regelung (geloggter Sollbereich je Zeile), sonst fest
                        ctx.fillStyle = 'rgba(39,174,96,0.12)';
                        const B0 = S.cols['VPD_Soll_min'], B1 = S.cols['VPD_Soll_max'];
                        let logged = false;
                        for (let i = i0; i <= i1; i++) {
                            if (!isFinite(B0[i]) || !isFinite(B1[i])) continue;
                            logged = true;
                            const tn = (i + 1 < N && S.t[i + 1] - S.t[i] <= GAP) ? S.t[i + 1] : S.t[i] + S.rowDt;
                            const x0 = Math.floor(Math.max(PL, X(S.t[i]))), x1 = Math.ceil(Math.min(w - PR, X(tn)));
                            if (x1 <= x0) continue;
                            const y1 = RA.Y(B1[i]), y0 = RA.Y(B0[i]);
                            ctx.fillRect(x0, y1, x1 - x0, y0 - y1);
                        }
                        if (!logged) {
                            ctx.fillStyle = 'rgba(39,174,96,0.10)';
                            const y1 = RA.Y(p.band[1]), y0 = RA.Y(p.band[0]);
                            ctx.fillRect(PL, y1, pw, y0 - y1);
                        }
                    }
                    const grid = (A, side) => {
                        A.nt.ticks.forEach(v => {
                            const y = Math.round(A.Y(v)) + 0.5;
                            if (y < PT - 1 || y > bottom + 1) return;
                            if (side === 'l' || !LA) { ctx.strokeStyle = '#ececec'; ctx.beginPath(); ctx.moveTo(PL, y); ctx.lineTo(w - PR, y); ctx.stroke(); }
                            ctx.fillStyle = side === 'l' ? '#777' : rightCol;
                            ctx.textAlign = side === 'l' ? 'right' : 'left';
                            ctx.fillText(v.toFixed(tickDec(A.nt.step)), side === 'l' ? PL - 5 : w - PR + 5, y);
                        });
                    };
                    if (LA) grid(LA, 'l');
                    if (RA) grid(RA, 'r');
                    ctx.save(); ctx.beginPath(); ctx.rect(PL, PT, pw, bottom - PT); ctx.clip();
                    lines.forEach(s => { const A = s.right ? RA : LA; if (A) { any = true; drawSeries(ctx, X, A.Y, S.cols[s.col], i0, i1, s, bottom); } });
                    ctx.restore();
                    states.forEach((s, k) => {
                        const y = h - PB - (k + 1) * 9 + 1, a = S.cols[s.col];
                        ctx.fillStyle = '#f1f1f1'; ctx.fillRect(PL, y, pw, 7);
                        ctx.fillStyle = s.color;
                        for (let i = i0; i <= i1; i++) {
                            if (!(a[i] > 0)) continue;
                            any = true;
                            const tn = (i + 1 < N && S.t[i + 1] - S.t[i] <= GAP) ? S.t[i + 1] : S.t[i] + S.rowDt;
                            const x0 = Math.max(PL, X(S.t[i])), x1 = Math.min(w - PR, X(tn));
                            if (x1 > x0) ctx.fillRect(x0, y, x1 - x0, 7);
                        }
                    });
                    if (!any && !states.length) {
                        ctx.fillStyle = '#aaa'; ctx.textAlign = 'center'; ctx.font = '12px Arial';
                        ctx.fillText(L('Keine Daten im Ausschnitt', 'No data in this window'), PL + pw / 2, (PT + bottom) / 2);
                    }
                    drawMarks(g, X); drawHover(g, X);

                    // Legende mit Wert am Cursor (bzw. letzter Wert im Ausschnitt)
                    const idx = S.hoverT !== null ? nearestIdx(S.hoverT) : Math.min(N - 1, lowerBound(S.t, S.v1) - 1);
                    p.leg.innerHTML = act.map(s => {
                        const v = idx >= 0 ? S.cols[s.col][idx] : NaN;
                        const txt = s.state ? (isFinite(v) ? (v > 0 ? L('an', 'on') : L('aus', 'off')) : '–') : fmt(v, s.dec) + ' ' + s.unit;
                        return '<span class="ch-li"><span class="ch-sw" style="background:' + s.color + '"></span>' + L(s.de, s.en) + ' <b>' + txt + '</b></span>';
                    }).join('');
                }

                const nozVal = c => S.nozMode === 'rate' ? c.r : S.nozMode === 'dur' ? c.d : c.p;
                const nozMed = c => S.nozMode === 'rate' ? c.mr : S.nozMode === 'dur' ? c.md : c.mp;
                const nozDec = () => S.nozMode === 'cyc' ? 0 : 1;
                const nozUnitLong = () => S.nozMode === 'rate' ? L('Impulse/s', 'pulses/s') : S.nozMode === 'dur' ? L('Laufzeit je Zyklus (s)', 'run time per cycle (s)') : L('Impulse je Zyklus', 'pulses per cycle');
                const nozUnit = () => S.nozMode === 'rate' ? L('Imp/s', 'p/s') : S.nozMode === 'dur' ? L('s/Zyklus', 's/cycle') : L('Imp/Zyklus', 'p/cycle');
                function nearestCycle(a, t) {
                    if (!a.length) return -1;
                    let i = lowerBoundT(a, t); if (i >= a.length) i = a.length - 1;
                    if (i > 0 && Math.abs(a[i - 1].t - t) < Math.abs(a[i].t - t)) i--;
                    return Math.abs(a[i].t - t) <= 40 * 60000 ? i : -1;
                }

                function drawNozzle(p) {
                    const g = prepCanvas(p); if (!g) return;
                    const { ctx, w, h, pw } = g;
                    const X = t => PL + (t - S.v0) / (S.v1 - S.v0) * pw;
                    drawFrame(g, X);
                    const bottom = h - PB, vis = [];
                    let lo = Infinity, hi = -Infinity;
                    S.noz.forEach((a, k) => {
                        if (!S.nozOn[k]) return;
                        const i0 = Math.max(0, lowerBoundT(a, S.v0) - 1), i1 = Math.min(a.length - 1, lowerBoundT(a, S.v1));
                        vis.push({ k, a, i0, i1 });
                        for (let i = i0; i <= i1; i++) { const v = nozVal(a[i]); if (v < lo) lo = v; if (v > hi) hi = v; }
                    });
                    const dec = nozDec();
                    if (lo === Infinity) {
                        ctx.fillStyle = '#aaa'; ctx.textAlign = 'center'; ctx.font = '12px Arial';
                        ctx.fillText(L('Keine Gießzyklen im Ausschnitt', 'No watering cycles in this window'), PL + pw / 2, (PT + bottom) / 2);
                    } else {
                        lo = Math.max(0, lo * 0.85); hi = hi * 1.1; if (hi - lo < 1) hi = lo + 1;
                        const nt = niceTicks(lo, hi, Math.max(2, Math.floor((bottom - PT) / 34)));
                        const Y = v => bottom - (v - nt.lo) / (nt.hi - nt.lo) * (bottom - PT);
                        ctx.font = '10px Arial'; ctx.textBaseline = 'middle'; ctx.textAlign = 'right';
                        nt.ticks.forEach(v => {
                            const y = Math.round(Y(v)) + 0.5;
                            ctx.strokeStyle = '#ececec'; ctx.beginPath(); ctx.moveTo(PL, y); ctx.lineTo(w - PR, y); ctx.stroke();
                            ctx.fillStyle = '#777'; ctx.fillText(v.toFixed(tickDec(nt.step)), PL - 5, y);
                        });
                        ctx.save(); ctx.beginPath(); ctx.rect(PL, PT, pw, bottom - PT); ctx.clip();
                        vis.forEach(({ k, a, i0, i1 }) => {
                            const col = NOZ_COLORS[k];
                            for (let i = i0; i <= i1; i++) {
                                const c = a[i], v = nozVal(c), m = nozMed(c), x = X(c.t), y = Y(v);
                                ctx.globalAlpha = 0.5; ctx.fillStyle = col;
                                ctx.beginPath(); ctx.arc(x, y, 2.4, 0, 7); ctx.fill(); ctx.globalAlpha = 1;
                                if (m > 0 && Math.abs(v - m) / m > OUTLIER) {
                                    ctx.strokeStyle = '#c0392b'; ctx.lineWidth = 1.5;
                                    ctx.beginPath(); ctx.arc(x, y, 5, 0, 7); ctx.stroke();
                                }
                            }
                            ctx.strokeStyle = col; ctx.lineWidth = 2; ctx.lineJoin = 'round';
                            ctx.beginPath();
                            let pen = false, lastT = 0;
                            for (let i = i0; i <= i1; i++) {
                                const c = a[i], x = X(c.t), y = Y(nozMed(c));
                                if (!pen || c.t - lastT > 3 * HOUR) { ctx.moveTo(x, y); pen = true; } else ctx.lineTo(x, y);
                                lastT = c.t;
                            }
                            ctx.stroke();
                        });
                        ctx.restore();
                    }
                    drawMarks(g, X); drawHover(g, X);

                    p.leg.innerHTML = '<span class="ch-li" style="color:#888;">' + nozUnitLong() + '</span>' +
                        vis.map(({ k, a }) => {
                            const i = S.hoverT !== null ? nearestCycle(a, S.hoverT) : lowerBoundT(a, S.v1) - 1;
                            const c = i >= 0 ? a[i] : null;
                            const txt = c ? fmt(nozVal(c), dec) + ' <span style="color:#999;font-weight:normal;">(' + c.p + ' / ' + c.d.toFixed(1) + ' s)</span>' : '–';
                            return '<span class="ch-li"><span class="ch-sw" style="background:' + NOZ_COLORS[k] + '"></span>' + L('Beh. ', 'Res. ') + (k + 1) + ' <b>' + txt + '</b></span>';
                        }).join('');
                }

                // ---------- Kennzahlen ----------
                // VPD im Zielbereich: mit VPD-Regelung (geloggter Sollbereich) wird der Blatt-VPD
                // gegen dessen Band geprueft (Tag und Nacht), sonst der Luft-VPD gegen 0,8-1,2 bei Licht.
                function vpdOnTarget(i0, i1) {
                    const B0 = S.cols['VPD_Soll_min'], B1 = S.cols['VPD_Soll_max'], LF = S.cols['VPD_Blatt_kPa'],
                          AIR = S.cols['ZeltVPD_kPa'], lm = S.cols['LichtMaske'];
                    let inb = 0, tot = 0, logged = false;
                    for (let i = i0; i <= i1; i++) {
                        if (isFinite(B0[i]) && isFinite(B1[i]) && isFinite(LF[i])) {
                            logged = true; tot++;
                            if (LF[i] >= B0[i] && LF[i] <= B1[i]) inb++;
                        } else if (!logged && isFinite(AIR[i]) && lm[i] > 0) {
                            tot++;
                            if (AIR[i] >= VPD_BAND[0] && AIR[i] <= VPD_BAND[1]) inb++;
                        }
                    }
                    return { inb, tot, logged };
                }

                function renderStats() {
                    const box = $('chStats'), N = S.t.length;
                    const i0 = lowerBound(S.t, S.v0), i1 = lowerBound(S.t, S.v1) - 1;
                    if (!N || i1 < i0) { box.innerHTML = ''; return; }
                    const lm = S.cols['LichtMaske'];
                    let html = '';
                    SERIES.filter(s => S.on[s.id] && !s.state).forEach(s => {
                        const a = S.cols[s.col];
                        let mn = Infinity, mx = -Infinity, sum = 0, n = 0, sd = 0, nd = 0, sn = 0, nn = 0;
                        for (let i = i0; i <= i1; i++) {
                            const v = a[i]; if (!isFinite(v)) continue;
                            if (v < mn) mn = v; if (v > mx) mx = v; sum += v; n++;
                            if (lm[i] > 0) { sd += v; nd++; } else if (lm[i] === 0) { sn += v; nn++; }
                        }
                        if (!n) return;
                        let main = '⌀ ' + fmt(sum / n, s.dec), extra = '';
                        if (s.id === 'vpd') {
                            const q = vpdOnTarget(i0, i1);
                            if (q.tot) extra = '<div class="ch-c-sub">' + (q.logged ? L('Im Zielbereich der Regelung: ', 'In the controller’s target range: ')
                                                                                   : L('Im Zielbereich bei Licht: ', 'In target range (lights on): ')) +
                                               '<b>' + Math.round(q.inb / q.tot * 100) + ' %</b></div>';
                            if (nd) main = '⌀ ' + fmt(sd / nd, s.dec) + ' <small>☀️</small>';
                        } else if (s.id === 'ppfd') {
                            let mol = 0;
                            for (let i = i0; i < i1; i++) { const v = a[i]; if (isFinite(v)) mol += v * Math.min(S.t[i + 1] - S.t[i], 10 * 60000) / 1000; }
                            main = fmt(mol / 1e6, 1) + ' <small>mol/m²</small>';
                            extra = '<div class="ch-c-sub">' + L('Lichtsumme im Ausschnitt · ⌀ bei Licht ', 'Light sum in window · ⌀ lights on ') + fmt(nd ? sd / nd : NaN, 0) + '</div>';
                        } else if (nd && nn && s.panel !== 'tank') {
                            extra = '<div class="ch-c-sub">☀️ ' + fmt(sd / nd, s.dec) + ' · 🌙 ' + fmt(sn / nn, s.dec) + ' ' + s.unit + '</div>';
                        }
                        html += '<div class="ch-card" style="border-left-color:' + s.color + ';"><div class="ch-c-title">' + L(s.de, s.en) +
                                ' <span style="font-weight:normal;color:#999;">' + s.unit + '</span></div>' +
                                '<div class="ch-c-main">' + main + '</div>' +
                                '<div class="ch-c-sub">min ' + fmt(mn, s.dec) + ' · max ' + fmt(mx, s.dec) + '</div>' + extra + '</div>';
                    });
                    const inView = type => S.marks.filter(m => m.type === type && m.t >= S.v0 && m.t <= S.v1);
                    const rs = inView('restart').length, ms = inView('mains'), ss = inView('sensor');
                    const ssMin = Math.round(ss.reduce((x, m) => x + m.n, 0) * S.rowDt / 60000);
                    const nEv = rs + ms.length + ss.length;
                    // RS485-Diagnose (ab FW 1.20.2): unbeantwortete Abfragen an den Zelt-Sensor
                    let pq = 0, to = 0;
                    const PA = S.cols['MS2_Abfragen'], TO = S.cols['MS2_Timeouts'];
                    for (let i = i0; i <= i1; i++) { if (isFinite(PA[i])) pq += PA[i]; if (isFinite(TO[i])) to += TO[i]; }
                    const toLine = pq ? '<div class="ch-c-sub">📡 ' + L('Unbeantwortete Abfragen', 'Unanswered polls') + ': ' + to + ' / ' + pq +
                                        ' (' + (to / pq * 100).toFixed(2) + ' %)</div>' : '';
                    html += '<div class="ch-card" style="border-left-color:' + (nEv ? '#c0392b' : '#27ae60') + ';"><div class="ch-c-title">' + L('Ereignisse', 'Events') + '</div>' +
                            '<div class="ch-c-main">' + (nEv ? '⚠️ ' + nEv : '✅ 0') + '</div>' +
                            '<div class="ch-c-sub">↻ ' + L('Neustarts', 'Restarts') + ': ' + rs + ' · ⚡ ' + L('Netzaussetzer', 'Mains dropouts') + ': ' + ms.reduce((x, m) => x + m.n, 0) + '</div>' +
                            '<div class="ch-c-sub">▲ ' + L('Zelt-Sensor-Aussetzer', 'Tent sensor dropouts') + ': ' + ss.length + (ss.length ? ' (~' + ssMin + ' min)' : '') + '</div>' +
                            toLine + '</div>';
                    box.innerHTML = html;
                }

                function renderDays() {
                    const tb = $('chDays'), N = S.t.length;
                    if (!N) { tb.innerHTML = ''; return; }
                    const C = n => S.cols[n];
                    const wd = L(['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'], ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']);
                    const avg = (s, n, d) => n ? (s / n).toFixed(d) : '–';
                    let body = '';
                    for (let d = S.d1; d > S.d0;) {
                        const e = d; d = addDays(d, -1);
                        const i0 = lowerBound(S.t, d), i1 = lowerBound(S.t, e) - 1;
                        if (i1 < i0) continue;
                        let mol = 0, lightMs = 0, cyc = 0, vpdS = 0, vpdN = 0, vpdIn = 0;
                        const T = [0, 0, 0, 0], H = [0, 0, 0, 0];
                        for (let i = i0; i <= i1; i++) {
                            const dt = i < i1 ? Math.min(S.t[i + 1] - S.t[i], 10 * 60000) : S.rowDt;
                            const l = C('LichtMaske')[i], pp = C('ZeltPPFD_umol')[i], t = C('ZeltTemp_C')[i], hm = C('ZeltFeuchte_%')[i], v = C('ZeltVPD_kPa')[i];
                            if (isFinite(pp)) mol += pp * dt / 1000;
                            if (l > 0) lightMs += dt;
                            if (isFinite(t)) { if (l > 0) { T[0] += t; T[1]++; } else if (l === 0) { T[2] += t; T[3]++; } }
                            if (isFinite(hm)) { if (l > 0) { H[0] += hm; H[1]++; } else if (l === 0) { H[2] += hm; H[3]++; } }
                            if (isFinite(v) && l > 0) { vpdS += v; vpdN++; if (v >= VPD_BAND[0] && v <= VPD_BAND[1]) vpdIn++; }
                            for (let k = 1; k <= 4; k++) if (isFinite(C('ImpulseBehaelter' + k)[i])) cyc++;
                        }
                        const rs = S.marks.filter(m => m.type === 'restart' && m.t >= d && m.t < e).length;
                        const mn = S.marks.filter(m => m.type === 'mains' && m.t >= d && m.t < e).reduce((x, m) => x + m.n, 0);
                        const sa = S.marks.filter(m => m.type === 'sensor' && m.t >= d && m.t < e).length;
                        const q = vpdOnTarget(i0, i1);
                        let toD = 0, hasTo = false;
                        for (let i = i0; i <= i1; i++) { const v = C('MS2_Timeouts')[i]; if (isFinite(v)) { toD += v; hasTo = true; } }
                        body += '<tr data-day="' + d + '"><td>' + wd[new Date(d).getDay()] + ' ' + fmtDate(d) + '</td>' +
                                '<td><b>' + (mol / 1e6).toFixed(1) + '</b></td><td>' + (lightMs / HOUR).toFixed(1) + '</td>' +
                                '<td>' + avg(T[0], T[1], 1) + ' / ' + avg(T[2], T[3], 1) + '</td>' +
                                '<td>' + avg(H[0], H[1], 0) + ' / ' + avg(H[2], H[3], 0) + '</td>' +
                                '<td>' + avg(vpdS, vpdN, 2) + '</td><td>' + (q.tot ? Math.round(q.inb / q.tot * 100) + ' %' + (q.logged ? ' 🎯' : '') : '–') + '</td>' +
                                '<td>' + cyc + '</td>' +
                                '<td>' + (rs ? '<span style="color:#c0392b;font-weight:bold;">' + rs + '</span>' : '0') + (mn ? ' ⚡' + mn : '') + '</td>' +
                                '<td>' + (sa ? '<span style="color:#c0392b;font-weight:bold;">' + sa + '</span>' : '0') +
                                (hasTo ? ' <span style="color:#888;" title="' + L('unbeantwortete RS485-Abfragen', 'unanswered RS485 polls') + '">📡' + toD + '</span>' : '') + '</td></tr>';
                    }
                    tb.innerHTML = '<tr><th>' + L('Tag', 'Day') + '</th><th>DLI<br><small>mol/m²·d</small></th><th>' + L('Licht', 'Light') + '<br><small>h</small></th>' +
                        '<th>' + L('Zelt', 'Tent') + ' °C<br><small>☀️ / 🌙</small></th><th>' + L('Feuchte', 'Humidity') + ' %<br><small>☀️ / 🌙</small></th>' +
                        '<th>VPD ☀️<br><small>kPa</small></th><th>VPD ' + L('im Ziel', 'on target') + '<br><small>' + L('🎯 = Soll der Regelung', '🎯 = controller target') + '</small></th>' +
                        '<th>' + L('Gießzyklen', 'Watering cycles') + '</th><th>' + L('Neustarts', 'Restarts') + '</th>' +
                        '<th>' + L('Sensor-', 'Sensor') + '<br><small>' + L('Aussetzer ▲', 'dropouts ▲') + '</small></th></tr>' + body;
                    tb.querySelectorAll('tr[data-day]').forEach(tr => tr.addEventListener('click', () => {
                        S.v0 = +tr.dataset.day; S.v1 = addDays(S.v0, 1); clampView(); redraw();
                        $('chPanels').scrollIntoView({ behavior: 'smooth', block: 'start' });
                    }));
                }

                function renderNozCards() {
                    const box = $('chNozCards'), data = [], meds = [];
                    S.noz.forEach((a, k) => {
                        if (!S.nozOn[k]) return;
                        const i0 = lowerBoundT(a, S.v0), i1 = lowerBoundT(a, S.v1) - 1, vs = [];
                        let outl = 0;
                        for (let i = i0; i <= i1; i++) {
                            const v = nozVal(a[i]), m = nozMed(a[i]); vs.push(v);
                            if (m > 0 && Math.abs(v - m) / m > OUTLIER) outl++;
                        }
                        data.push({ k, vs, outl });
                        if (vs.length) meds.push(median(vs));
                    });
                    const allMean = meds.length ? meds.reduce((x, y) => x + y, 0) / meds.length : NaN;
                    const maxMed = Math.max(0, ...meds);
                    const dec = nozDec();
                    const unit = nozUnit();
                    // Bei der Laufzeit bedeutet "steigt" weniger Durchfluss (Duese setzt zu), sonst umgekehrt
                    const inv = S.nozMode === 'dur';
                    box.innerHTML = data.map(({ k, vs, outl }) => {
                        const col = NOZ_COLORS[k], n = vs.length;
                        const head = '<div class="ch-c-title">' + L('Behälter ', 'Reservoir ') + (k + 1);
                        if (!n) return '<div class="ch-card" style="border-left-color:' + col + ';">' + head + '</div><div class="ch-c-sub">' + L('Keine Zyklen im Ausschnitt', 'No cycles in this window') + '</div></div>';
                        const med = median(vs), mean = vs.reduce((x, y) => x + y, 0) / n;
                        const sd = Math.sqrt(vs.reduce((x, y) => x + (y - mean) * (y - mean), 0) / n);
                        const cv = mean > 0 ? sd / mean * 100 : 0;
                        let trend = NaN;
                        if (n >= 6) { const th = Math.floor(n / 3), a = median(vs.slice(0, th)), b = median(vs.slice(n - th)); if (a > 0) trend = (b - a) / a * 100; }
                        const dev = allMean > 0 ? (med - allMean) / allMean * 100 : NaN;
                        let amp = '🟢', txt = L('stabil', 'stable');
                        if (n < 6) { amp = '⚪'; txt = L('zu wenige Zyklen für eine Bewertung', 'too few cycles to rate'); }
                        else if (inv ? trend >= 10 : trend <= -10) { amp = '🟠'; txt = inv ? L('steigt – Düse/Filter prüfen', 'rising – check nozzle/filter') : L('fällt – Düse/Filter prüfen', 'dropping – check nozzle/filter'); }
                        else if (inv ? trend <= -10 : trend >= 10) { amp = '🟠'; txt = inv ? L('fällt – Leck oder lose Düse?', 'dropping – leak or loose nozzle?') : L('steigt – Leck oder lose Düse?', 'rising – leak or loose nozzle?'); }
                        else if (cv > 15) { amp = '🟡'; txt = L('unruhig – Luft oder Schmutz?', 'erratic – air or dirt?'); }
                        return '<div class="ch-card" style="border-left-color:' + col + ';">' + head + ' <span style="float:right;">' + amp + '</span></div>' +
                               '<div class="ch-c-main">' + fmt(med, dec) + ' <small>' + unit + ' (Median)</small></div>' +
                               '<div class="ch-bar"><span style="width:' + (maxMed > 0 ? med / maxMed * 100 : 0) + '%;background:' + col + ';"></span></div>' +
                               '<div class="ch-c-sub"><b>' + txt + '</b></div>' +
                               '<div class="ch-c-sub">' + n + L(' Zyklen', ' cycles') + ' · ' + L('Streuung', 'spread') + ' ±' + fmt(cv, 0) + ' % · Trend ' + signed(trend, 0) + (isFinite(trend) ? ' %' : '') + '</div>' +
                               '<div class="ch-c-sub">' + L('Ausreißer', 'Outliers') + ': ' + outl + ' · ' + L('zum ⌀ aller', 'vs. ⌀ of all') + ': ' + signed(dev, 0) + (isFinite(dev) ? ' %' : '') + '</div></div>';
                    }).join('');
                }

                // ---------- Einstieg (von showTab()/setLang() aufgerufen) ----------
                window.chartsOnShow = function () {
                    if (!S.inited) {
                        S.inited = true;
                        loadPrefs(); buildDom(); wireControls(); buildToggles();
                        loadFileList().then(() => loadData(false))
                            .catch(e => { $('chStatus').textContent = L('Fehler: ', 'Error: ') + e; });
                    } else {
                        redraw();
                    }
                };
                window.chartsOnLangChange = function () {
                    if (!S.inited) return;
                    buildToggles(); buildWindowSelect(); renderDays(); S.statsKey = ''; redraw();
                };
            })();
            </script>

            <!-- ============ EC-Messungen (EC-Meter, Datei EC-Messungen.csv) ============ -->
            <h3 class="section-heading">🧪 <span class="de">EC-Messungen</span><span class="en">EC measurements</span></h3>
            <div class="status-box-info ch-toolbar">
                <label><span class="de">Zeitraum</span><span class="en">Period</span>
                    <select id="ecRange">
                        <option class="i18n-opt" value="30" data-de="30 Tage" data-en="30 days">30 Tage</option>
                        <option class="i18n-opt" value="90" data-de="90 Tage" data-en="90 days">90 Tage</option>
                        <option class="i18n-opt" value="0" data-de="Alles" data-en="All">Alles</option>
                    </select></label>
                <button id="ecReloadBtn" class="ch-btn">🔄 <span class="de">Neu laden</span><span class="en">Reload</span></button>
                <a class="ch-btn" style="text-decoration:none;" href="/api/logs/download?file=EC-Messungen.csv">⬇ CSV</a>
                <button id="ecUploadBtn" class="ch-btn">⬆ <span class="de">CSV hochladen</span><span class="en">Upload CSV</span></button>
                <input type="file" id="ecUploadFile" accept=".csv,text/csv" style="display:none;">
                <span id="ecStatus" class="ch-status"></span>
            </div>
            <div class="ch-panel">
                <div class="ch-head" id="ecLegend"></div>
                <canvas id="ecCanvas" class="ch-cv" style="height:220px;"></canvas>
            </div>
            <div class="ch-hoverbar"><span id="ecHover"></span></div>
            <div id="ecCards" class="ch-cards"></div>
            <div style="overflow-x:auto;margin-top:10px;"><table id="ecTable" class="ch-days"></table></div>
            <div class="ch-hint">
                <span class="de">Werte vom EC-Meter (LOG 2 s halten, dann Kategorie wählen), auf 25 °C kompensiert. Die Datei EC-Messungen.csv auf der SD-Karte sammelt alle Messungen tagesübergreifend; zusätzlich stehen sie in den Tageslogs (Spalten EC_…). Roter Ring = über Messbereich, hohler Punkt = ohne Temperaturfühler gemessen.</span>
                <span class="en">Values from the EC meter (hold LOG for 2 s, then pick a category), compensated to 25 °C. The file EC-Messungen.csv on the SD card collects all measurements across days; they are also in the daily logs (EC_… columns). Red ring = over range, hollow dot = measured without temperature probe.</span>
            </div>

            <script>
            (function () {
                'use strict';
                const FILE = 'EC-Messungen.csv', DAY = 86400000;
                const KAT = [
                    { key: 'Naehrloesung', de: 'Nährlösung', en: 'Nutrient solution', color: '#27ae60' },
                    { key: 'Ruecklauf',    de: 'Rücklauf',   en: 'Return',            color: '#2e86de' },
                    { key: 'Cal-Mag',      de: 'Cal-Mag',    en: 'Cal-Mag',           color: '#e67e22' },
                    { key: 'Rohwasser',    de: 'Rohwasser',  en: 'Tap water',         color: '#8e44ad' },
                    { key: 'VE-Wasser',    de: 'VE-Wasser',  en: 'Demineralized water', color: '#7f8c8d' },
                ];
                const E = { rows: [], loaded: false, wired: false, pts: [] };
                const $ = id => document.getElementById(id);
                function L(de, en) { return (typeof currentLang !== 'undefined' && currentLang === 'en') ? en : de; }
                const p2 = n => String(n).padStart(2, '0');
                const fDate = t => { const d = new Date(t); return p2(d.getDate()) + '.' + p2(d.getMonth() + 1) + '.' + d.getFullYear(); };
                const fTime = t => { const d = new Date(t); return p2(d.getHours()) + ':' + p2(d.getMinutes()); };
                const fMs = v => (v / 1000).toFixed(2);
                const sgn = v => (v >= 0 ? '+' : '') + v;

                function parse(txt) {
                    const lines = txt.split(/\r?\n/), h = (lines[0] || '').split(',');
                    const di = h.indexOf('Datum'), zi = h.indexOf('Zeit'), ki = h.indexOf('Kategorie'),
                          ei = h.indexOf('EC25_uS'), ti = h.indexOf('Temp_C'), hi = h.indexOf('Hinweis');
                    if (di < 0 || ki < 0 || ei < 0) return [];
                    const out = [];
                    for (let i = 1; i < lines.length; i++) {
                        if (!lines[i]) continue;
                        const c = lines[i].split(','), d = (c[di] || '').split('-').map(Number), z = (c[zi] || '').split(':').map(Number);
                        if (d.length < 3 || !d[0]) continue;
                        const k = KAT.findIndex(x => x.key === c[ki]); if (k < 0) continue;
                        const ec = parseFloat(c[ei]); if (!isFinite(ec)) continue;
                        const tp = parseFloat(c[ti]), note = hi >= 0 ? (c[hi] || '') : '';
                        out.push({ t: new Date(d[0], d[1] - 1, d[2], z[0] || 0, z[1] || 0, z[2] || 0).getTime(), k, ec,
                                   temp: isFinite(tp) ? tp : NaN, over: /Messbereich/.test(note), noTemp: /Temperatur/.test(note) });
                    }
                    return out.sort((a, b) => a.t - b.t);
                }

                // Bearbeitete EC-Messungen.csv wieder auf die Steuerung laden. Vorher normalisieren:
                // Excel speichert oft mit Semikolon + Dezimalkomma und deutschem Datum, Kategorien
                // werden ggf. frei geschrieben ("Nährlösung", "VE Wasser") -> auf die festen Namen.
                function katKey(s) {
                    const n = s.trim().toLowerCase().replace(/ä/g, 'ae').replace(/ö/g, 'oe').replace(/ü/g, 'ue').replace(/[\s_\-]/g, '');
                    const map = { naehrloesung: 'Naehrloesung', ruecklauf: 'Ruecklauf', calmag: 'Cal-Mag', nurcalmag: 'Cal-Mag',
                                  rohwasser: 'Rohwasser', vewasser: 'VE-Wasser' };
                    return map[n] || s.trim();
                }
                function normalize(txt) {
                    txt = txt.replace(/^\uFEFF/, '');
                    let lines = txt.split(/\r?\n/).filter(l => l.trim() !== '');
                    if ((lines[0] || '').indexOf(';') >= 0)   // Excel: ; als Trenner, , als Dezimalzeichen
                        lines = lines.map(l => l.replace(/,/g, '.').replace(/;/g, ','));
                    const h = (lines[0] || '').split(',').map(x => x.trim().replace(/^"|"$/g, ''));
                    const ki = h.indexOf('Kategorie'), di = h.indexOf('Datum'), zi = h.indexOf('Zeit');
                    const out = [h.join(',')];
                    for (let i = 1; i < lines.length; i++) {
                        const c = lines[i].split(',').map(x => x.trim().replace(/^"|"$/g, ''));
                        if (ki >= 0 && c[ki] !== undefined) c[ki] = katKey(c[ki]);
                        if (di >= 0 && /^\d{1,2}\.\d{1,2}\.\d{4}$/.test(c[di] || '')) {
                            const [d, mo, y] = c[di].split('.');
                            c[di] = y + '-' + p2(+mo) + '-' + p2(+d);
                        }
                        if (zi >= 0 && /^\d{1,2}:\d{2}$/.test(c[zi] || '')) c[zi] += ':00';
                        if (zi >= 0 && /^\d:/.test(c[zi] || '')) c[zi] = '0' + c[zi];
                        out.push(c.join(','));
                    }
                    return out.join('\n') + '\n';
                }
                async function upload(file) {
                    const txt = normalize(await file.text());
                    const lines = txt.split('\n').filter(l => l);
                    const rows = parse(txt);
                    if (!rows.length) {
                        alert(L('Keine gültigen Messungen erkannt – Kopfzeile muss Datum, Zeit, Kategorie, EC25_uS enthalten.',
                                'No valid measurements found – header must contain Datum, Zeit, Kategorie, EC25_uS.'));
                        return;
                    }
                    const ki = lines[0].split(',').indexOf('Kategorie');
                    const unbekannt = [...new Set(lines.slice(1).map(l => l.split(',')[ki])
                                       .filter(k => k && !KAT.some(x => x.key === k)))];
                    const skip = lines.length - 1 - rows.length;
                    let msg = L(rows.length + ' Messungen erkannt', rows.length + ' measurements recognized');
                    if (skip > 0) msg += L(', ' + skip + ' Zeilen werden in der Auswertung ignoriert', ', ' + skip + ' rows will be ignored');
                    if (unbekannt.length) msg += L(' (unbekannte Kategorie: ', ' (unknown category: ') + unbekannt.join(', ') + ')';
                    msg += L('.\n\nEC-Messungen.csv auf der Steuerung ersetzen? Die bisherige Datei bleibt als EC-Messungen_vorher.csv erhalten.',
                             '.\n\nReplace EC-Messungen.csv on the controller? The previous file is kept as EC-Messungen_vorher.csv.');
                    if (!confirm(msg)) return;
                    $('ecStatus').textContent = L('Lade hoch …', 'Uploading …');
                    const fd = new FormData();
                    fd.append('file', new Blob([txt], { type: 'text/csv' }), FILE);
                    try {
                        const r = await fetch('/api/logs/upload-ec', { method: 'POST', body: fd });
                        const d = await r.json().catch(() => ({}));
                        if (!r.ok || !d.ok) { $('ecStatus').textContent = L('Upload fehlgeschlagen: ', 'Upload failed: ') + (d.error || r.status); return; }
                        await load();
                    } catch (e) {
                        $('ecStatus').textContent = L('Upload fehlgeschlagen: ', 'Upload failed: ') + e;
                    }
                }

                async function load() {
                    $('ecStatus').textContent = L('Lade …', 'Loading …');
                    try {
                        const r = await fetch('/api/logs/download?file=' + encodeURIComponent(FILE));
                        E.rows = r.ok ? parse(await r.text()) : [];
                        E.loaded = true;
                        render();
                    } catch (e) {
                        $('ecStatus').textContent = L('Fehler beim Laden: ', 'Load error: ') + e;
                    }
                }

                function visible() {
                    const days = parseInt($('ecRange').value) || 0;
                    if (!days) return E.rows;
                    const from = Date.now() - days * DAY;
                    return E.rows.filter(r => r.t >= from);
                }

                function render() {
                    const rows = visible();
                    $('ecStatus').textContent = E.rows.length
                        ? rows.length + L(' von ', ' of ') + E.rows.length + L(' Messungen', ' measurements')
                        : L('Noch keine EC-Messungen gespeichert.', 'No EC measurements stored yet.');
                    $('ecLegend').innerHTML = '<span class="ch-title">EC 25 °C (mS/cm)</span>' + KAT.map((k, i) =>
                        '<span class="ch-li"><span class="ch-sw" style="background:' + k.color + ';"></span>' + L(k.de, k.en) +
                        ' (' + rows.filter(r => r.k === i).length + ')</span>').join('');
                    drawChart(rows);
                    renderCards();
                    renderTable(rows);
                }

                function drawChart(rows) {
                    const cv = $('ecCanvas'), dpr = window.devicePixelRatio || 1;
                    const W = cv.clientWidth || 600, H = cv.clientHeight || 220;
                    cv.width = W * dpr; cv.height = H * dpr;
                    const ctx = cv.getContext('2d');
                    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
                    ctx.clearRect(0, 0, W, H);
                    E.pts = [];
                    const PL = 46, PR = 14, PT = 10, PB = 24, pw = W - PL - PR, ph = H - PT - PB;
                    ctx.font = '11px sans-serif'; ctx.fillStyle = '#888';
                    if (!rows.length) { ctx.fillText(L('Keine Messungen im Zeitraum', 'No measurements in this period'), PL, PT + ph / 2); return; }
                    let t0 = rows[0].t, t1 = rows[rows.length - 1].t;
                    if (t1 - t0 < DAY) { t0 -= DAY / 2; t1 += DAY / 2; }
                    const pad = (t1 - t0) * 0.03; t0 -= pad; t1 += pad;
                    const yMax = Math.max(0.5, Math.ceil(Math.max(...rows.map(r => r.ec / 1000)) * 1.1 * 4) / 4);
                    const X = t => PL + (t - t0) / (t1 - t0) * pw, Y = v => PT + ph - v / yMax * ph;
                    ctx.strokeStyle = '#eee'; ctx.lineWidth = 1; ctx.textAlign = 'right';
                    for (let i = 0; i <= 5; i++) {
                        const v = yMax * i / 5, y = Y(v);
                        ctx.beginPath(); ctx.moveTo(PL, y); ctx.lineTo(PL + pw, y); ctx.stroke();
                        ctx.fillText(v.toFixed(2), PL - 5, y + 4);
                    }
                    ctx.textAlign = 'center';
                    const nx = Math.max(2, Math.min(7, Math.floor(pw / 90)));
                    for (let i = 0; i <= nx; i++) {
                        const t = t0 + (t1 - t0) * i / nx, d = new Date(t);
                        ctx.fillText(p2(d.getDate()) + '.' + p2(d.getMonth() + 1) + '.', X(t), H - 6);
                    }
                    KAT.forEach((kat, k) => {
                        const rs = rows.filter(r => r.k === k); if (!rs.length) return;
                        ctx.strokeStyle = kat.color; ctx.lineWidth = 1.5; ctx.beginPath();
                        rs.forEach((r, i) => { const x = X(r.t), y = Y(r.ec / 1000); if (i) ctx.lineTo(x, y); else ctx.moveTo(x, y); });
                        ctx.stroke();
                        rs.forEach(r => {
                            const x = X(r.t), y = Y(r.ec / 1000);
                            ctx.beginPath(); ctx.arc(x, y, 4, 0, 2 * Math.PI);
                            if (r.noTemp) { ctx.fillStyle = '#fff'; ctx.fill(); ctx.strokeStyle = kat.color; ctx.lineWidth = 1.5; ctx.stroke(); }
                            else { ctx.fillStyle = kat.color; ctx.fill(); }
                            if (r.over) { ctx.beginPath(); ctx.arc(x, y, 7, 0, 2 * Math.PI); ctx.strokeStyle = '#e74c3c'; ctx.lineWidth = 1.5; ctx.stroke(); }
                            E.pts.push({ x, y, r });
                        });
                    });
                }

                function hoverText(r) {
                    return fDate(r.t) + ' ' + fTime(r.t) + ' · ' + L(KAT[r.k].de, KAT[r.k].en) + ' · ' + fMs(r.ec) + ' mS/cm' +
                           (isFinite(r.temp) ? ' · ' + r.temp.toFixed(1) + ' °C' : '') +
                           (r.over ? L(' · über Messbereich', ' · over range') : '') + (r.noTemp ? L(' · ohne Temperaturfühler', ' · no temperature probe') : '');
                }

                function renderCards() {
                    const lastOf = k => { for (let i = E.rows.length - 1; i >= 0; i--) if (E.rows[i].k === k) return i; return -1; };
                    const prevOf = (k, before) => { for (let i = before - 1; i >= 0; i--) if (E.rows[i].k === k) return E.rows[i]; return null; };
                    $('ecCards').innerHTML = KAT.map((kat, k) => {
                        const head = '<div class="ch-c-title">' + L(kat.de, kat.en) + '</div>';
                        const li = lastOf(k);
                        if (li < 0) return '<div class="ch-card" style="border-left-color:' + kat.color + ';">' + head + '<div class="ch-c-sub">' + L('Noch keine Messung', 'No measurement yet') + '</div></div>';
                        const r = E.rows[li], p = prevOf(k, li);
                        const sub = fDate(r.t) + ' ' + fTime(r.t) + (isFinite(r.temp) ? ' · ' + r.temp.toFixed(1) + ' °C' : '');
                        let delta = '';
                        if (p) {
                            const d = (r.ec - p.ec) / 1000, pc = p.ec > 0 ? (r.ec - p.ec) / p.ec * 100 : 0;
                            delta = L('zur vorigen', 'vs. previous') + ' (' + fDate(p.t) + '): ' + sgn(d.toFixed(2)) + ' mS/cm (' + sgn(pc.toFixed(0)) + ' %)';
                        }
                        let extra = '';
                        if (k === 1) {   // Ruecklauf: Unterschied zur letzten Naehrloesung -- zeigt, was die Pflanzen aufnehmen
                            const ni = lastOf(0);
                            if (ni >= 0) {
                                const n = E.rows[ni], d = (r.ec - n.ec) / 1000;
                                extra = '<div class="ch-c-sub">' + L('zur Nährlösung', 'vs. nutrient solution') + ' (' + fDate(n.t) + '): ' + sgn(d.toFixed(2)) + ' mS/cm</div>';
                            }
                        }
                        return '<div class="ch-card" style="border-left-color:' + kat.color + ';">' + head +
                               '<div class="ch-c-main">' + fMs(r.ec) + ' <small>mS/cm</small></div>' +
                               '<div class="ch-c-sub">' + sub + (r.over ? ' · <b style="color:#e74c3c;">' + L('über Messbereich', 'over range') + '</b>' : '') + '</div>' +
                               (delta ? '<div class="ch-c-sub">' + delta + '</div>' : '') + extra + '</div>';
                    }).join('');
                }

                function renderTable(rows) {
                    const last = rows.slice(-50).reverse();
                    if (!last.length) { $('ecTable').innerHTML = ''; return; }
                    $('ecTable').innerHTML = '<tr><th>' + L('Datum', 'Date') + '</th><th>' + L('Zeit', 'Time') + '</th><th style="text-align:left;">' +
                        L('Kategorie', 'Category') + '</th><th>mS/cm</th><th>µS/cm</th><th>°C</th><th style="text-align:left;">' + L('Hinweis', 'Note') + '</th></tr>' +
                        last.map(r => '<tr><td>' + fDate(r.t) + '</td><td>' + fTime(r.t) + '</td><td style="text-align:left;"><span class="ch-sw" style="background:' +
                            KAT[r.k].color + ';"></span>' + L(KAT[r.k].de, KAT[r.k].en) + '</td><td><b>' + fMs(r.ec) + '</b></td><td>' + Math.round(r.ec) +
                            '</td><td>' + (isFinite(r.temp) ? r.temp.toFixed(1) : '–') + '</td><td style="text-align:left;">' +
                            (r.over ? L('über Messbereich', 'over range') : r.noTemp ? L('ohne Temperaturfühler', 'no temperature probe') : '') + '</td></tr>').join('') +
                        (rows.length > 50 ? '<tr><td colspan="7" style="text-align:left;color:#999;">' + L('… ältere Messungen in der CSV-Datei', '… older measurements in the CSV file') + '</td></tr>' : '');
                }

                function wire() {
                    E.wired = true;
                    $('ecReloadBtn').addEventListener('click', load);
                    $('ecUploadBtn').addEventListener('click', () => $('ecUploadFile').click());
                    $('ecUploadFile').addEventListener('change', ev => {
                        const f = ev.target.files[0]; ev.target.value = '';
                        if (f) upload(f);
                    });
                    $('ecRange').addEventListener('change', () => { if (E.loaded) render(); });
                    const cv = $('ecCanvas');
                    cv.addEventListener('mousemove', ev => {
                        const b = cv.getBoundingClientRect(), mx = ev.clientX - b.left, my = ev.clientY - b.top;
                        let best = null, bd = 144;
                        E.pts.forEach(p => { const d = (p.x - mx) ** 2 + (p.y - my) ** 2; if (d < bd) { bd = d; best = p; } });
                        $('ecHover').textContent = best ? hoverText(best.r) : '';
                    });
                    cv.addEventListener('mouseleave', () => { $('ecHover').textContent = ''; });
                    window.addEventListener('resize', () => { if (E.loaded && $('chartsTab').classList.contains('active')) render(); });
                }

                const prevShow = window.chartsOnShow, prevLang = window.chartsOnLangChange;
                window.chartsOnShow = function () {
                    if (typeof prevShow === 'function') prevShow();
                    if (!E.wired) wire();
                    if (!E.loaded) load(); else render();
                };
                window.chartsOnLangChange = function () {
                    if (typeof prevLang === 'function') prevLang();
                    if (E.loaded) render();
                };
            })();
            </script>

            <!-- ============ Rücklauf (Füllstand-Modul, Datei Ruecklauf.csv) ============ -->
            <h3 class="section-heading">♻️ <span class="de">Rücklauf</span><span class="en">Return flow</span></h3>
            <div class="status-box-info ch-toolbar">
                <label><span class="de">Zeitraum</span><span class="en">Period</span>
                    <select id="rlRange">
                        <option class="i18n-opt" value="14" data-de="14 Tage" data-en="14 days">14 Tage</option>
                        <option class="i18n-opt" value="30" data-de="30 Tage" data-en="30 days">30 Tage</option>
                        <option class="i18n-opt" value="90" data-de="90 Tage" data-en="90 days">90 Tage</option>
                        <option class="i18n-opt" value="0" data-de="Alles" data-en="All">Alles</option>
                    </select></label>
                <button id="rlReloadBtn" class="ch-btn">🔄 <span class="de">Neu laden</span><span class="en">Reload</span></button>
                <a class="ch-btn" style="text-decoration:none;" href="/api/logs/download?file=Ruecklauf.csv">⬇ CSV</a>
                <span id="rlStatus" class="ch-status"></span>
            </div>
            <div class="ch-panel">
                <div class="ch-head"><span class="ch-title"><span class="de">Rücklauf pro Tag (Liter)</span><span class="en">Return per day (litres)</span></span></div>
                <canvas id="rlCanvas" class="ch-cv" style="height:200px;"></canvas>
            </div>
            <div class="ch-hoverbar"><span id="rlHover"></span></div>
            <div id="rlCards" class="ch-cards"></div>
            <div style="overflow-x:auto;margin-top:10px;"><table id="rlDays" class="ch-days"></table></div>
            <div style="overflow-x:auto;margin-top:10px;"><table id="rlLast" class="ch-days"></table></div>
            <div class="ch-hint">
                <span class="de">Das Füllstand-Modul meldet jeden Pumpzyklus per WLAN. Liter = vollständige automatische Zyklen × Volumen zwischen den Schwimmern (am Modul eingestellt). „Bis MIN“ = Pumpzeit bis der untere Schwimmer frei ist, „Abstand“ = Zeit seit dem vorigen Zyklus (kürzer = mehr Rücklauf).</span>
                <span class="en">The level module reports every pump cycle via WiFi. Litres = complete automatic cycles × volume between the floats (set on the module). "To MIN" = pumping time until the lower float is free, "Gap" = time since the previous cycle (shorter = more return flow).</span>
            </div>

            <script>
            (function () {
                'use strict';
                const FILE = 'Ruecklauf.csv', DAY = 86400000;
                const R = { rows: [], loaded: false, wired: false, bars: [] };
                const $ = id => document.getElementById(id);
                function L(de, en) { return (typeof currentLang !== 'undefined' && currentLang === 'en') ? en : de; }
                const p2 = n => String(n).padStart(2, '0');
                const fDate = t => { const d = new Date(t); return p2(d.getDate()) + '.' + p2(d.getMonth() + 1) + '.' + d.getFullYear(); };
                const fTime = t => { const d = new Date(t); return p2(d.getHours()) + ':' + p2(d.getMinutes()); };
                const dayKey = t => { const d = new Date(t); return d.getFullYear() + '-' + p2(d.getMonth() + 1) + '-' + p2(d.getDate()); };
                function fDur(s) {
                    s = Math.round(s);
                    if (s >= 3600) return Math.floor(s / 3600) + ' h ' + p2(Math.floor(s / 60) % 60) + ' min';
                    if (s >= 60) return Math.floor(s / 60) + ' min ' + p2(s % 60) + ' s';
                    return s + ' s';
                }
                const ART = { auto: ['auto', 'auto'], manuell: ['manuell', 'manual'], stopp: ['abgebrochen', 'stopped'], alarm: ['Alarm', 'alarm'] };
                const artTxt = a => ART[a] ? L(ART[a][0], ART[a][1]) : a;

                function parse(txt) {
                    const lines = txt.split(/\r?\n/), h = (lines[0] || '').split(',');
                    const di = h.indexOf('Datum'), zi = h.indexOf('Zeit'), du = h.indexOf('Dauer_s'), bm = h.indexOf('BisMIN_s'),
                          ai = h.indexOf('Art'), vi = h.indexOf('Volumen_ml');
                    if (di < 0 || zi < 0 || du < 0) return [];
                    const out = [];
                    for (let i = 1; i < lines.length; i++) {
                        if (!lines[i]) continue;
                        const c = lines[i].split(','), d = (c[di] || '').split('-').map(Number), z = (c[zi] || '').split(':').map(Number);
                        if (d.length < 3 || !d[0]) continue;
                        out.push({ t: new Date(d[0], d[1] - 1, d[2], z[0] || 0, z[1] || 0, z[2] || 0).getTime(),
                                   dur: +c[du] || 0, toMin: bm >= 0 ? (+c[bm] || 0) : 0, art: ai >= 0 ? c[ai] : 'auto',
                                   ml: vi >= 0 ? (+c[vi] || 0) : 0 });
                    }
                    out.sort((a, b) => a.t - b.t);
                    out.forEach((r, i) => { r.gap = i ? (r.t - out[i - 1].t) / 1000 : NaN; });
                    return out;
                }

                async function load() {
                    $('rlStatus').textContent = L('Lade …', 'Loading …');
                    try {
                        const r = await fetch('/api/logs/download?file=' + encodeURIComponent(FILE));
                        R.rows = r.ok ? parse(await r.text()) : [];
                        R.loaded = true;
                        render();
                    } catch (e) {
                        $('rlStatus').textContent = L('Fehler beim Laden: ', 'Load error: ') + e;
                    }
                }

                // Tage im Zeitraum (auch Tage ohne Zyklus, damit Luecken sichtbar sind)
                function days() {
                    const n = parseInt($('rlRange').value) || 0;
                    const map = {};
                    for (const r of R.rows) {
                        const k = dayKey(r.t);
                        const o = map[k] || (map[k] = { k, t: new Date(new Date(r.t).setHours(0, 0, 0, 0)).getTime(), n: 0, ml: 0, s: 0, toMin: 0, nAuto: 0, gS: 0, gN: 0 });
                        if (isFinite(r.gap)) { o.gS += r.gap; o.gN++; }
                        if (r.art === 'auto' || r.art === 'manuell') o.n++;
                        if (r.art === 'auto') { o.nAuto++; o.toMin += r.toMin; }
                        o.ml += r.ml; o.s += r.dur;
                    }
                    let first = R.rows.length ? new Date(R.rows[0].t).setHours(0, 0, 0, 0) : Date.now();
                    const today = new Date().setHours(0, 0, 0, 0);
                    if (n) first = Math.max(first, today - (n - 1) * DAY);
                    const out = [];
                    for (let t = first; t <= today; t = new Date(t + DAY * 1.5).setHours(0, 0, 0, 0)) {
                        const k = dayKey(t);
                        out.push(map[k] || { k, t, n: 0, ml: 0, s: 0, toMin: 0, nAuto: 0, gS: 0, gN: 0 });
                    }
                    return out;
                }

                function render() {
                    const ds = days();
                    const inRange = ds.length ? R.rows.filter(r => r.t >= ds[0].t) : [];
                    $('rlStatus').textContent = R.rows.length ? inRange.length + L(' von ', ' of ') + R.rows.length + L(' Zyklen', ' cycles')
                                                              : L('Noch keine Meldungen vom Füllstand-Modul.', 'No reports from the level module yet.');
                    drawChart(ds);
                    renderCards(ds);
                    renderDays(ds);
                    renderLast();
                }

                function drawChart(ds) {
                    const cv = $('rlCanvas'), dpr = window.devicePixelRatio || 1;
                    const W = cv.clientWidth || 600, H = cv.clientHeight || 200;
                    cv.width = W * dpr; cv.height = H * dpr;
                    const ctx = cv.getContext('2d');
                    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
                    ctx.clearRect(0, 0, W, H);
                    R.bars = [];
                    const PL = 40, PR = 10, PT = 10, PB = 24, pw = W - PL - PR, ph = H - PT - PB;
                    ctx.font = '11px sans-serif'; ctx.fillStyle = '#888';
                    if (!ds.length || !R.rows.length) { ctx.fillText(L('Keine Daten', 'No data'), PL, PT + ph / 2); return; }
                    const maxL = Math.max(1, ...ds.map(d => d.ml / 1000));
                    const yMax = Math.ceil(maxL * 1.15);
                    const Y = v => PT + ph - v / yMax * ph;
                    ctx.strokeStyle = '#eee'; ctx.lineWidth = 1; ctx.textAlign = 'right';
                    for (let i = 0; i <= 4; i++) {
                        const v = yMax * i / 4, y = Y(v);
                        ctx.beginPath(); ctx.moveTo(PL, y); ctx.lineTo(PL + pw, y); ctx.stroke();
                        ctx.fillText(v.toFixed(v < 10 ? 1 : 0), PL - 5, y + 4);
                    }
                    const bw = pw / ds.length, step = Math.max(1, Math.ceil(ds.length / Math.floor(pw / 45)));
                    ctx.textAlign = 'center';
                    ds.forEach((d, i) => {
                        const x = PL + i * bw, v = d.ml / 1000, y = Y(v);
                        ctx.fillStyle = '#2e86de';
                        if (v > 0) ctx.fillRect(x + bw * 0.15, y, bw * 0.7, PT + ph - y);
                        R.bars.push({ x0: x, x1: x + bw, d });
                        if (i % step === 0) {
                            const dt = new Date(d.t);
                            ctx.fillStyle = '#888';
                            ctx.fillText(p2(dt.getDate()) + '.' + p2(dt.getMonth() + 1) + '.', x + bw / 2, H - 6);
                        }
                    });
                }

                function card(title, main, sub, color) {
                    return '<div class="ch-card" style="border-left-color:' + (color || '#2e86de') + ';"><div class="ch-c-title">' + title +
                           '</div><div class="ch-c-main">' + main + '</div>' + (sub ? '<div class="ch-c-sub">' + sub + '</div>' : '') + '</div>';
                }

                function renderCards(ds) {
                    if (!R.rows.length) { $('rlCards').innerHTML = ''; return; }
                    const today = ds[ds.length - 1], yest = ds.length > 1 ? ds[ds.length - 2] : null;
                    // Mittel ueber die letzten 7 vollstaendigen Tage (ohne heute)
                    const full = ds.slice(0, -1).slice(-7);
                    const avg = full.length ? full.reduce((a, d) => a + d.ml, 0) / full.length / 1000 : NaN;
                    const last = R.rows[R.rows.length - 1];
                    $('rlCards').innerHTML =
                        card(L('Heute', 'Today'), (today.ml / 1000).toFixed(1) + ' <small>l</small>', today.n + L(' Zyklen', ' cycles')) +
                        (yest ? card(L('Gestern', 'Yesterday'), (yest.ml / 1000).toFixed(1) + ' <small>l</small>', yest.n + L(' Zyklen', ' cycles')) : '') +
                        (isFinite(avg) ? card(L('Ø pro Tag', 'Avg per day'), avg.toFixed(1) + ' <small>l</small>', L('letzte ', 'last ') + full.length + L(' Tage (ohne heute)', ' days (excl. today)')) : '') +
                        card(L('Letzter Zyklus', 'Last cycle'), fTime(last.t), fDate(last.t) + ' · ' + artTxt(last.art) +
                             (isFinite(last.gap) ? ' · ' + L('Abstand ', 'gap ') + fDur(last.gap) : ''),
                             last.art === 'alarm' ? '#e74c3c' : '#2e86de');
                }

                function renderDays(ds) {
                    const rows = ds.slice().reverse();
                    if (!R.rows.length) { $('rlDays').innerHTML = ''; return; }
                    $('rlDays').innerHTML = '<tr><th style="text-align:left;">' + L('Tag', 'Day') + '</th><th>' + L('Zyklen', 'Cycles') + '</th><th>' +
                        L('Liter', 'Litres') + '</th><th>' + L('Pumpzeit', 'Pump time') + '</th><th>' + L('Ø bis MIN', 'Avg to MIN') + '</th><th>' +
                        L('Ø Abstand', 'Avg gap') + '</th></tr>' +
                        rows.map(d => {
                            const gap = d.gN ? fDur(d.gS / d.gN) : '–';
                            return '<tr><td style="text-align:left;">' + fDate(d.t) + '</td><td>' + d.n + '</td><td><b>' + (d.ml / 1000).toFixed(1) +
                                   '</b></td><td>' + (d.s ? fDur(d.s) : '–') + '</td><td>' + (d.nAuto ? fDur(d.toMin / d.nAuto) : '–') + '</td><td>' + gap + '</td></tr>';
                        }).join('');
                }

                function renderLast() {
                    const last = R.rows.slice(-30).reverse();
                    if (!last.length) { $('rlLast').innerHTML = ''; return; }
                    $('rlLast').innerHTML = '<tr><th style="text-align:left;">' + L('Start', 'Start') + '</th><th>' + L('Abstand', 'Gap') + '</th><th>' +
                        L('Bis MIN', 'To MIN') + '</th><th>' + L('Dauer', 'Duration') + '</th><th>' + L('Liter', 'Litres') + '</th><th style="text-align:left;">' +
                        L('Art', 'Type') + '</th></tr>' +
                        last.map(r => '<tr><td style="text-align:left;">' + fDate(r.t) + ' ' + fTime(r.t) + '</td><td>' + (isFinite(r.gap) ? fDur(r.gap) : '–') +
                            '</td><td>' + fDur(r.toMin) + '</td><td>' + fDur(r.dur) + '</td><td>' + (r.ml ? (r.ml / 1000).toFixed(1) : '–') +
                            '</td><td style="text-align:left;' + (r.art === 'alarm' ? 'color:#e74c3c;' : '') + '">' + artTxt(r.art) + '</td></tr>').join('');
                }

                function wire() {
                    R.wired = true;
                    $('rlReloadBtn').addEventListener('click', load);
                    $('rlRange').addEventListener('change', () => { if (R.loaded) render(); });
                    const cv = $('rlCanvas');
                    cv.addEventListener('mousemove', ev => {
                        const b = cv.getBoundingClientRect(), mx = ev.clientX - b.left;
                        const hit = R.bars.find(p => mx >= p.x0 && mx < p.x1);
                        $('rlHover').textContent = hit ? fDate(hit.d.t) + ' · ' + (hit.d.ml / 1000).toFixed(1) + ' l · ' + hit.d.n + L(' Zyklen', ' cycles') : '';
                    });
                    cv.addEventListener('mouseleave', () => { $('rlHover').textContent = ''; });
                    window.addEventListener('resize', () => { if (R.loaded && $('chartsTab').classList.contains('active')) render(); });
                }

                const prevShow = window.chartsOnShow, prevLang = window.chartsOnLangChange;
                window.chartsOnShow = function () {
                    if (typeof prevShow === 'function') prevShow();
                    if (!R.wired) wire();
                    if (!R.loaded) load(); else render();
                };
                window.chartsOnLangChange = function () {
                    if (typeof prevLang === 'function') prevLang();
                    if (R.loaded) render();
                };
            })();
            </script>
        </div>
)rawliteral"
