#ifndef USER_REGISTRY_H
#define USER_REGISTRY_H

#include "Config.h"

// User registry for dynamic UID-to-name mapping
// Stores: UID, Name, Role (optional), Registered timestamp
const char* USERS_PATH = "/users.json";

struct UserRecord {
  String uid;
  String name;
  String role;
  unsigned long registered;
};

bool loadUsers(JsonArray& users);
bool saveUsers(JsonArray& users);
bool registerUser(const String& uid, const String& name, const String& role = "");
bool findUser(const String& uid, UserRecord& outUser);
String getUserName(const String& uid);
void handleRegisterUser();
void handleGetUsers();
void handleDeleteUser();

#endif