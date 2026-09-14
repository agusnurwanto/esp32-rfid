#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include "Config.h"
#include "UserRegistry.h"

// HTML templates (defined in WebDashboard.cpp)
extern const char* SETUP_HTML;
extern const char* DASHBOARD_HTML;
extern const char* CHAT_HTML;
extern const char* USERS_HTML;

// Web Dashboard functions
void handleRoot();
void handleChatPage();
void handleUsersPage();
void handleConfigPage();
void handleAPIChat();
void handleAPIStatus();
void handleAPIFeatures();
void setupWebServer();

#endif