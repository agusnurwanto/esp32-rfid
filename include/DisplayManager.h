#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Config.h"

// Display functions
void initDisplay();
void renderIdleScreen(const DateTime& now);
void renderSetupScreen();
void showMessage(const String& l1, const String& l2, const String& l3, int delayMs = 2000);

#endif