#include "WiFiProvisioning.h"
#include "WebDashboard.h"

bool loadWiFiConfig(String& outSSID, String& outPass) {
  if (!LittleFS.exists(CONFIG_PATH)) return false;
  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return false;
  String ssid = f.readStringUntil('\n'); ssid.trim();
  String pass = f.readStringUntil('\n'); pass.trim();
  f.close();
  if (ssid.length() == 0) { LittleFS.remove(CONFIG_PATH); return false; }
  outSSID = ssid; outPass = pass; return true;
}

String getProvisionedSSID() {
  String ssid, pass;
  if (loadWiFiConfig(ssid, pass)) return ssid;
  return "";
}

void handleAPMode() {
  wifiProvisioningMode = true;
  Serial.println("[WiFi] Starting AP: ESP32-Setup");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("[WiFi] Connect to 'ESP32-Setup', browse to http://192.168.4.1");
}

void setupWiFi() {
  String ssid, pass;
  if (loadWiFiConfig(ssid, pass)) {
    Serial.print("[WiFi] Connecting to '" + ssid + "'...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 30) { delay(500); Serial.print("."); i++; }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("\n[WiFi] Failed - entering AP mode");
      handleAPMode();
    }
  } else {
    handleAPMode();
  }
}

void handleConfigPage() {
  webServer.send(200, "text/html", SETUP_HTML);
}

void handleSaveConfig() {
  if (webServer.method() != HTTP_POST) { webServer.send(405, "text/plain", "Method not allowed"); return; }
  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");
  if (ssid.length() == 0) { webServer.send(400, "text/plain", "SSID required"); return; }
  
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (f) { f.println(ssid); f.println(pass); f.close(); }
  
  Serial.println("[WiFi] Saved credentials: " + ssid);
  webServer.send(200, "text/html", "<!DOCTYPE html><html><body><h1>Konfigurasi Disimpan!</h1><p>SSID: " + ssid + 
         "</p><p>Menghubungkan...</p><script>setTimeout(()=>{location.reload();},1500);</script></body></html>");
  delay(1000);
  ESP.restart();
}