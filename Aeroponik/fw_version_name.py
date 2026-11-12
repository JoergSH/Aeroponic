# PlatformIO-Post-Build-Skript: legt nach jedem Build eine Kopie von firmware.bin mit
# Versionsnummer im Projektordner firmware/ ab, z.B. firmware/firmware-1-19-1.bin. Bewusst
# ausserhalb von .pio/ -- "Clean" loescht dort alles, die alten Versionen sollen aber zum
# schnellen Zurueckflashen erhalten bleiben (nicht im Repo, siehe .gitignore). Ein erneuter
# Build derselben Version ueberschreibt nur deren Datei. Die Version kommt aus FW_VERSION in
# src/config.h, muss also nur dort gepflegt werden.
import os
import re
import shutil

Import("env")


def read_fw_version():
    path = os.path.join(env.subst("$PROJECT_DIR"), "src", "config.h")
    try:
        with open(path, encoding="utf-8") as f:
            m = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', f.read())
            return m.group(1) if m else None
    except OSError:
        return None


def copy_versioned(source, target, env):
    version = read_fw_version()
    if not version:
        print("fw_version_name: FW_VERSION nicht gefunden, keine Kopie angelegt")
        return
    src = str(target[0])
    name = "firmware-" + version.replace(".", "-") + ".bin"
    out_dir = os.path.join(env.subst("$PROJECT_DIR"), "firmware")
    os.makedirs(out_dir, exist_ok=True)
    dst = os.path.join(out_dir, name)
    shutil.copyfile(src, dst)
    print("fw_version_name: " + dst)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_versioned)
