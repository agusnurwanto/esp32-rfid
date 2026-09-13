#include "Config.h"
#include "WiFiProvisioning.h"
#include "WebDashboard.h"
#include "RFIDAttendance.h"
#include "DisplayManager.h"
#include "UserRegistry.h"

// Global objects
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;
WebServer webServer(80);

// Global state
bool sdCardReady = false;
bool rtcReady = false;
bool wifiProvisioningMode = false;
unsigned long lastDisplayUpdate = 0;

// Config constants
const char* AP_SSID     = "ESP32-Setup";
const char* AP_PASSWORD = "1234567890";
const char* CONFIG_PATH = "/wifi_config.txt";
const char* USERS_PATH  = "/users.json";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("   INISIALISASI ABSENSI IoT + WEB DASHBOARD  ");
  Serial.println("==========================================");
  
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_GREEN, HIGH); delay(300); digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH); delay(300); digitalWrite(PIN_LED_RED, LOW);

  // Init LittleFS
  if (!LittleFS.begin(true)) Serial.println("[LittleFS] Init failed");

  // Init WiFi
  setupWiFi();

  // Init OLED
  initDisplay();

  // Init RTC
  if (rtc.begin()) {
    rtcReady = true;
    if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Init RFID & SD
  initRFID();
  initSDCard();

  // Setup web server routes
  setupWebServer();
}

void loop() {
  webServer.handleClient();
  DateTime now = rtcReady ? rtc.now() : DateTime(2026, 9, 8, 12, 0, 0);

  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    if (wifiProvisioningMode) {
      renderSetupScreen();
    } else {
      renderIdleScreen(now);
    }
  }

  processRFID();
}