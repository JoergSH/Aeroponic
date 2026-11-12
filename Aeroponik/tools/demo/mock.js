// mock.js - Demo-Modus fuer die Aeroponik-Weboberflaeche (GitHub Pages)
// Ersetzt window.fetch fuer alle /api/...-Aufrufe durch eine Simulation im Browser.
// Alle Werte sind erfunden; "Speichern" aendert nur den Zustand in diesem Browser-Tab.
(function () {
    'use strict';

    // ---------------- Hilfsfunktionen ----------------
    const pad = n => String(n).padStart(2, '0');
    const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
    const r1 = v => Math.round(v * 10) / 10;
    const r2 = v => Math.round(v * 100) / 100;

    // Deterministisches, glattes Rauschen: gleiche Zeit -> gleicher Wert (Diagramm und Live-Wert passen zusammen)
    function hash(n) { const x = Math.sin(n * 127.1 + 311.7) * 43758.5453; return x - Math.floor(x); }
    function smooth(x, seed) {
        const i = Math.floor(x), f = x - i, u = f * f * (3 - 2 * f);
        return (hash(i + seed * 1000) * (1 - u) + hash(i + 1 + seed * 1000) * u) * 2 - 1;   // -1..1
    }
    function noise(tMin, seed, periodMin) {     // mehrere Oktaven
        return smooth(tMin / periodMin, seed) * 0.6 + smooth(tMin / (periodMin / 4), seed + 7) * 0.3 +
               smooth(tMin / (periodMin / 16), seed + 13) * 0.1;
    }
    function svp(t) { return 0.6108 * Math.exp(17.27 * t / (t + 237.3)); }   // kPa
    function vpd(t, rh, off) { return svp(t + (off || 0)) - svp(t) * rh / 100; }
    function absHum(t, rh) { return 216.7 * (rh / 100 * 6.112 * Math.exp(17.62 * t / (243.12 + t))) / (273.15 + t); }

    const BOOT = Date.now() - (3 * 86400 + 5 * 3600 + 17 * 60) * 1000;   // "laeuft seit 3 d 5 h"

    // ---------------- Konfiguration (veraenderbar per "Speichern") ----------------
    const cfg = {
        scheduler: { enabled: true, manual_percent: 100, dawn_start: 360, dawn_end: 390, dusk_start: 1410, dusk_end: 1439, num_ssr: 4 },
        vpd: { enabled: true, phase: 2, day_min: 100, day_max: 140, night_min: 80, night_max: 120, leaf_offset: -15,
               use_humid: true, use_fan: true, temp_max: 25, hum_max: 70, alarm_min: 30, temp_min: 20 },
        fan: { mode: 2, manual_percent: 30, min_speed: 15, max_speed: 80, hum_min: 55, hum_max: 70,
               temp_min: 24, temp_max: 27, enabled: true, temp_mode: 0, temp_cap_diff: 0 },
        humid: { enabled: true, target_node_id: 21, target_relay_bit: 1, humid_min: 55, humid_max: 65 },
        co2: { enabled: false, target_node_id: 21, target_relay_bit: 2, co2_min: 800, co2_max: 1100 },
        dwc: { enabled: false, target_node_id: 21, target_relay_bit: 3, on_min: 15, off_min: 15 },
        raumlicht: { target_node_id: 22, target_relay_bit: 0, on: false },
        gehaeuse: { enabled: true, temp_min: 35, temp_max: 45 },
        lueftmodul: { enabled: true, gleiche_werte: true, percent: [50, 50, 50, 50], monitor_enabled: true, monitor_tol_pct: 25 },
        umwaelz: { aktiv: true, laufzeit_s: 120, pausenzeit_min: 30 },
        drucksensor: { enabled: false },
        output: { fan_output: 0, light_output: 0 },
        tank: { hoehe_mm: 400, radius_unten_mm: 170, radius_oben_mm: 190, leer_abstand_mm: 430, min_mm: 60, max_mm: 360,
                offset_mm: 0, sensor_type: 0, druck_null_raw: 0, druck_counts_pro_mm: 0 },
        flowmon: { enabled: true, toleranz_prozent: 30, erwartet_impulse_s: [41.5, 40.8, 42.1, 39.7] },
        ventile: { druckaufbau_s: 8, gleiche_zeiten: true,
                   behaelter: [0, 1, 2, 3].map(() => ({ aktiv: true, oeffnungszeit_s: 5, pausenzeit_min: 5,
                                                        oeffnungszeit_s_dunkel: 4, pausenzeit_min_dunkel: 10 })) },
        notify: { global_enabled: true, phone: '', has_apikey: false, netzausfall_enabled: true, sensorausfall_enabled: true,
                  feuchte_enabled: true, feuchte_min: 40, feuchte_max: 80, temperatur_enabled: true, temperatur_min: 16,
                  temperatur_max: 30, tank_enabled: true, tank_min_prozent: 20, lueftmodul_enabled: true,
                  verbindung_enabled: true, whatsapp_enabled: false, telegram_enabled: true, telegram_chat_id: '123456789',
                  has_telegram_token: true, email_enabled: false, smtp_host: '', smtp_port: 465, smtp_user: '', email_to: '',
                  has_smtp_pass: false },
        wifi: { ssid: 'Demo-WLAN', has_password: true },
    };
    const nodes = [
        { id: 21, type: 3, relay_mask: 0 },               // Steckdosenleiste (Befeuchter, CO2, DWC)
        { id: 22, type: 3, relay_mask: 0 },               // Steckdose Raumlicht
        { id: 31, type: 2 },                               // Multisensor per ESP-NOW (Ersatz/zweiter Sensor)
    ];
    let rs485logOn = false, testState = 0, testMode = false, dose = null;

    // ---------------- Licht ----------------
    function lightLevel(min) {          // 0..1 laut Zeitplan
        const s = cfg.scheduler;
        if (!s.enabled) return s.manual_percent / 100;
        const inWin = (a, b, m) => a <= b ? (m >= a && m < b) : (m >= a || m < b);
        const span = (a, b) => ((b - a) + 1440) % 1440 || 1;
        if (inWin(s.dawn_start, s.dawn_end, min)) return ((min - s.dawn_start + 1440) % 1440) / span(s.dawn_start, s.dawn_end);
        if (inWin(s.dawn_end, s.dusk_start, min)) return 1;
        if (inWin(s.dusk_start, s.dusk_end, min)) return 1 - ((min - s.dusk_start + 1440) % 1440) / span(s.dusk_start, s.dusk_end);
        return 0;
    }
    function lightMask(level) {
        const n = cfg.scheduler.num_ssr || 4, on = Math.ceil(level * n - 1e-6);
        return (1 << on) - 1;
    }

    // ---------------- Klima-Simulation ----------------
    // Liefert einen vollstaendigen Zustand fuer einen Zeitpunkt (ms). Wird fuer Live-Werte und Logs genutzt.
    function sim(ts) {
        const d = new Date(ts), min = d.getHours() * 60 + d.getMinutes() + d.getSeconds() / 60;
        const tMin = ts / 60000;
        const L = lightLevel(Math.floor(min)), lightOn = L > 0.02;
        // Lampe heizt verzoegert auf: gleitender Anteil ueber die letzten ~40 min
        let heat = 0; for (let k = 0; k < 8; k++) heat += lightLevel(Math.floor((min - k * 5 + 1440) % 1440));
        heat /= 8;
        const roomT = 20.5 + 1.2 * Math.sin((min - 900) / 1440 * 2 * Math.PI) + 0.4 * noise(tMin, 1, 240);
        const roomH = 52 + 6 * noise(tMin, 2, 600);
        let tentT = roomT + 0.6 + 4.6 * heat + 0.35 * noise(tMin, 3, 90);
        const vb = vpdBand(lightOn);
        const off = lightOn ? cfg.vpd.leaf_offset / 10 : 0;
        // Feuchte: Befeuchter haelt VPD grob in der Bandmitte, mit Pulsmuster. Ziel und Blatt-Offset
        // gleiten ueber ~30 min (wie die Rampe der echten Regelung), sonst springt die Feuchte bei Licht an/aus.
        let vMid = 0, offS = 0;
        for (let k = 0; k < 7; k++) {
            const on = lightLevel(Math.floor((min - k * 5 + 1440) % 1440)) > 0.02, b = vpdBand(on);
            vMid += (b[0] + b[1]) / 2 / 7; offS += (on ? cfg.vpd.leaf_offset / 10 : 0) / 7;
        }
        const vTarget = vMid + 0.08 * noise(tMin, 4, 120);
        let tentH = clamp(100 * (svp(tentT + offS) - vTarget) / svp(tentT), 35, 85);
        const humidOn = cfg.vpd.enabled && cfg.vpd.use_humid ? ((Math.floor(ts / 1000) % 180) < 60 && tentH > 50 && lightOn)
                                                             : tentH < cfg.humid.humid_min;
        tentH = r1(tentH + (humidOn ? 0.6 : -0.3));
        tentT = r1(tentT);
        // Luefter: steigt mit Temperatur oberhalb temp_max-1
        const tm = cfg.vpd.temp_max, f = cfg.fan;
        let fanPct = f.min_speed + (lightOn ? 10 : 0);
        if (tentT > tm - 1) fanPct = Math.max(fanPct, f.min_speed + (f.max_speed - f.min_speed) * clamp((tentT - (tm - 1)) / 3, 0, 1));
        fanPct = Math.round(clamp(fanPct, f.min_speed, f.max_speed));
        const co2 = Math.round(lightOn ? 520 + 80 * noise(tMin, 5, 60) : 690 + 60 * noise(tMin, 5, 60));
        const ppfd = r1(L * (640 + 25 * noise(tMin, 6, 30)));
        // Tank: sinkt ueber den Tag, wird alle 3 Tage nachgefuellt
        const cycle = (tMin / 1440 + 0.4) % 3;
        const fuell = Math.round(clamp(92 - cycle * 24 + 0.6 * noise(tMin, 8, 30), 5, 100));
        return {
            ts, min, L, lightOn, mask: lightMask(L), roomT: r1(roomT), roomH: r1(roomH), tentT, tentH, co2, ppfd,
            vpdAir: r2(vpd(tentT, tentH, 0)), vpdLeaf: r2(vpd(tentT, tentH, off)), band: vb,
            fanPct, fanRpm: Math.round(fanPct * 23.5 + 40 * noise(tMin, 9, 10)), humidOn,
            tempVorrat: r1(19.4 + 0.8 * heat + 0.2 * noise(tMin, 10, 300)),
            tempPflanze: r1(21.0 + 2.4 * heat + 0.3 * noise(tMin, 11, 200)),
            tempInnen: r1(33 + 6 * heat + noise(tMin, 12, 60)),
            fuell, wasserMM: Math.round(cfg.tank.min_mm + (cfg.tank.max_mm - cfg.tank.min_mm) * fuell / 100),
            spektrum: [0.08, 0.32, 0.45, 0.52, 0.70, 0.86, 1.0, 0.62].map((v, i) => Math.round(v * L * (3400 + 80 * noise(tMin, 20 + i, 15)))),
        };
    }
    function vpdBand(lightOn) {
        const v = cfg.vpd, T = [[40, 80, 30, 60], [80, 120, 60, 100], [100, 140, 80, 120], [120, 160, 100, 140]];
        const p = v.phase < 4 ? T[v.phase] : [v.day_min, v.day_max, v.night_min, v.night_max];
        return lightOn ? [p[0] / 100, p[1] / 100] : [p[2] / 100, p[3] / 100];
    }

    // Ventilzyklus: Behaelter nacheinander, Zustand aus der Uhrzeit abgeleitet
    function ventilState(ts, lightOn) {
        const v = cfg.ventile, sec = Math.floor(ts / 1000);
        return v.behaelter.map((b, i) => {
            const open = lightOn ? b.oeffnungszeit_s : b.oeffnungszeit_s_dunkel;
            const per = (lightOn ? b.pausenzeit_min : b.pausenzeit_min_dunkel) * 60 + open + v.druckaufbau_s;
            const ph = (sec + i * Math.floor(per / 4)) % per;
            let z = 'Pause', timer = ph - open - v.druckaufbau_s;
            if (ph < v.druckaufbau_s) { z = 'Druckaufbau'; timer = ph; }
            else if (ph < v.druckaufbau_s + open) { z = 'Aktiv'; timer = ph - v.druckaufbau_s; }
            return { zustand: b.aktiv ? z : 'Pause', timer_s: b.aktiv ? timer : 0 };
        });
    }

    // ---------------- Log-Dateien (CSV wie storage.cpp) ----------------
    const CSV_HEAD = 'Zeit,TempVorrat_C,TempPflanze_C,Druck_bar,Fuellstand_%,Volumen_L,' +
        'RaumTemp_C,RaumFeuchte_%,ZeltTemp_C,ZeltFeuchte_%,ZeltCO2_ppm,ZeltVPD_kPa,ZeltPPFD_umol,' +
        'LichtMaske,LuefterLeistung_%,LuefterRPM,CO2Ausgang,HumidAusgang,' +
        'ImpulseBehaelter1,DauerBehaelter1_s,ImpulseBehaelter2,DauerBehaelter2_s,' +
        'ImpulseBehaelter3,DauerBehaelter3_s,ImpulseBehaelter4,DauerBehaelter4_s,Laufzeit_min,NeustartGrund,Lichtx4Starts,Lichtx4Grund,Netzaussetzer,NetzaussetzerMax_ms,' +
        'CO2Steckdose,HumidSteckdose,WLAN_dBm,WLAN_Abbrueche,EspNow_online,HeapMin_kB,' +
        'MS2_Abfragen,MS2_Timeouts,VPD_Blatt_kPa,VPD_Soll_min,VPD_Soll_max';
    const LOG_DAYS = 14;
    function dayName(d) { return d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()); }
    function logFiles() {
        const out = [], today = new Date(); today.setHours(0, 0, 0, 0);
        for (let k = LOG_DAYS - 1; k >= 0; k--) {
            const d = new Date(today.getTime() - k * 86400000);
            out.push({ name: '/' + dayName(d) + '.csv', size: k === 0 ? Math.round(410 * (Date.now() - d) / 60000) : 590000 });
        }
        return out;
    }
    const csvCache = {};
    function csvFor(name) {
        const m = /(\d{4})-(\d{2})-(\d{2})/.exec(name);
        if (!m) return null;
        const start = new Date(+m[1], +m[2] - 1, +m[3]).getTime();
        const end = Math.min(start + 86400000, Date.now());
        if (end <= start) return null;
        const isToday = start + 86400000 > Date.now();
        if (!isToday && csvCache[name]) return csvCache[name];
        const rows = [CSV_HEAD];
        for (let ts = start; ts < end; ts += 60000) {
            const s = sim(ts), d = new Date(ts), sec0 = Math.floor(ts / 1000);
            // Ein Eintrag je Behaelter, wenn in dieser Minute ein Giesszyklus begann (Takt wie ventilState)
            const fl = cfg.ventile.behaelter.map((b, i) => {
                const open = s.lightOn ? b.oeffnungszeit_s : b.oeffnungszeit_s_dunkel;
                const per = (s.lightOn ? b.pausenzeit_min : b.pausenzeit_min_dunkel) * 60 + open + cfg.ventile.druckaufbau_s;
                const off = i * Math.floor(per / 4), k = sec0 + ((per - (sec0 + off) % per) % per);
                if (!b.aktiv || k >= sec0 + 60) return ['', ''];
                const ips = cfg.flowmon.erwartet_impulse_s[i] * (1 + 0.03 * (hash(k + i) - 0.5));
                return [Math.round(ips * open), open.toFixed(1)];
            });
            rows.push([pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds()),
                s.tempVorrat, s.tempPflanze, '0.00', s.fuell, r1(s.fuell * 0.32),
                s.roomT, s.roomH, s.tentT, s.tentH, s.co2, s.vpdAir.toFixed(2), s.ppfd,
                s.mask, s.fanPct, s.fanRpm, 0, s.humidOn ? 1 : 0,
                ...fl.flat(), Math.floor((ts - BOOT) / 60000), '', 1, '', 0, 0,
                '', s.humidOn ? 1 : 0, -58 - Math.round(4 * hash(ts)), 0, '3/3', 142,
                60, 0, s.vpdLeaf.toFixed(2), s.band[0].toFixed(2), s.band[1].toFixed(2)].join(','));
        }
        const txt = rows.join('\n') + '\n';
        if (!isToday) csvCache[name] = txt;
        return txt;
    }

    // ---------------- GET-Antworten ----------------
    function fmtDate(d) {
        return pad(d.getDate()) + '.' + pad(d.getMonth() + 1) + '.' + d.getFullYear() + ' ' +
               pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds());
    }
    function relay(nodeId, bit) { const n = nodes.find(x => x.id === nodeId); return n ? ((n.relay_mask >> bit) & 1) : -1; }
    function updateRelays(s) {
        const n21 = nodes.find(n => n.id === 21);
        const bit = cfg.humid.target_relay_bit;
        if (cfg.humid.enabled) n21.relay_mask = s.humidOn ? (n21.relay_mask | (1 << bit)) : (n21.relay_mask & ~(1 << bit));
        const n22 = nodes.find(n => n.id === 22);
        n22.relay_mask = cfg.raumlicht.on ? 1 : 0;
    }

    const GET = {
        '/api/data': s => ({
            tempVorrat: s.tempVorrat, tempPflanze: s.tempPflanze, aht21Ok: true, aht21Temp: s.roomT, aht21Hum: s.roomH,
            ens160Ok: false, netzOk: true, lastResetReason: 'Power-on', druckBar: 0, druckSensorEnabled: cfg.drucksensor.enabled,
            sensorTyp: cfg.tank.sensor_type, distanzMM: cfg.tank.leer_abstand_mm - s.wasserMM, wasserHoeheMM: s.wasserMM,
            fuellstandProzent: s.fuell, volumenLiter: r1(s.fuell * 0.32), datetime: fmtDate(new Date()),
            fwVersion: (window.DEMO_FW || '1.21.5') + ' (Demo)', freeHeap: 168432, uptime: Math.floor((Date.now() - BOOT) / 1000),
            chipModel: 'ESP32-S3', chipRevision: 0, flashSize: 16777216, sketchSize: 1573104,
            wifiMode: 'STA', ipAddress: '192.0.2.10', ethIP: '192.0.2.11', sdOk: true,
        }),
        '/api/espnow': s => ({
            count: nodes.length,
            nodes: nodes.map(n => {
                const o = { id: n.id, type: n.type, online: true, since_s: 2, age_s: 2 };
                if (n.type === 3) o.relay_mask = n.relay_mask;
                if (n.type === 2) { o.temperature = r2(s.tentT + 0.2); o.humidity = r2(s.tentH - 0.8); o.co2 = s.co2 + 12;
                                    o.ppfd = r1(s.ppfd * 0.97); o.light_sensor_ok = true; o.spektrum = s.spektrum; }
                return o;
            }),
            discovered: [{ mac: '02:00:00:12:34:56', id: 23, type: 3, age_s: 14, id_conflict: false }],
        }),
        '/api/rs485': s => ({
            ms2: { addr: 2, online: true, age_s: 1, co2: s.co2, temp: s.tentT, hum: s.tentH, ppfd: s.ppfd, status: 3,
                   co2_sensor_ok: true, co2_valid: true, co2_estimate: false, light_sensor_ok: true,
                   channels: s.spektrum.map(v => Math.min(255, Math.round(v / 14))) },
            licht: { online: true, mask: s.mask },
        }),
        '/api/ventile': s => {
            const vs = ventilState(Date.now(), s.lightOn);
            return {
                behaelter: cfg.ventile.behaelter.map((b, i) => Object.assign({}, b, vs[i])),
                druckaufbau_s: cfg.ventile.druckaufbau_s, gleiche_zeiten: cfg.ventile.gleiche_zeiten,
                pumpe_aktiv: vs.some(v => v.zustand !== 'Pause'), umwaelzpumpe_aktiv: umwaelzOn(), licht_an: s.lightOn,
            };
        },
        '/api/output/test': () => ({ state: testState, testmode: testMode, pending: false,
            names: ['Ventil 1 (Beh. 1)', 'Ventil 2 (Beh. 2)', 'Ventil 3 (Beh. 3)', 'Ventil 4 (Beh. 4)', 'Umwälzpumpe', 'Pumpe'] }),
        '/api/flowmon': () => ({ enabled: cfg.flowmon.enabled, toleranz_prozent: cfg.flowmon.toleranz_prozent,
            erwartet_impulse_s: cfg.flowmon.erwartet_impulse_s,
            last_cycle_pulses: cfg.flowmon.erwartet_impulse_s.map((e, i) => Math.round(e * 5 + 4 * (hash(i) - 0.5))),
            last_cycle_s: [5.0, 5.0, 5.0, 5.0] }),
        '/api/logs': () => ({ files: logFiles() }),
        '/api/ventile/dose/status': () => {
            if (!dose) return { active: false };
            const rest = Math.ceil((dose.end - Date.now()) / 1000);
            if (rest <= 0) { dose = null; return { active: false }; }
            return { active: true, index: dose.index, waiting: false, remaining_s: rest };
        },
        '/api/umwaelz': () => Object.assign({}, cfg.umwaelz, { laeuft: umwaelzOn() }),
        '/api/scheduler': s => Object.assign({}, cfg.scheduler, { now_min: Math.floor(s.min),
            cur_mask: cfg.scheduler.enabled ? s.mask : 0, cur_ssr_count: cfg.scheduler.enabled ? bits(s.mask) : 0 }),
        '/api/lichtkal': () => ({ state: 'idle', schritt: 10, start_error: '', apply: true,
            saved: { ts: Math.floor(Date.now() / 1000) - 5 * 86400,
                     ppfd: [0, 38.2, 96.5, 161.0, 228.4, 297.3, 365.8, 431.2, 497.6, 561.9, 624.3] } }),
        '/api/co2': s => Object.assign({}, cfg.co2, { output_on: false, node_state: relay(cfg.co2.target_node_id, cfg.co2.target_relay_bit),
            co2_online: true, co2_now: s.co2, co2_geschaetzt: false }),
        '/api/humid': s => Object.assign({}, cfg.humid, { output_on: s.humidOn,
            node_state: relay(cfg.humid.target_node_id, cfg.humid.target_relay_bit), humid_online: true, humid_now: s.tentH,
            vpd: cfg.vpd.enabled && cfg.vpd.use_humid, pulse_pause: cfg.vpd.enabled && cfg.vpd.use_humid && !s.humidOn && s.lightOn,
            vpd_min: s.band[0], vpd_max: s.band[1] }),
        '/api/vpd': s => {
            const v = cfg.vpd, lo = s.band[0], hi = s.band[1];
            let reasons = s.vpdLeaf < lo ? 0x04 : s.vpdLeaf > hi ? 0x08 : 0x02;
            if (s.tentT > v.temp_max - 1) reasons |= 0x10;
            return Object.assign({}, v, {
                phases: [[40, 80, 30, 60], [80, 120, 60, 100], [100, 140, 80, 120], [120, 160, 100, 140], [v.day_min, v.day_max, v.night_min, v.night_max]],
                valid: true, light_on: s.lightOn, reasons, humid_want: s.humidOn, humid_demand: s.humidOn, fan_pct: s.fanPct,
                humid_dev: cfg.humid.enabled, fan_dev: true, vpd_air: s.vpdAir, vpd_leaf: s.vpdLeaf, soll_min: lo, soll_max: hi,
                ah_tent: r1(absHum(s.tentT, s.tentH)), ah_room: r1(absHum(s.roomT, s.roomH)),
            });
        },
        '/api/dwc': () => Object.assign({}, cfg.dwc, { output_on: false }),
        '/api/raumlicht': () => ({ target_node_id: cfg.raumlicht.target_node_id, target_relay_bit: cfg.raumlicht.target_relay_bit,
            online: true, output_on: cfg.raumlicht.on }),
        '/api/drucksensor': () => ({ enabled: cfg.drucksensor.enabled }),
        '/api/gehaeuse': s => Object.assign({}, cfg.gehaeuse, { sensor_available: true, temp_innen: s.tempInnen,
            fan_available: true, on: s.tempInnen > cfg.gehaeuse.temp_min + 2 }),
        '/api/lueftmodul': () => Object.assign({}, cfg.lueftmodul, { online: true,
            rpm: cfg.lueftmodul.percent.map((p, i) => Math.round(p * 40 + [70, -60, 70, -150][i])) }),
        '/api/notify': () => Object.assign({}, cfg.notify, { ms2_online: true }),
        '/api/fan': s => Object.assign({}, cfg.fan, { vpd: cfg.vpd.enabled && cfg.vpd.use_fan, room_ok: true, room_temp: s.roomT,
            online: true, percent: s.fanPct, rpm: s.fanRpm, target: s.fanPct, pulsed: false }),
        '/api/output': () => Object.assign({}, cfg.output, { analog_online: true, analog_ch1_raw: 1228, analog_ch2_raw: 0, gp8403_online: false }),
        '/api/wifi': () => ({ ssid: cfg.wifi.ssid, has_password: true, ap_active: false, connected: true, ip: '192.0.2.10',
            ap_ssid: 'Aeroponik-Setup', ap_ip: '192.168.4.1' }),
        '/api/tank': () => Object.assign({}, cfg.tank),
        '/api/rs485log': () => ({ enabled: rs485logOn, file: rs485logOn ? '/rs485_demo.txt' : '' }),
        '/api/rs485/ota/status': () => ({ state: 'idle', target: 'multimodul', busy: false, sent: 0, total: 0 }),
        '/api/ds18b20/scan': s => ({ count: 3, devices: [
            { addr: '28FF4A1B6E1A04C2', temp: s.tempVorrat, role: 'vorrat' },
            { addr: '28FF9C3D701603A7', temp: s.tempPflanze, role: 'pflanze' },
            { addr: '28FF02E8621704F1', temp: s.tempInnen, role: 'innen' }] }),
    };
    function bits(m) { let c = 0; while (m) { c += m & 1; m >>= 1; } return c; }
    function umwaelzOn() {
        const u = cfg.umwaelz; if (!u.aktiv) return false;
        const per = u.laufzeit_s + u.pausenzeit_min * 60;
        return (Math.floor(Date.now() / 1000) % per) < u.laufzeit_s;
    }

    // ---------------- POST-Handler ----------------
    const SAVE = { '/api/scheduler/save': 'scheduler', '/api/vpd/save': 'vpd', '/api/fan/save': 'fan', '/api/humid/save': 'humid',
        '/api/co2/save': 'co2', '/api/dwc/save': 'dwc', '/api/raumlicht/save': 'raumlicht', '/api/gehaeuse/save': 'gehaeuse',
        '/api/lueftmodul/save': 'lueftmodul', '/api/umwaelz/save': 'umwaelz', '/api/drucksensor/save': 'drucksensor',
        '/api/output/save': 'output', '/api/tank/config': 'tank', '/api/flowmon/save': 'flowmon', '/api/notify/save': 'notify',
        '/api/wifi/save': 'wifi' };
    const DEMO_DENIED = { success: false, ok: false,
        error: 'Demo: Diese Funktion braucht die echte Hardware. / Demo: this function needs the real hardware.' };

    function post(path, body) {
        let b = {};
        try { b = typeof body === 'string' && body ? JSON.parse(body) : {}; } catch (e) { b = {}; }
        if (SAVE[path]) {
            const t = cfg[SAVE[path]];
            for (const k of Object.keys(b)) if (k in t || SAVE[path] === 'notify') t[k] = b[k];
            return { success: true };
        }
        switch (path) {
            case '/api/ventile/config':
                for (const k of ['druckaufbau_s', 'gleiche_zeiten']) if (k in b) cfg.ventile[k] = b[k];
                if (Array.isArray(b.behaelter)) b.behaelter.forEach((x, i) => Object.assign(cfg.ventile.behaelter[i], x));
                return { success: true };
            case '/api/espnow/relay': { const n = nodes.find(x => x.id === b.node_id); if (n) n.relay_mask = b.mask; return { success: true }; }
            case '/api/raumlicht/toggle': cfg.raumlicht.on = !cfg.raumlicht.on; return { success: true };
            case '/api/ventile/dose/start': dose = { index: b.index || 0, end: Date.now() + 1000 * ((b.sekunden || b.seconds || 5) + 8) };
                return { success: true };
            case '/api/output/test':
                if ('testmode' in b) testMode = !!b.testmode;
                if ('index' in b) testState = b.on ? (testState | (1 << b.index)) : (testState & ~(1 << b.index));
                if (!testMode) testState = 0;
                return { success: true };
            case '/api/rs485log': rs485logOn = !!b.enabled; return { success: true, enabled: rs485logOn, file: rs485logOn ? '/rs485_demo.txt' : '' };
            case '/api/espnow/pwm': case '/api/espnow/pair': case '/api/espnow/unpair': case '/api/logs/delete':
            case '/api/notify/test': case '/api/ds18b20/assign':
                return { success: true };
            default:
                return DEMO_DENIED;   // OTA, Neustart, Lichtkalibrierung, Scan ...
        }
    }

    // ---------------- fetch ersetzen ----------------
    const realFetch = window.fetch.bind(window);
    function respond(obj, status) {
        const isText = typeof obj === 'string';
        return Promise.resolve(new Response(isText ? obj : JSON.stringify(obj),
            { status: status || 200, headers: { 'Content-Type': isText ? 'text/csv' : 'application/json' } }));
    }
    window.fetch = function (input, init) {
        const url = new URL(typeof input === 'string' ? input : input.url, location.href);
        const path = url.pathname.replace(/^.*?(\/api\/)/, '/api/');
        if (!/^\/api\//.test(path)) return realFetch(input, init);
        const method = ((init && init.method) || 'GET').toUpperCase();
        const delay = 40 + Math.random() * 80;
        return new Promise(res => setTimeout(res, delay)).then(() => {
            if (path === '/api/logs/download') {
                const txt = csvFor(url.searchParams.get('file') || '');
                return txt ? respond(txt) : respond('not found', 404);
            }
            if (method === 'POST') return respond(post(path, init && init.body));
            const s = sim(Date.now());
            updateRelays(s);
            const h = GET[path];
            return h ? respond(h(s)) : respond({ success: false, error: 'Demo: unbekannter Endpunkt' }, 404);
        });
    };

    // Log-Download per Link (window.location.href = '/api/logs/download?file=...') als Datei ausliefern
    window.demoDownload = function (file) {
        const txt = csvFor(file); if (!txt) return;
        const a = document.createElement('a');
        a.href = URL.createObjectURL(new Blob([txt], { type: 'text/csv' }));
        a.download = String(file).replace(/^\//, '');
        document.body.appendChild(a); a.click(); a.remove();
    };
})();
