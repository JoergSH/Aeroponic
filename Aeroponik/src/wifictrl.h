#ifndef WIFICTRL_H
#define WIFICTRL_H

#include <stdint.h>

struct WifiConfig {
    char ssid[33];      // max. 32 Zeichen + Nullterminator
    char password[65];  // max. 64 Zeichen (WPA2) + Nullterminator
};

extern WifiConfig wifiConfig;

void loadWifiConfig();
void saveWifiConfig();

#endif
