#include "DisplayManager.h"

void initDisplay() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] OK");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println(wifiProvisioningMode ? "Mode Setup" : "Ready");
    display.display();
  } else {
    Serial.println("[OLED] Init failed");
  }
}

void renderIdleScreen(const DateTime& now) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 2);
  display.println("SISTEM ABSENSI");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  
  char tb[10];
  snprintf(tb, sizeof(tb), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.setTextSize(2);
  display.setCursor(16, 18);
  display.println(tb);
  
  char db[12];
  snprintf(db, sizeof(db), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  display.setTextSize(1);
  display.setCursor(32, 40);
  display.println(db);
  
  display.drawLine(0, 52, 128, 52, SSD1306_WHITE);
  display.setCursor(8, 55);
  display.print("Tempelkan Kartu...");
  display.display();
}

void renderSetupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 2);
  display.println("════════════════");
  display.setCursor(8, 14);
  display.println("ESP32 Setup");
  display.setCursor(8, 28);
  display.println("SSID: ESP32-Setup");
  display.setCursor(8, 40);
  display.println("Pass: 1234567890");
  display.setCursor(8, 54);
  display.print("IP: 192.168.4.1");
  display.display();
}

void showMessage(const String& l1, const String& l2, const String& l3, int delayMs) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("====================");
  display.setCursor(0, 16);
  display.println(l1);
  display.setCursor(0, 32);
  display.println(l2);
  display.setCursor(0, 48);
  display.println(l3);
  display.display();
  if (delayMs > 0) delay(delayMs);
}