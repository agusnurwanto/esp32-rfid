#ifndef RFID_ATTENDANCE_H
#define RFID_ATTENDANCE_H

#include "Config.h"

// RFID & Attendance functions
ServerResponse sendAndVerifyCard(const String& uid);
void logToSDCard(const String& ts, const String& uid, const String& name, const String& status);
void saveAttendanceLog(const String& ts, const String& uid, const String& name, const String& status);
void initRFID();
void initSDCard();
void processRFID();

#endif