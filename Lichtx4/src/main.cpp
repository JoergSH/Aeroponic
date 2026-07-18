#include <Arduino.h>
#include "pinout.h"
#include "rs485_slave.h"
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
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[MAIN] Lichtx4 startet...");

    relay_manager_init();
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
}
