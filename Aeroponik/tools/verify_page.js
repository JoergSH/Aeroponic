// Pruefroutine fuer src/page.h, gedacht als einziger fester Befehl (node tools/verify_page.js),
// damit die Freigabe in .claude/settings.local.json nicht bei jeder kleinen Aenderung des
// Pruefbefehls (Dateiname, Zeilenzahl, Inline-Skript) neu erteilt werden muss.
//
// Prueft: 1) JS-Syntax aller <script>-Bloecke (per vm, ohne Temp-Datei)
//         2) doppelte id="..." Attribute (bekannter, harmloser Fall: nodes_control_loading)

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const pagePath = path.join(__dirname, '..', 'src', 'page.h');
const html = fs.readFileSync(pagePath, 'utf8');

let ok = true;

// --- 1) JS-Syntax ---
const scriptBlocks = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]);
const combined = scriptBlocks.join('\n;\n');
try {
    new vm.Script(combined, { filename: 'page.h (script blocks)' });
    console.log(`JS-Syntax OK (${scriptBlocks.length} <script>-Block(e), ${combined.split('\n').length} Zeilen)`);
} catch (e) {
    ok = false;
    console.log('JS-SYNTAXFEHLER:', e.message);
}

// --- 2) doppelte IDs ---
const ids = [...html.matchAll(/id="([a-zA-Z0-9_]+)"/g)].map(m => m[1]);
const counts = {};
for (const id of ids) counts[id] = (counts[id] || 0) + 1;
const dupes = Object.entries(counts).filter(([, n]) => n > 1);
const KNOWN_BENIGN = new Set(['nodes_control_loading']);
const unexpectedDupes = dupes.filter(([id]) => !KNOWN_BENIGN.has(id));

if (dupes.length === 0) {
    console.log('Keine doppelten IDs.');
} else {
    for (const [id, n] of dupes) {
        console.log(`${KNOWN_BENIGN.has(id) ? '(bekannt, harmlos)' : 'UNERWARTET DOPPELT'}: id="${id}" x${n}`);
    }
}
if (unexpectedDupes.length > 0) ok = false;

process.exit(ok ? 0 : 1);
