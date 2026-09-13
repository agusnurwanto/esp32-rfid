#include "UserRegistry.h"
#include <ArduinoJson.h>

bool loadUsers(JsonArray& users) {
  if (!LittleFS.exists(USERS_PATH)) return false;
  File f = LittleFS.open(USERS_PATH, "r");
  if (!f) return false;
  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  users = doc.as<JsonArray>();
  return true;
}

bool saveUsers(JsonArray& users) {
  File f = LittleFS.open(USERS_PATH, "w");
  if (!f) return false;
  DynamicJsonDocument doc(8192);
  doc.set(users);
  serializeJson(doc, f);
  f.close();
  return true;
}

bool registerUser(const String& uid, const String& name, const String& role) {
  DynamicJsonDocument doc(8192);
  JsonArray users;
  if (loadUsers(users)) {
    doc.set(users);
  } else {
    users = doc.to<JsonArray>();
  }
  
  // Check if UID already exists
  for (JsonObject u : users) {
    if (u["uid"] == uid) {
      u["name"] = name;
      u["role"] = role;
      u["registered"] = millis();
      return saveUsers(users);
    }
  }
  
  // Add new user
  JsonObject newUser = users.createNestedObject();
  newUser["uid"] = uid;
  newUser["name"] = name;
  newUser["role"] = role;
  newUser["registered"] = millis();
  
  return saveUsers(users);
}

bool findUser(const String& uid, UserRecord& outUser) {
  DynamicJsonDocument doc(8192);
  JsonArray users;
  if (!loadUsers(users)) return false;
  
  for (JsonObject u : users) {
    if (u["uid"] == uid) {
      outUser.uid = u["uid"].as<String>();
      outUser.name = u["name"].as<String>();
      outUser.role = u["role"].as<String>();
      outUser.registered = u["registered"].as<unsigned long>();
      return true;
    }
  }
  return false;
}

String getUserName(const String& uid) {
  UserRecord user;
  if (findUser(uid, user)) return user.name;
  return "Tidak Dikenal";
}

void handleRegisterUser() {
  if (webServer.method() != HTTP_POST) {
    webServer.send(405, "text/plain", "Method not allowed");
    return;
  }
  String body = webServer.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    webServer.send(400, "text/plain", "Invalid JSON");
    return;
  }
  String uid = doc["uid"] | "";
  String name = doc["name"] | "";
  String role = doc["role"] | "";
  if (uid.length() == 0 || name.length() == 0) {
    webServer.send(400, "text/plain", "UID and name required");
    return;
  }
  if (registerUser(uid, name, role)) {
    webServer.send(200, "application/json", "{\"success\":true,\"message\":\"User registered\"}");
  } else {
    webServer.send(500, "text/plain", "Failed to save");
  }
}

void handleGetUsers() {
  DynamicJsonDocument doc(8192);
  JsonArray users;
  loadUsers(users);
  doc.set(users);
  String output;
  serializeJson(doc, output);
  webServer.send(200, "application/json", output);
}

void handleDeleteUser() {
  if (webServer.method() != HTTP_DELETE && webServer.method() != HTTP_POST) {
    webServer.send(405, "text/plain", "Method not allowed");
    return;
  }
  String uid = webServer.arg("uid");
  if (uid.length() == 0) {
    webServer.send(400, "text/plain", "UID required");
    return;
  }
  DynamicJsonDocument doc(8192);
  JsonArray users;
  if (!loadUsers(users)) {
    webServer.send(404, "text/plain", "No users");
    return;
  }
  JsonArray newUsers = doc.to<JsonArray>();
  bool found = false;
  for (JsonObject u : users) {
    if (u["uid"] != uid) {
      newUsers.add(u);
    } else {
      found = true;
    }
  }
  if (!found) {
    webServer.send(404, "text/plain", "User not found");
    return;
  }
  if (saveUsers(newUsers)) {
    webServer.send(200, "application/json", "{\"success\":true}");
  } else {
    webServer.send(500, "text/plain", "Failed to save");
  }
}