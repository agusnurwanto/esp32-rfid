#include "UserRegistry.h"

bool loadUsers(JsonDocument& doc) {
  if (!LittleFS.exists(USERS_PATH)) return false;
  File f = LittleFS.open(USERS_PATH, "r");
  if (!f) return false;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  return true;
}

bool saveUsers(const JsonDocument& doc) {
  File f = LittleFS.open(USERS_PATH, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

bool registerUser(const String& uid, const String& name, const String& role) {
  JsonDocument doc;
  JsonArray users = doc.to<JsonArray>();
  
  // Load existing users
  JsonDocument loadDoc;
  if (loadUsers(loadDoc)) {
    JsonArray existingUsers = loadDoc.as<JsonArray>();
    for (JsonObject u : existingUsers) {
      users.add(u);
    }
  }
  
  // Check if UID already exists
  for (JsonObject u : users) {
    String existingUid = u["uid"].as<String>();
    if (existingUid == uid) {
      u["name"] = name;
      u["role"] = role;
      u["registered"] = millis();
      return saveUsers(doc);
    }
  }
  
  // Add new user
  JsonObject newUser = users.add<JsonObject>();
  newUser["uid"] = uid;
  newUser["name"] = name;
  newUser["role"] = role;
  newUser["registered"] = millis();
  
  return saveUsers(doc);
}

bool findUser(const String& uid, UserRecord& outUser) {
  JsonDocument doc;
  if (!loadUsers(doc)) return false;
  
  JsonArray users = doc.as<JsonArray>();
  for (JsonObject u : users) {
    String existingUid = u["uid"].as<String>();
    if (existingUid == uid) {
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
  JsonDocument doc;
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
  JsonDocument doc;
  loadUsers(doc);
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
  JsonDocument doc;
  if (!loadUsers(doc)) {
    webServer.send(404, "text/plain", "No users");
    return;
  }
  JsonArray users = doc.as<JsonArray>();
  JsonDocument newDoc;
  JsonArray newUsers = newDoc.to<JsonArray>();
  bool found = false;
  for (JsonObject u : users) {
    String existingUid = u["uid"].as<String>();
    if (existingUid != uid) {
      newUsers.add(u);
    } else {
      found = true;
    }
  }
  if (!found) {
    webServer.send(404, "text/plain", "User not found");
    return;
  }
  if (saveUsers(newDoc)) {
    webServer.send(200, "application/json", "{\"success\":true}");
  } else {
    webServer.send(500, "text/plain", "Failed to save");
  }
}