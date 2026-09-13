#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include "Config.h"
#include "UserRegistry.h"

// Web Dashboard functions
void handleRoot();
void handleChatPage();
void handleAPIChat();
void handleAPIStatus();
void handleAPIFeatures();
void setupWebServer();

#endif