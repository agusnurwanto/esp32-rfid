#ifndef USER_REGISTRY_H
#define USER_REGISTRY_H

#include "Config.h"
#include <ArduinoJson.h>

// User registry for dynamic UID-to-name mapping
// Stores: UID, Name, Role (optional), Registered timestamp
extern const char* USERS_PATH;

struct UserRecord {
  String uid;
  String name;
  String role;
  unsigned long registered;
};

// Functions
bool loadUsers(JsonDocument& doc);
bool saveUsers(const JsonDocument& doc);
bool registerUser(const String& uid, const String& name, const String& role = "");
bool findUser(const String& uid, UserRecord& outUser);
String getUserName(const String& uid);
void handleRegisterUser();
void handleGetUsers();
void handleDeleteUser();

#endif