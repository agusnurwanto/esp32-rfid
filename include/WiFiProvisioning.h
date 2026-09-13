#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include "Config.h"

// WiFi config functions
bool loadWiFiConfig(String& outSSID, String& outPass);
String getProvisionedSSID();
void handleAPMode();
void handleSaveConfig();
void handleConfigPage();
void setupWiFi();

#endif