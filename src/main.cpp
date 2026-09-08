#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"  // ← kredensial lokal, tidak di-commit ke Git

// Konfigurasi Pin SPI (Berbagi bus antara MFRC522 dan MicroSD)
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_RFID_SS    5
#define PIN_RFID_RST  22
#define PIN_SD_CS      4

// Konfigurasi Pin I2C (Berbagi bus antara OLED & DS1307 RTC)
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   15

// Konfigurasi Pin Indikator LED
#define PIN_LED_GREEN  2
#define PIN_LED_RED   12

// Dimensi Layar OLED
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;

bool sdCardReady = false;
bool rtcReady = false;
unsigned long lastDisplayUpdate = 0;

// Struktur untuk menampung respon JSON dari Google Apps Script
struct ServerResponse {
  bool isRegistered;
  String name;
  String status;
};

// Fungsi pembantu untuk mengambil nilai dari string JSON sederhana
String extractJsonValue(const String& json, const String& key) {
  String searchKey = "\"" + key + "\":\"";
  int start = json.indexOf(searchKey);
  if (start != -1) {
    start += searchKey.length();
    int end = json.indexOf("\"", start);
    if (end != -1) return json.substring(start, end);
  }
  return "";
}

// Fungsi untuk memverifikasi UID ke Google Sheets dan menerima nama siswa
ServerResponse checkAndLogToGoogle(const String& date, const String& time, const String& uid) {
  ServerResponse responseData = {false, "Tidak Dikenal", "Ditolak"};

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Tidak terhubung. Pengecekan cloud dibatalkan.");
    return responseData;
  }

  HTTPClient http;
  
  // Susun URL permintaan dengan query parameter UID, Date, dan Time
  String url = GOOGLE_SCRIPT_URL + "?uid=" + uid + "&date=" + date + "&time=" + time;
  url.replace(" ", "%20"); 

  Serial.println("[HTTP] Menghubungi Google Sheets Database...");
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("[HTTP] Respon Google: " + payload);

    // Cek apakah kartu terdaftar
    if (payload.indexOf("\"registered\":true") != -1 || payload.indexOf("\"registered\": true") != -1) {
      responseData.isRegistered = true;
    } else {
      responseData.isRegistered = false;
    }

    String parsedName = extractJsonValue(payload, "name");
    String parsedStatus = extractJsonValue(payload, "status");

    if (parsedName.length() > 0) responseData.name = parsedName;
    if (parsedStatus.length() > 0) responseData.status = parsedStatus;

  } else {
    Serial.printf("[HTTP] Gagal terhubung, kode error: %d\n", httpCode);
  }
  
  http.end();
  return responseData;
}

void showMessage(const String& line1, const String& line2, const String& line3, int delayMs = 2000) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("====================");
  display.setCursor(0, 16); display.println(line1);
  display.setCursor(0, 32); display.println(line2);
  display.setCursor(0, 48); display.println(line3);
  display.display();
  if (delayMs > 0) delay(delayMs);
}

void renderIdleScreen(const DateTime& now) {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 2);
  display.println("SISTEM ABSENSI");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  // Waktu
  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.setTextSize(2);
  display.setCursor(16, 18);
  display.println(timeBuffer);

  // Tanggal
  char dateBuffer[12];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  display.setTextSize(1);
  display.setCursor(32, 40);
  display.println(dateBuffer);

  // Footer
  display.drawLine(0, 52, 128, 52, SSD1306_WHITE);
  display.setCursor(8, 55);
  display.print("Tempelkan Kartu...");
  
  display.display();
}

void logToSDCard(const String& timestamp, const String& uid, const String& name, const String& status) {
  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS, LOW);

  if (!sdCardReady) {
    digitalWrite(PIN_SD_CS, HIGH);
    return;
  }

  File logFile = SD.open("/absensi.csv", FILE_APPEND);
  if (logFile) {
    logFile.print(timestamp); logFile.print(",");
    logFile.print(uid);       logFile.print(",");
    logFile.print(name);      logFile.print(",");
    logFile.println(status);
    logFile.close();
    Serial.println("[SD] Log berhasil ditulis ke MicroSD.");
  }

  digitalWrite(PIN_SD_CS, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("   INISIALISASI ABSENSI IoT GOOGLE CLOUD  ");
  Serial.println("==========================================");

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // Tes Kedip LED
  digitalWrite(PIN_LED_GREEN, HIGH); delay(300); digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);   delay(300); digitalWrite(PIN_LED_RED, LOW);

  // Inisialisasi WiFi
  Serial.print("[WiFi] Menghubungkan ke "); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 15) {
    delay(500); Serial.print("."); retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Terhubung! IP: " + WiFi.localIP().toString());
  }

  // Inisialisasi I2C & OLED
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println("Connecting Cloud...");
    display.display();
  }

  // Inisialisasi RTC DS1307
  if (rtc.begin()) {
    rtcReady = true;
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Inisialisasi SPI Bus
  pinMode(PIN_RFID_SS, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS, HIGH);
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

  // Inisialisasi RFID
  digitalWrite(PIN_RFID_SS, LOW);
  rfid.PCD_Init();
  digitalWrite(PIN_RFID_SS, HIGH);

  // Inisialisasi MicroSD
  digitalWrite(PIN_SD_CS, LOW);
  if (SD.begin(PIN_SD_CS)) {
    sdCardReady = true;
    if (!SD.exists("/absensi.csv")) {
      File headerFile = SD.open("/absensi.csv", FILE_WRITE);
      if (headerFile) {
        headerFile.println("Waktu,UID,Nama,Status");
        headerFile.close();
      }
    }
  }
  digitalWrite(PIN_SD_CS, HIGH);

  showMessage("  SISTEM READY", " Tempelkan Kartu", " Database Online", 1500);
}

void loop() {
  DateTime now = rtcReady ? rtc.now() : DateTime(2026, 9, 8, 12, 0, 0);

  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    renderIdleScreen(now);
  }

  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_RFID_SS, LOW);

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    digitalWrite(PIN_RFID_SS, HIGH);
    return;
  }

  // Ambil UID Kartu
  String scannedUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) scannedUID += "0";
    scannedUID += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) scannedUID += " ";
  }
  scannedUID.toUpperCase();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  digitalWrite(PIN_RFID_SS, HIGH);

  Serial.println("\n[RFID] Kartu Terdeteksi! UID: " + scannedUID);

  // Tampilkan pesan proses di OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 20); display.println("Memeriksa Card...");
  display.setCursor(15, 36); display.println("UID: " + scannedUID);
  display.display();

  char dateBuf[15], timeBuf[15], timeStampBuf[25];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  snprintf(timeStampBuf, sizeof(timeStampBuf), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  // Kirim UID ke Google Sheets & dapatkan data siswa
  ServerResponse res = checkAndLogToGoogle(String(dateBuf), String(timeBuf), scannedUID);

  // Tampilkan hasil berdasarkan respon Google Cloud
  if (res.isRegistered) {
    Serial.println("[AKSES] DITERIMA: " + res.name);
    digitalWrite(PIN_LED_GREEN, HIGH);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);   display.println("====================");
    display.setCursor(18, 14); display.println("SELAMAT DATANG");
    display.setCursor(0, 28);  display.println(res.name);
    display.setCursor(0, 44);  display.println("Absen Berhasil!");
    display.display();

    logToSDCard(timeStampBuf, scannedUID, res.name, "Hadir");

    delay(2500);
    digitalWrite(PIN_LED_GREEN, LOW);

  } else {
    Serial.println("[AKSES] DITOLAK: Kartu Tidak Terdaftar!");
    digitalWrite(PIN_LED_RED, HIGH);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);   display.println("====================");
    display.setCursor(15, 14); display.println("AKSES DITOLAK!");
    display.setCursor(0, 28);  display.println("UID: " + scannedUID);
    display.setCursor(0, 44);  display.println("Tidak Terdaftar");
    display.display();

    logToSDCard(timeStampBuf, scannedUID, "N/A", "Ditolak");

    delay(2500);
    digitalWrite(PIN_LED_RED, LOW);
  }
}