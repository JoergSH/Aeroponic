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
                        <option class="i18n-opt" value="rate" data-de="Impulse pro Sekunde" data-en="Pulses per second">Impulse pro Sekunde</option>
                        <option class="i18n-opt" value="cyc" data-de="Impulse je Zyklus" data-en="Pulses per cycle">Impulse je Zyklus</option>
                    </select></label>
            </div>
            <div id="chNozzleWrap"></div>
            <div id="chNozCards" class="ch-cards"></div>
            <div class="ch-hint">
                <span class="de">Punkte = einzelne Gießzyklen, Linie = gleitender Median über 9 Zyklen, roter Ring = Ausreißer (&gt;15 % vom Median). „Impulse pro Sekunde“ bleibt vergleichbar, auch wenn die Öffnungszeit geändert wurde. Die Bewertung bezieht sich auf den Ausschnitt oben: Trend = letztes gegen erstes Drittel der Zyklen, Streuung = Standardabweichung in %.</span>
                <span class="en">Dots = single watering cycles, line = rolling median over 9 cycles, red ring = outlier (&gt;15 % from the median). "Pulses per second" stays comparable even if the open time was changed. The rating refers to the window above: trend = last vs. first third of cycles, spread = standard deviation in %.</span>
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
                    nozOn: [true, true, true, true], nozMode: 'rate',
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
                        if (p.nozMode === 'cyc' || p.nozMode === 'rate') S.nozMode = p.nozMode;
                        if (p.range === 7) S.range = 7;
                    } catch (e) {}
                }
                function savePrefs() {
                    try { localStorage.setItem(PREF_KEY, JSON.stringify({ on: S.on, nozOn: S.nozOn, nozMode: S.nozMode, range: S.range })); } catch (e) {}
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

                const nozVal = c => S.nozMode === 'rate' ? c.r : c.p;
                const nozMed = c => S.nozMode === 'rate' ? c.mr : c.mp;
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
                    const dec = S.nozMode === 'rate' ? 1 : 0;
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

                    p.leg.innerHTML = '<span class="ch-li" style="color:#888;">' + (S.nozMode === 'rate' ? L('Impulse/s', 'pulses/s') : L('Impulse je Zyklus', 'pulses per cycle')) + '</span>' +
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
                    const dec = S.nozMode === 'rate' ? 1 : 0;
                    const unit = S.nozMode === 'rate' ? L('Imp/s', 'p/s') : L('Imp/Zyklus', 'p/cycle');
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
                        else if (trend <= -10) { amp = '🟠'; txt = L('fällt – Düse/Filter prüfen', 'dropping – check nozzle/filter'); }
                        else if (trend >= 10) { amp = '🟠'; txt = L('steigt – Leck oder lose Düse?', 'rising – leak or loose nozzle?'); }
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
        </div>
)rawliteral"
