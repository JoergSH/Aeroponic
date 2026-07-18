#include "config.h"

// WiFi-Zugangsdaten fürs Heim-WLAN liegen nicht hier, siehe wifictrl.h/.cpp.

// Access Point Konfiguration
const char* AP_SSID     = "ESP32-Aeroponik";
const char* AP_PASSWORD = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
bool USE_ACCESS_POINT = false;
