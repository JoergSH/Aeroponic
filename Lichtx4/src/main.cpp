#include <Arduino.h>
#include <Preferences.h>
#include "pinout.h"
#include "rs485_slave.h"
#include "fw_update.h"
#include "relay/relay_manager.h"

// ========== RS485-Schreibbefehl vom Master ==========

static void on_rs485_write(uint16_t reg, uint16_t value) {
    if (reg == RS485_REG_MASK) {
        uint8_t mask = (uint8_t)(value & 0x0F);
        relay_manager_set_mask(mask);
        rs485_slave_update(mask);
    }
}

// ========== Serial-Debugbefehle (Testen ohne Bus) ==========

static void handle_serial_command(const String& cmd) {
    if (cmd == "STATUS") {
        uint8_t mask = relay_manager_get_mask();
        Serial.printf("[RELAY] Maske: 0x%02X  R1=%d R2=%d R3=%d R4=%d\n",
                      mask,
                      (mask >> 0) & 1, (mask >> 1) & 1,
                      (mask >> 2) & 1, (mask >> 3) & 1);
    } else if (cmd.startsWith("MASK ")) {
        int val = cmd.substring(5).toInt();
        uint8_t mask = (uint8_t)(val & 0x0F);
        relay_manager_set_mask(mask);
        rs485_slave_update(mask);
    } else if (cmd.startsWith("R") && cmd.length() >= 4) {
        int ch  = cmd.charAt(1) - '0';
        int val = cmd.charAt(3) - '0';
        if (ch >= 1 && ch <= 4 && (val == 0 || val == 1)) {
            uint8_t mask = relay_manager_get_mask();
            if (val) mask |=  (1 << (ch - 1));
            else     mask &= ~(1 << (ch - 1));
            relay_manager_set_mask(mask);
            rs485_slave_update(mask);
        } else {
            Serial.println("[CMD] Syntax: R<1-4> <0|1>  z.B. R1 1");
        }
    } else if (cmd.length() > 0) {
        Serial.println("[CMD] STATUS  MASK <0-15>  R<1-4> <0|1>");
    }
}

// ========== Setup / Loop ==========

void setup() {
    // Relais zuerst: nach einem Reset sind alle SSRs aus, bis die gespeicherte Maske wieder
    // anliegt -- jede Verzoegerung davor (frueher delay(500) fuer Serial) ist Dunkelzeit im Zelt.
    relay_manager_init();

    Serial.begin(115200);
    Serial.println("\n[MAIN] Lichtx4 startet...");
    Serial.printf("[MAIN] Firmware %d.%d.%d (Update ueber RS485 moeglich, siehe fw_update.h)\n",
                  FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);

    // Neustart-Diagnose fuer den Master (siehe rs485_slave.h): Startzaehler + Reset-Grund
    Preferences sys;
    sys.begin("sys", false);
    uint16_t boots = sys.getUShort("boots", 0) + 1;
    sys.putUShort("boots", boots);
    sys.end();
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[MAIN] Start Nr. %u, Reset-Grund %d\n", boots, (int)reason);
    rs485_slave_set_reg(RS485_REG_BOOTS, boots);
    rs485_slave_set_reg(RS485_REG_RESET, (uint16_t)reason);

    rs485_slave_update(relay_manager_get_mask());  // Preferences-Zustand sofort fuer Poll sichtbar
    rs485_slave_set_write_cb(on_rs485_write);
    rs485_slave_init(RS485_TX, RS485_RX);
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        handle_serial_command(cmd);
    }

    rs485_slave_loop();
    rs485_slave_set_reg(RS485_REG_UPTIME, (uint16_t)(millis() / 60000UL));
}
