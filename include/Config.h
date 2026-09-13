#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// Pin konfigurasi
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_RFID_SS   5
#define PIN_RFID_RST  15
#define PIN_SD_CS     4
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22
#define PIN_LED_GREEN 2
#define PIN_LED_RED   12
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// WiFi AP config
const char* AP_SSID     = "ESP32-Setup";
const char* AP_PASSWORD = "1234567890";
const char* CONFIG_PATH = "/wifi_config.txt";

// Global objects
extern MFRC522 rfid;
extern Adafruit_SSD1306 display;
extern RTC_DS1307 rtc;
extern WebServer webServer;

// Global state
extern bool sdCardReady;
extern bool rtcReady;
extern bool wifiProvisioningMode;
extern unsigned long lastDisplayUpdate;

// Common struct
struct ServerResponse {
  bool isRegistered;
  String name;
  String status;
};

// Utility functions
String extractJsonValue(const String& json, const String& key);

#endif