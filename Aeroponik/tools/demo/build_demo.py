#!/usr/bin/env python3
"""Baut die Demo der Weboberflaeche fuer GitHub Pages.

Setzt die Seite genau wie die Firmware aus src/page.h + page_*.h zusammen und haengt
mock.js davor, das alle /api/...-Aufrufe im Browser simuliert.

Aufruf (aus beliebigem Ordner):  python Aeroponik/tools/demo/build_demo.py
Ergebnis:                        <Repo>/docs/demo/index.html
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE.parent.parent / "src"            # Aeroponik/src
REPO = HERE.parent.parent.parent            # MasterAeroponik (Git-Wurzel)
OUT = REPO / "docs" / "demo" / "index.html"

TOKEN = re.compile(r'R"rawliteral\((.*?)\)rawliteral"|#include\s+"([^"]+)"', re.S)


def assemble(path: Path) -> str:
    text = path.read_text(encoding="utf-8")
    parts = []
    for m in TOKEN.finditer(text):
        if m.group(1) is not None:
            parts.append(m.group(1))
        else:
            parts.append(assemble(path.parent / m.group(2)))
    return "".join(parts)


def replace_once(html: str, old: str, new: str, what: str) -> str:
    if old not in html:
        sys.exit(f"build_demo: Stelle fuer '{what}' nicht gefunden -- page*.h geaendert? Skript anpassen.")
    return html.replace(old, new, 1)


def main():
    html = assemble(SRC / "page.h")
    fw = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', (SRC / "config.h").read_text(encoding="utf-8"))
    fw = fw.group(1) if fw else "?"
    mock = (HERE / "mock.js").read_text(encoding="utf-8")

    head = ("<head>\n    <script>window.DEMO_FW = %r;</script>\n    <script>\n%s\n    </script>\n"
            "    <meta name=\"robots\" content=\"noindex\">\n" % (fw, mock))
    html = replace_once(html, "<head>", head, "<head>")
    html = replace_once(html, "<title>Aeroponik Steuerung</title>", "<title>Aeroponik Steuerung – Demo</title>", "title")

    banner = (
        '<body>\n<div id="demoBanner" style="position:sticky;top:0;z-index:9999;margin:-20px -20px 16px;padding:8px 14px;'
        'background:#fff3cd;border-bottom:2px solid #f0ad4e;color:#664d03;font-size:0.9em;text-align:center;">'
        '<b>DEMO</b> &ndash; '
        '<span class="de">simulierte Werte, keine echte Anlage. Änderungen gelten nur in diesem Tab und sind nach dem Neuladen weg.</span>'
        '<span class="en">simulated values, no real system. Changes only apply in this tab and are gone after reloading.</span>'
        ' &middot; Firmware %s</div>\n' % fw)
    html = replace_once(html, "<body>", banner, "<body>")

    html = replace_once(html,
                        "window.location.href = '/api/logs/download?file=' + encodeURIComponent(sel.value);",
                        "demoDownload(sel.value);", "Log-Download")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8", newline="\n")
    print(f"Demo geschrieben: {OUT}  ({len(html) // 1024} KB, FW {fw})")


if __name__ == "__main__":
    main()
