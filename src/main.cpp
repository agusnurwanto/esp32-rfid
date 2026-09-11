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
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"  // ← kredensial lokal, tidak di-commit ke Git

// ════════════════════════════════════════════════════════════════════════════
// KONFIGURASI OPSI PENYIMPANAN DATA
// ════════════════════════════════════════════════════════════════════════════
// Pilih satu dari opsi berikut:
// #define USE_STORAGE_GAS        // Uncomment ini untuk Google Apps Script
#define USE_STORAGE_FIREBASE    // Uncomment ini untuk Firebase Realtime Database

// Jika menggunakan FIREBASE, aktifkan library FirebaseClient di bawah ini
#if defined(USE_STORAGE_FIREBASE)
  #include <FirebaseClient.h>
#endif

// ════════════════════════════════════════════════════════════════════════════
// AKTIFKAN saat testing di Wokwi (VS Code / Online) — HTTPS tidak didukung
// NONAKTIFKAN (komen baris di bawah) saat deploy ke hardware ESP32 asli
// #define WOKWI_SIMULATION
// ════════════════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════════════════
// OBJEK GLOBAL FIREBASE (hanya saat USE_STORAGE_FIREBASE)
// ════════════════════════════════════════════════════════════════════════════
#if defined(USE_STORAGE_FIREBASE)
  FirebaseClient firebase;
  WiFiClientSecure ssl;
  bool firebaseReady = false;
#endif

// Konfigurasi Pin SPI (Berbagi bus antara MFRC522 dan MicroSD)
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_RFID_SS    5
#define PIN_RFID_RST  15
#define PIN_SD_CS      4

// ════════════════════════════════════════════════════════════════════════════
// HELPER FUNCTION: Clean UID untuk Firebase (remove spaces & special chars)
// ════════════════════════════════════════════════════════════════════════════
String cleanUIDForFirebase(const String& uid) {
  String cleaned = uid;
  // Replace spaces dengan underscore
  cleaned.replace(" ", "_");
  // Replace colons dengan underscore
  cleaned.replace(":", "_");
  // Replace slashes dengan underscore
  cleaned.replace("/", "_");
  return cleaned;
}

// Konfigurasi Pin I2C (Berbagi bus antara OLED & DS1307 RTC)
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

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

// ════════════════════════════════════════════════════════════════════════════
// FUNGSI FIREBASE: Inisialisasi dan operasi database
// ════════════════════════════════════════════════════════════════════════════
#if defined(USE_STORAGE_FIREBASE)

void initializeFirebase() {
  Serial.println("[Firebase] Inisialisasi Firebase Realtime Database dengan FirebaseClient...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] WiFi belum terhubung, skip Firebase init");
    return;
  }

  // Setup FirebaseClient dengan kredensial
  // Format: firebase.setSecure() untuk menggunakan SSL
  ssl.setInsecure(); // Bypass SSL verification (untuk development)
  
  // Konfigurasi koneksi Firebase
  // FirebaseClient menggunakan REST API langsung
  firebaseReady = true;
  Serial.println("[Firebase] FirebaseClient siap. Tunggu test koneksi...");
}

// Test koneksi Firebase dengan simple GET
bool testFirebaseConnection() {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  Serial.println("[Firebase] Test koneksi...");
  
  // Buat HTTP request ke Firebase
  HTTPClient http;
  http.setConnectTimeout(3000);
  
  String testUrl = String(FIREBASE_DATABASE_URL) + "/.json?auth=" + String(FIREBASE_API_KEY);
  
  if (http.begin(ssl, testUrl)) {
    int httpCode = http.GET();
    if (httpCode == 200) {
      Serial.println("[Firebase] ✓ Koneksi berhasil!");
      http.end();
      return true;
    } else {
      Serial.printf("[Firebase] HTTP Error: %d\n", httpCode);
      http.end();
      return false;
    }
  } else {
    Serial.println("[Firebase] Gagal membuat request");
    return false;
  }
}

#endif

// Fungsi untuk memverifikasi UID ke Firebase (menggunakan REST API)
ServerResponse checkAndLogToFirebase(const String& date, const String& time, const String& uid) {
  ServerResponse responseData = {false, "Tidak Dikenal", "Ditolak"};

  if (!firebaseReady || WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] Tidak terhubung. Pengecekan ditolak.");
    return responseData;
  }

  Serial.println("[Firebase] Mencari UID di database...");

  // Clean UID untuk Firebase compatibility
  String cleanUID = cleanUIDForFirebase(uid);
  
  // Gunakan .json dengan auth parameter
  // Firebase REST API memerlukan auth untuk verifikasi rules
  String path = String(FIREBASE_DATABASE_URL) + "/students/" + cleanUID + ".json?auth=" + String(FIREBASE_API_KEY);
  
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  
  Serial.printf("[Firebase] GET: %s\n", path.c_str());
  Serial.printf("[Firebase] API Key Length: %d chars\n", String(FIREBASE_API_KEY).length());
  
  if (http.begin(ssl, path)) {
    int httpCode = http.GET();
    Serial.printf("[Firebase] Response Code: %d\n", httpCode);
    
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.printf("[Firebase] Data: %s\n", payload.c_str());

      // Cek apakah null (node tidak ada)
      if (payload == "null") {
        Serial.println("[Firebase] UID tidak ditemukan (empty node)");
        responseData.isRegistered = false;
      } else {
        // Parse JSON response
        if (payload.indexOf("\"registered\":true") != -1 || payload.indexOf("\"registered\": true") != -1) {
          responseData.isRegistered = true;
        }

        String parsedName = extractJsonValue(payload, "name");
        String parsedStatus = extractJsonValue(payload, "status");

        if (parsedName.length() > 0) responseData.name = parsedName;
        if (parsedStatus.length() > 0) responseData.status = parsedStatus;
      }
      
    } else if (httpCode == 404) {
      Serial.println("[Firebase] UID tidak ditemukan (404)");
      responseData.isRegistered = false;
    } else if (httpCode == 400) {
      String errorBody = http.getString();
      Serial.println("[Firebase] ✗ Bad Request (400)");
      Serial.println("[Firebase] Possible causes:");
      Serial.println("  - UID format invalid (has special chars)");
      Serial.println("  - Database URL wrong");
      Serial.printf("  - Response: %s\n", errorBody.c_str());
    } else if (httpCode == 401) {
      String errorBody = http.getString();
      Serial.println("[Firebase] ✗ Unauthorized (401)");
      Serial.println("[Firebase] Fix checklist:");
      Serial.println("  1. Verify FIREBASE_API_KEY in secrets.h");
      Serial.println("  2. Get fresh API Key from Firebase Console");
      Serial.println("  3. Check Firebase Rules allow .read");
      Serial.println("  4. Try: Rules > Start in test mode (public)");
      if (errorBody.length() > 0) {
        Serial.printf("  5. Error: %s\n", errorBody.c_str());
      }
    } else {
      String errorBody = http.getString();
      Serial.printf("[Firebase] HTTP Error %d\n", httpCode);
      if (errorBody.length() > 0) {
        Serial.printf("[Firebase] Response: %s\n", errorBody.c_str());
      }
    }
    
    http.end();
  } else {
    Serial.println("[Firebase] Gagal membuat HTTP request");
  }

  return responseData;
}

// Fungsi untuk menyimpan log ke Firebase (menggunakan REST API)
void logToFirebase(const String& timestamp, const String& uid, const String& name, const String& status) {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) {
    Serial.println("[Firebase] Tidak terhubung. Log tidak disimpan.");
    return;
  }

  Serial.println("[Firebase] Menyimpan log ke database...");

  // Clean UID untuk Firebase path compatibility
  String cleanUID = cleanUIDForFirebase(uid);
  
  // Gunakan POST dengan auth parameter
  // Format: https://DATABASE_URL/attendance.json?auth=API_KEY
  String path = String(FIREBASE_DATABASE_URL) + "/attendance.json?auth=" + String(FIREBASE_API_KEY);
  
  // Buat JSON payload - minimal data untuk reduce errors
  String jsonData = "{";
  jsonData += "\"timestamp\":\"" + timestamp + "\",";
  jsonData += "\"uid\":\"" + cleanUID + "\",";
  jsonData += "\"name\":\"" + name + "\",";
  jsonData += "\"status\":\"" + status + "\"";
  jsonData += "}";

  Serial.printf("[Firebase] POST to: %s\n", path.c_str());
  Serial.printf("[Firebase] Data length: %d bytes\n", jsonData.length());
  Serial.printf("[Firebase] API Key Length: %d chars\n", String(FIREBASE_API_KEY).length());

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  
  if (http.begin(ssl, path)) {
    // Use POST untuk auto-generate key di Firebase
    int httpCode = http.POST(jsonData);
    Serial.printf("[Firebase] Response Code: %d\n", httpCode);
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.println("[Firebase] ✓ Log berhasil disimpan!");
      Serial.printf("[Firebase] Response: %s\n", response.c_str());
    } else if (httpCode == 400) {
      String errorBody = http.getString();
      Serial.println("[Firebase] ✗ Bad Request (400)");
      Serial.println("[Firebase] Debugging tips:");
      Serial.println("  1. Verifikasi FIREBASE_DATABASE_URL di secrets.h");
      Serial.println("  2. Pastikan Firebase Rules mengizinkan write ke /attendance");
      Serial.println("  3. Cek apakah UID memiliki karakter khusus");
      Serial.println("  4. Format JSON valid? Lihat payload di atas");
      if (errorBody.length() > 0) {
        Serial.printf("  5. Server error: %s\n", errorBody.c_str());
      }
    } else if (httpCode == 401) {
      String errorBody = http.getString();
      Serial.println("[Firebase] ✗ Unauthorized (401)");
      Serial.println("[Firebase] Authentication failed. Fix:");
      Serial.println("  1. Check FIREBASE_API_KEY in secrets.h (must not be empty)");
      Serial.println("  2. Get fresh API Key from Firebase Console > Project Settings");
      Serial.println("  3. Update Firebase Rules to allow write:");
      Serial.println("     {\"rules\": {\".read\": true, \".write\": true}}");
      Serial.println("  4. Or use: Rules > Start in test mode (temporarily public)");
      if (errorBody.length() > 0) {
        Serial.printf("  5. Server response: %s\n", errorBody.c_str());
      }
    } else if (httpCode == 403) {
      Serial.println("[Firebase] ✗ Forbidden (403)");
      Serial.println("[Firebase] Firebase Rules tidak mengizinkan write");
      Serial.println("[Firebase] Buka Firebase Console > Realtime Database > Rules");
      Serial.println("[Firebase] Update ke: {\"rules\": {\".read\": true, \".write\": true}}");
    } else {
      String errorBody = http.getString();
      Serial.printf("[Firebase] ✗ HTTP Error %d\n", httpCode);
      if (errorBody.length() > 0) {
        Serial.printf("[Firebase] Response: %s\n", errorBody.c_str());
      }
    }
    
    http.end();
  } else {
    Serial.println("[Firebase] ✗ Gagal membuat HTTP POST request");
    Serial.println("[Firebase] Check database URL dan WiFi connection");
  }
}

// Fungsi untuk memverifikasi UID ke Google Sheets dan menerima nama siswa
ServerResponse checkAndLogToGoogle(const String& date, const String& time, const String& uid) {
  ServerResponse responseData = {false, "Tidak Dikenal", "Ditolak"};

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Tidak terhubung. Pengecekan cloud dibatalkan.");
    return responseData;
  }

  // Susun URL permintaan dengan query parameter UID, Date, dan Time
  String url = String(GOOGLE_SCRIPT_URL) + "?uid=" + uid + "&date=" + date + "&time=" + time;
  url.replace(" ", "%20"); 

  Serial.println("[HTTP] Menghubungi Google Sheets Database...");

#ifdef WOKWI_SIMULATION
  // ── MODE SIMULASI WOKWI ──────────────────────────────────────────────────
  // HTTPS ke script.google.com tidak didukung di Wokwi VS Code extension.
  // Mock response digunakan agar alur UI/LED/SD bisa ditest sepenuhnya.
  // Hapus define WOKWI_SIMULATION saat deploy ke hardware asli.
  Serial.println("[SIM] Mock response aktif (HTTPS tidak didukung di Wokwi).");
  responseData.isRegistered = true;
  responseData.name = "Siswa Simulasi";
  responseData.status = "Hadir";
  return responseData;
  // ────────────────────────────────────────────────────────────────────────
#endif


  WiFiClientSecure client;
  client.setInsecure(); // Lewati verifikasi sertifikat SSL (diperlukan untuk script.google.com)
  
  HTTPClient http;
  http.begin(client, url);
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

// ════════════════════════════════════════════════════════════════════════════
// FUNGSI DISPATCH: Router untuk memilih metode penyimpanan
// ════════════════════════════════════════════════════════════════════════════
ServerResponse sendAndVerifyCard(const String& date, const String& time, const String& uid) {
  #if defined(USE_STORAGE_FIREBASE)
    Serial.println("\n[SYSTEM] Storage Mode: FIREBASE");
    return checkAndLogToFirebase(date, time, uid);
  #else
    Serial.println("\n[SYSTEM] Storage Mode: GAS");
    return checkAndLogToGoogle(date, time, uid);
  #endif
}

// Fungsi wrapper untuk menyimpan log
void saveAttendanceLog(const String& timestamp, const String& uid, const String& name, const String& status) {
  #if defined(USE_STORAGE_FIREBASE)
    logToFirebase(timestamp, uid, name, status);
  #else
    logToSDCard(timestamp, uid, name, status);
  #endif
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
  #if defined(USE_STORAGE_FIREBASE)
    Serial.println("[System] Storage Mode: FIREBASE");
  #else
    Serial.println("[System] Storage Mode: GAS");
  #endif

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // Tes Kedip LED
  digitalWrite(PIN_LED_GREEN, HIGH); delay(300); digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);   delay(300); digitalWrite(PIN_LED_RED, LOW);

  // Inisialisasi WiFi
  Serial.print("[WiFi] Menghubungkan ke "); Serial.println(WIFI_SSID);
  // Set DNS custom (Google + Cloudflare) — tetap pakai DHCP untuk IP/gateway
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8,8,8,8), IPAddress(1,1,1,1));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 15) {
    delay(500); Serial.print("."); retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Terhubung! IP: " + WiFi.localIP().toString());
  }else {
    Serial.println("\n[WiFi] GAGAL KONEK! Cek SSID/Pass");
  }

  // ─── Inisialisasi Firebase jika diaktifkan ───────────────────────────────
  #if defined(USE_STORAGE_FIREBASE)
    if (WiFi.status() == WL_CONNECTED) {
      initializeFirebase();
    } else {
      Serial.println("[Firebase] Menunggu WiFi terhubung...");
    }
  #endif
  // ────────────────────────────────────────────────────────────────────────

  // Inisialisasi I2C & OLED
  Serial.print("[OLED] Inisialisasi... ");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("GAGAL! Cek kabel SDA SCL & Alamat 0x3C/0x3D");
  // display mati tapi program tetap jalan
  } else {
    Serial.println("BERHASIL");
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

  // Kirim UID ke cloud (Firebase atau Google Sheets) & dapatkan data siswa
  ServerResponse res = sendAndVerifyCard(String(dateBuf), String(timeBuf), scannedUID);

  // Tampilkan hasil berdasarkan respon cloud
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

    saveAttendanceLog(timeStampBuf, scannedUID, res.name, "Hadir");

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

    saveAttendanceLog(timeStampBuf, scannedUID, "N/A", "Ditolak");

    delay(2500);
    digitalWrite(PIN_LED_RED, LOW);
  }
}