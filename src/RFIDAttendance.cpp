#include "RFIDAttendance.h"
#include "UserRegistry.h"

ServerResponse sendAndVerifyCard(const String& uid) {
  ServerResponse responseData = {false, "Tidak Dikenal", "Ditolak"};
  DateTime now = rtcReady ? rtc.now() : DateTime(2026,9,8,12,0,0);
  char ts[25];
  snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  // Look up user from registry
  UserRecord user;
  if (findUser(uid, user)) {
    responseData.isRegistered = true;
    responseData.name = user.name;
    responseData.status = "Hadir";
  }
  
  return responseData;
}

void logToSDCard(const String& ts, const String& uid, const String& name, const String& status) {
  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS, LOW);
  if (!sdCardReady) { digitalWrite(PIN_SD_CS, HIGH); return; }
  File f = SD.open("/absensi.csv", FILE_APPEND);
  if (f) { f.print(ts + "," + uid + "," + name + "," + status + "\n"); f.close(); }
  digitalWrite(PIN_SD_CS, HIGH);
}

void saveAttendanceLog(const String& ts, const String& uid, const String& name, const String& status) {
  logToSDCard(ts, uid, name, status);
}

void initRFID() {
  pinMode(PIN_RFID_SS, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS, HIGH);
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  digitalWrite(PIN_RFID_SS, LOW);
  rfid.PCD_Init();
  digitalWrite(PIN_RFID_SS, HIGH);
  Serial.println("[RFID] Initialized");
}

void initSDCard() {
  digitalWrite(PIN_SD_CS, LOW);
  if (SD.begin(PIN_SD_CS)) {
    sdCardReady = true;
    if (!SD.exists("/absensi.csv")) {
      File f = SD.open("/absensi.csv", FILE_WRITE);
      if (f) { f.println("Waktu,UID,Nama,Status"); f.close(); }
    }
    Serial.println("[SD] Card ready");
  } else {
    Serial.println("[SD] Card failed or not present");
  }
  digitalWrite(PIN_SD_CS, HIGH);
}

void processRFID() {
  if (WiFi.status() != WL_CONNECTED || wifiProvisioningMode) return;
  
  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_RFID_SS, LOW);
  
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    digitalWrite(PIN_RFID_SS, HIGH);
    return;
  }
  
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  digitalWrite(PIN_RFID_SS, HIGH);
  
  Serial.println("\n[RFID] UID: " + uid);
  
  DateTime now = rtcReady ? rtc.now() : DateTime(2026,9,8,12,0,0);
  char ts[25];
  snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  ServerResponse res = sendAndVerifyCard(uid);
  if (res.isRegistered) {
    digitalWrite(PIN_LED_GREEN, HIGH);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0); display.println("Selamat datang!");
    display.setCursor(0,28); display.println(res.name);
    display.setCursor(0,44); display.println("Absen berhasil!");
    display.display();
    saveAttendanceLog(String(ts), uid, res.name, "Hadir");
    delay(2500);
    digitalWrite(PIN_LED_GREEN, LOW);
  } else {
    digitalWrite(PIN_LED_RED, HIGH);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0); display.println("AKSES DITOLAK!");
    display.setCursor(0,28); display.println("UID: " + uid);
    display.display();
    saveAttendanceLog(String(ts), uid, "N/A", "Ditolak");
    delay(2500);
    digitalWrite(PIN_LED_RED, LOW);
  }
}