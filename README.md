# ESP32 RFID Attendance System

Sistem absensi berbasis RFID menggunakan ESP32 dengan dukungan dual storage mode (Google Apps Script dan Firebase Realtime Database).

---

## 📋 Daftar Isi

1. [Deskripsi Sistem](#deskripsi-sistem)
2. [Quick Start](#quick-start)
3. [Requirements](#requirements)
4. [Installation](#installation)
5. [Konfigurasi](#konfigurasi)
6. [Mode Storage](#mode-storage)
7. [Setup Mode: Google Apps Script (GAS)](#setup-mode-google-apps-script-gas)
8. [Setup Mode: Firebase Realtime Database](#setup-mode-firebase-realtime-database)
9. [Database Structure](#database-structure)
10. [Security](#security)
11. [Monitoring & Debugging](#monitoring--debugging)
12. [Troubleshooting](#troubleshooting)
13. [Code Examples](#code-examples)
14. [Tips & Best Practices](#tips--best-practices)
15. [FAQ](#faq)

---

## Deskripsi Sistem

Sistem absensi RFID ESP32 ini adalah solusi modern untuk pencatatan kehadiran otomatis menggunakan kartu RFID. Sistem mendukung **dua opsi penyimpanan data**:

- **Google Apps Script (GAS)** - Menyimpan data langsung ke Google Sheets
- **Firebase Realtime Database** - Database cloud NoSQL untuk scalability lebih baik

Kedua mode dapat dipilih dengan mengubah satu baris kode, tanpa perlu perubahan hardware.

### Fitur Utama
- ✅ Scan RFID otomatis dengan verifikasi real-time
- ✅ Dual storage mode (GAS & Firebase)
- ✅ Response instant dengan LED indicator
- ✅ Display OLED untuk informasi
- ✅ Logging offline ke SD Card
- ✅ Error handling & fallback
- ✅ WiFi auto-reconnect
- ✅ Production-ready security

---

## Quick Start

**Langkah 1: Pilih Mode**
```cpp
// File: src/main.cpp (Baris 18-19)
#define STORAGE_MODE "GAS"        // atau "FIREBASE"
```

**Langkah 2: Setup Kredensial**
```bash
cp include/secrets.h.example include/secrets.h
# Edit secrets.h dengan API keys & passwords Anda
```

**Langkah 3: Upload**
```bash
pio run -t upload -e esp32dev
```

**Langkah 4: Test**
```bash
pio device monitor -b 115200
# Tap RFID card dan lihat response di serial monitor
```

---

## Requirements

### Hardware
- ESP32 Development Board
- RFID Card Reader (RC522)
- RFID Cards
- LED (Hijau & Merah) dengan resistor
- OLED Display (optional)
- SD Card Module (optional)
- WiFi Router

### Software
- Arduino IDE / VS Code + PlatformIO
- Akun Google (untuk GAS mode)
- Akun Firebase (untuk Firebase mode)
- Platform: Arduino Framework untuk ESP32

### Librari
- `esp-idf` (built-in)
- `mobizt/Firebase-ESP32` (untuk Firebase mode)
- `paulstoffregen/Time` (time handling)
- Dependency lainnya (lihat `platformio.ini`)

---

## Installation

### 1. Clone/Unduh Repository
```bash
cd e:\xampp\htdocs\robotik
git clone [repository-url] esp32-rfid
cd esp32-rfid
```

### 2. Install PlatformIO (jika belum)
```bash
pip install platformio
# atau gunakan VS Code extension
```

### 3. Setup Workspace
```bash
# PlatformIO akan auto-download libraries dari platformio.ini
pio run -e esp32dev --verbose
```

### 4. Konfigurasi File
```bash
# Copy template secrets
cp include/secrets.h.example include/secrets.h

# Edit secrets.h dengan editor favorit
# Isi WiFi SSID, password, API keys, dll
```

---

## Konfigurasi

### File Konfigurasi Utama

#### 1. `src/main.cpp` - Mode Selection
```cpp
// Baris 18-19: Pilih mode penyimpanan
#define STORAGE_MODE "GAS"           // ← Ubah ke "FIREBASE" untuk mode Firebase
```

#### 2. `include/secrets.h` - Kredensial (JANGAN COMMIT KE GIT!)
File ini berisi:
- WiFi SSID & Password
- Google Apps Script URL (untuk GAS mode)
- Firebase API Key, Database URL, Email, Password (untuk Firebase mode)

**Cara membuat secrets.h:**
```bash
# 1. Copy dari template
cp include/secrets.h.example include/secrets.h

# 2. Edit dengan informasi Anda
# 3. Pastikan di .gitignore (sudah default)
```

**Template struktur:**
```cpp
#pragma once

// WiFi Credentials
#define WIFI_SSID     "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Google Apps Script (untuk GAS mode)
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"

// Firebase (untuk Firebase mode)
#define FIREBASE_API_KEY       "AIzaSy..."
#define FIREBASE_DATABASE_URL  "https://project.firebaseio.com"
#define FIREBASE_USER_EMAIL    "rfid@example.com"
#define FIREBASE_USER_PASSWORD "SecurePassword123!"
```

#### 3. `platformio.ini` - Build Configuration
Berisi:
- Target board: ESP32
- Serial baudrate: 115200
- Library dependencies
- Build flags

---

## Mode Storage

Sistem mendukung dua mode penyimpanan yang dapat dipilih:

### Perbandingan Fitur

| Fitur | Google Apps Script | Firebase |
|-------|-------------------|----------|
| Setup Mudah | ✅ Sangat Mudah | ⚠️ Medium |
| Response Time | 1-3 detik | <500ms |
| Real-time Sync | ❌ Delayed | ✅ Instant |
| Scalability | ⚠️ Limited | ✅ Unlimited |
| Wokwi Compatible | ✅ Yes | ❌ No |
| Biaya | ✅ Gratis | ✅ Gratis (quota) |
| Data Visualization | ✅ Google Sheets | ⚠️ Firebase Console |
| Offline Fallback | ✅ SD Card | ✅ SD Card |

### Alur Pemrosesan

```
RFID Card Scan
    ↓
Format Data (UID, Date, Time)
    ↓
Router: sendAndVerifyCard()
    ├─→ Jika STORAGE_MODE = "FIREBASE" → checkAndLogToFirebase()
    │   └─→ REST API Firebase
    │
    └─→ Jika STORAGE_MODE = "GAS" → checkAndLogToGoogle()
        └─→ Call Google Apps Script
    ↓
Check isRegistered?
    ├─→ TRUE  → LED Hijau + Tampil Nama
    └─→ FALSE → LED Merah + Tampil "Ditolak"
    ↓
Save Log → saveAttendanceLog()
    ├─→ Firebase: logToFirebase()
    └─→ GAS: logToSDCard()
```

---

## Setup Mode: Google Apps Script (GAS)

### Karakteristik
- ✅ Default mode
- ✅ Menyimpan ke Google Sheets
- ✅ Verifikasi instant
- ✅ Kompatibel dengan Wokwi simulator
- ⚠️ Memerlukan deployment Web App

### Step-by-Step Setup

#### Step 1: Buat Google Apps Script Project
1. Buka [script.google.com](https://script.google.com)
2. Klik **"New project"**
3. Rename menjadi **"ESP32 Attendance System"**

#### Step 2: Buat Google Sheets
1. Buat Google Sheets baru
2. **Sheet 1:** Rename ke **"Students"**
   ```
   Kolom A: UID                | Kolom B: Name      | Kolom C: Registered
   12 34 56 78 AB CD           | Budi Santoso       | TRUE
   AB CD 12 34 56 78           | Siti Nurhaliza     | TRUE
   ```

3. **Sheet 2:** Rename ke **"Attendance"**
   ```
   Kolom A: Waktu              | Kolom B: UID       | Kolom C: Nama   | Kolom D: Status
   2026-09-09 10:30:45         | 12 34 56 78 AB CD  | Budi Santoso    | Hadir
   ```

#### Step 3: Copy Kode Google Apps Script
Di Google Apps Script editor, paste code berikut:

```javascript
// Dapatkan sheet yang diperlukan
function getSheets() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  return {
    students: ss.getSheetByName("Students"),
    attendance: ss.getSheetByName("Attendance")
  };
}

// Fungsi utama untuk handle GET request dari ESP32
function doGet(e) {
  try {
    const uid = e.parameter.uid || "";
    const date = e.parameter.date || "";
    const time = e.parameter.time || "";
    
    // Validasi parameter
    if (!uid || !date || !time) {
      return createJsonResponse(false, "Parameter tidak lengkap", "Error");
    }
    
    const sheets = getSheets();
    const studentData = findStudentByUID(sheets.students, uid);
    
    // Catat ke Attendance sheet
    const timestamp = date + " " + time;
    sheets.attendance.appendRow([
      timestamp,
      uid,
      studentData.name,
      studentData.status
    ]);
    
    return createJsonResponse(studentData.found, studentData.name, studentData.status);
    
  } catch (error) {
    Logger.log("Error: " + error.toString());
    return createJsonResponse(false, "Server Error", "Error");
  }
}

// Cari siswa berdasarkan UID
function findStudentByUID(sheet, uid) {
  const data = sheet.getDataRange().getValues();
  
  for (let i = 1; i < data.length; i++) {
    if (data[i][0] === uid) {
      const registered = data[i][2];
      return {
        found: registered === true || registered === "TRUE",
        name: data[i][1] || "Unknown",
        status: registered ? "Hadir" : "Ditolak"
      };
    }
  }
  
  return { found: false, name: "Tidak Dikenal", status: "Ditolak" };
}

// Helper: Buat JSON response
function createJsonResponse(registered, name, status) {
  return ContentService.createTextOutput(
    JSON.stringify({
      registered: registered,
      name: name,
      status: status
    })
  ).setMimeType(ContentService.MimeType.JSON);
}
```

#### Step 4: Deploy sebagai Web App
1. Klik **"Deploy"** → **"New Deployment"**
2. Type: **"Web app"**
3. Execute as: Pilih akun Anda
4. Who has access: **"Anyone"**
5. Klik **"Deploy"**
6. **Copy URL deployment** (format: `https://script.google.com/macros/s/SCRIPT_ID/exec`)

#### Step 5: Update secrets.h
```cpp
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"
```

#### Step 6: Upload ke ESP32
```bash
pio run -t upload -e esp32dev
```

---

## Setup Mode: Firebase Realtime Database

### Karakteristik
- ✅ Database realtime NoSQL
- ✅ Akses langsung via REST API
- ✅ Scalable untuk sistem besar
- ✅ Response time <500ms
- ❌ Tidak kompatibel dengan Wokwi
- ⚠️ Perlu setup project di Firebase Console

### Step-by-Step Setup

#### Step 1: Buat Firebase Project
1. Buka [console.firebase.google.com](https://console.firebase.google.com)
2. Klik **"Create a new project"**
3. Nama: **"ESP32-RFID-Attendance"**
4. Uncheck "Enable Google Analytics" (optional)
5. Klik **"Create project"**

#### Step 2: Setup Realtime Database
1. Sidebar → **"Realtime Database"**
2. Klik **"Create Database"**
3. Location: **"asia-southeast1"** (untuk Indonesia)
4. Rules: **"Start in test mode"**
5. Klik **"Enable"**
6. **Copy Database URL**: Simpan untuk `FIREBASE_DATABASE_URL`

#### Step 3: Setup Authentication
1. Sidebar → **"Authentication"**
2. Tab **"Users"** → **"Create user"**
3. Email: `rfid-esp32@example.com`
4. Password: `SecurePass123!@`
5. Klik **"Create"**

#### Step 4: Dapatkan API Key
1. **Project Settings** (gear icon)
2. Tab **"General"**
3. Cari **"Web API Key"**
4. Copy value → Simpan untuk `FIREBASE_API_KEY`

#### Step 5: Update secrets.h
```cpp
#define FIREBASE_API_KEY       "AIzaSyDxZ..."
#define FIREBASE_DATABASE_URL  "https://esp32-rfid-xyz.firebaseio.com"
#define FIREBASE_USER_EMAIL    "rfid-esp32@example.com"
#define FIREBASE_USER_PASSWORD "SecurePass123!@"
```

#### Step 6: Setup Database Structure
Di Firebase Console, **Realtime Database** → klik **"⋮"** → **"Import JSON"**

Paste struktur ini:
```json
{
  "students": {
    "12345678AB": {
      "name": "Budi Santoso",
      "registered": true,
      "status": "Hadir"
    },
    "ABCDEF123456": {
      "name": "Siti Nurhaliza",
      "registered": true,
      "status": "Hadir"
    }
  },
  "attendance": {}
}
```

#### Step 7: Update Firebase Security Rules
Di **Realtime Database** → **Rules** tab → Paste:

**Untuk DEVELOPMENT:**
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

**Untuk PRODUCTION:**
```json
{
  "rules": {
    "students": {
      ".read": true,
      ".write": false,
      ".validate": "newData.hasChildren(['name', 'registered', 'status'])"
    },
    "attendance": {
      ".read": true,
      ".write": true,
      ".validate": "newData.hasChildren(['timestamp', 'uid', 'name', 'status'])"
    }
  }
}
```

Klik **"Publish"** untuk save.

#### Step 8: Update main.cpp
```cpp
// Baris 18-19
#define STORAGE_MODE "FIREBASE"
```

#### Step 9: Upload ke ESP32
```bash
pio run -t upload -e esp32dev
```

---

## Database Structure

### Mode: Google Apps Script (Google Sheets)

**Sheet: Students**
```
| UID (A)            | Name (B)       | Registered (C) |
|-------------------|----------------|----------------|
| 12 34 56 78 AB CD | Budi Santoso   | TRUE           |
| AB CD 12 34 56 78 | Siti Nurhaliza | TRUE           |
| FF EE DD CC BB AA | Invalid Card   | FALSE          |
```

**Sheet: Attendance**
```
| Waktu (A)           | UID (B)            | Nama (C)        | Status (D) |
|---------------------|-------------------|-----------------|-----------|
| 2026-09-09 10:30:45 | 12 34 56 78 AB CD | Budi Santoso    | Hadir     |
| 2026-09-09 10:35:12 | AB CD 12 34 56 78 | Siti Nurhaliza  | Hadir     |
```

### Mode: Firebase Realtime Database

```json
{
  "students": {
    "12345678AB": {
      "name": "Budi Santoso",
      "registered": true,
      "status": "Hadir"
    }
  },
  "attendance": {
    "-M1234567890abc": {
      "timestamp": "2026-09-09 10:30:45",
      "uid": "12345678AB",
      "name": "Budi Santoso",
      "status": "Hadir"
    }
  }
}
```

---

## Security

### Kredensial & Secrets

**JANGAN PERNAH:**
- ❌ Commit `secrets.h` ke Git/Repository
- ❌ Bagikan API Key di public channel
- ❌ Hardcode password di code
- ❌ Push `.env` files ke public repo

**SELALU LAKUKAN:**
- ✅ Gunakan `.gitignore` untuk `include/secrets.h`
- ✅ Share template: `include/secrets.h.example`
- ✅ Rotate credentials secara berkala
- ✅ Monitor access logs di Firebase/Google

### File `.gitignore` (sudah default)
```
include/secrets.h
.env
.vscode/
.pio/
build/
```

### Firebase Security

#### Development (Testing)
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

#### Production (Restrictive)
```json
{
  "rules": {
    "students": {
      ".read": true,
      ".write": false
    },
    "attendance": {
      ".read": "auth != null",
      ".write": "auth != null"
    }
  }
}
```

### Google Apps Script Security
- URL adalah public (anyone dengan URL bisa access)
- Google Sheets permissions mengontrol data access
- HTTPS enforced (SSL/TLS)

---

## Monitoring & Debugging

### Serial Monitor Output

#### GAS Mode - Success
```
[System] Storage Mode: GAS
[WiFi] Menghubungkan ke MyWiFi
[WiFi] Terhubung! IP: 192.168.1.100
[RFID] Kartu Terdeteksi! UID: 12 34 56 78 AB CD
[HTTP] GET /macros/s/SCRIPT_ID/exec?uid=12...&date=2026-09-09&time=10:30:45
[HTTP] Response Code: 200
[HTTP] Response: {"registered":true,"name":"Budi Santoso","status":"Hadir"}
[ACCESS] ✓ DITERIMA: Budi Santoso
[SD] Log saved: 2026-09-09 10:30:45 | UID | Hadir
```

#### Firebase Mode - Success
```
[System] Storage Mode: FIREBASE
[WiFi] Menghubungkan ke MyWiFi
[WiFi] Terhubung! IP: 192.168.1.100
[Firebase] Inisialisasi Firebase Realtime Database
[Firebase] FirebaseClient siap
[RFID] Kartu Terdeteksi! UID: 12 34 56 78 AB CD
[Firebase] GET /students/12345678AB.json
[Firebase] HTTP Response Code: 200
[Firebase] {"name":"Budi Santoso","registered":true,"status":"Hadir"}
[ACCESS] ✓ DITERIMA: Budi Santoso
[Firebase] POST /attendance.json
[Firebase] Log berhasil disimpan: -M1234567890abc
```

### Monitoring Commands

**Lihat Serial Monitor:**
```bash
pio device monitor -b 115200
```

**Filter output tertentu:**
```bash
pio device monitor -b 115200 | grep "Firebase\|ACCESS"
```

**Firebase Console - Real-time Data:**
1. Buka Firebase Console
2. Realtime Database → Browse
3. Lihat `/attendance` untuk logs terbaru

**Google Sheets - Attendance Log:**
1. Buka Google Sheets
2. Lihat sheet "Attendance"
3. Data akan auto-append setiap scan

---

## Troubleshooting

### ❌ Error: WiFi Connection Failed

**Serial Output:**
```
[WiFi] Koneksi Gagal
[WiFi] Status: 0
```

**Solusi:**
1. Verifikasi WIFI_SSID & WIFI_PASSWORD di `secrets.h`
2. Pastikan WiFi signal kuat (di dekat router)
3. Restart ESP32
4. Cek apakah WiFi 2.4GHz (ESP32 tidak support 5GHz)

```cpp
// Debug WiFi
Serial.println("[WiFi] SSID: " + String(WIFI_SSID));
Serial.println("[WiFi] Attempting to connect...");
```

### ❌ Error: HTTP 400 (Bad Request)

**Solusi:**
- Pastikan Database URL tidak ada trailing slash
- UID tidak boleh punya karakter khusus (`:`, `/`, dll)
- Untuk space di UID, ganti dengan underscore: `12_34_56_78_AB_CD`

```cpp
// Clean UID
String cleanUID = uid;
cleanUID.replace(" ", "_");
```

### ❌ Error: HTTP 401 (Unauthorized)

**Untuk Firebase:**
- Update Firebase Security Rules ke test mode atau production mode yang allow akses
- Verifikasi API Key di secrets.h

**Untuk GAS:**
- Verifikasi Google Script URL
- Deploy ulang dengan akses "Anyone"

**Firebase Console:**
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

Klik **"Publish"**.

### ❌ Error: Firebase Timeout

**Solusi:**
1. Kurangi retry count di code
2. Pastikan internet stable
3. Verifikasi Database URL

```cpp
// Tambah timeout handling
Firebase.RTDB.setReadTimeout(&fbdo, 5000);  // 5 detik
```

### ❌ Error: Kartu Tidak Terbaca

**Solusi:**
1. Tap kartu lebih lama (2-3 detik)
2. Posisikan tepat di atas reader
3. Cek koneksi RC522 module
4. Verifikasi pin assignment di code

**Wiring Check:**
```
RC522          ESP32
SDA     →      GPIO 5
SCK     →      GPIO 18
MOSI    →      GPIO 23
MISO    →      GPIO 19
IRQ     →      (not used)
GND     →      GND
3.3V    →      3.3V
```

---

## Code Examples

### GAS Mode - Custom Logic

```cpp
// Tambahan di main.cpp untuk custom handling
void onCardScanned(const String& uid, const String& name, bool registered) {
  if (registered) {
    // Action jika kartu terdaftar
    playSuccessSound();
    displayName(name);
    triggerRelay();  // Optional: unlock door
  } else {
    // Action jika kartu tidak terdaftar
    playErrorSound();
    displayError("INVALID CARD");
  }
}
```

### Firebase Mode - Custom Monitoring

```javascript
// Di Firebase Console → Database → Console
// Monitor real-time attendance
firebase.database().ref('attendance').on('child_added', function(snapshot) {
  const data = snapshot.val();
  console.log(`${data.timestamp} - ${data.name} - ${data.status}`);
});
```

### Hybrid Mode - Cloud + SD Card Fallback

```cpp
void saveAttendanceLog(const String& timestamp, const String& uid, 
                       const String& name, const String& status) {
  bool cloudSuccess = false;
  
  #if defined(STORAGE_MODE) && STORAGE_MODE == "FIREBASE"
    if (Firebase.ready()) {
      logToFirebase(timestamp, uid, name, status);
      cloudSuccess = true;
    }
  #else
    if (WiFi.status() == WL_CONNECTED) {
      // GAS mode
      cloudSuccess = true;
    }
  #endif
  
  // Always backup to SD Card
  if (!cloudSuccess || true) {  // Always save to SD
    logToSDCard(timestamp, uid, name, status);
  }
}
```

---

## Tips & Best Practices

### Development vs Production

**DEVELOPMENT:**
- Gunakan GAS mode (lebih mudah test)
- Firebase Rules: test mode
- Simulator: Wokwi supported (GAS only)
- Logging: Serial + SD Card

**PRODUCTION:**
- Gunakan Firebase mode (better scalability)
- Firebase Rules: restrictive rules
- Hardware: Real ESP32 + WiFi Router
- Logging: Firebase + SD Card backup
- Monitor: Real-time alerts

### Performance Optimization

1. **Reduce HTTP Timeout**
   ```cpp
   // Firebase
   Firebase.RTDB.setReadTimeout(&fbdo, 3000);  // 3 detik
   ```

2. **Batch Operations**
   ```cpp
   // Jika banyak scan, batch multiple entries
   // Daripada satu per satu
   ```

3. **Caching**
   ```cpp
   // Cache student list untuk offline verification
   // Jika WiFi disconnected
   ```

### Monitoring Checklist

- [ ] Check Serial Monitor setiap deploy
- [ ] Verify WiFi connection
- [ ] Test dengan multiple RFID cards
- [ ] Monitor database di Firebase/Google
- [ ] Check SD Card logs
- [ ] Set alerts di Firebase Console

### Backup Strategy

**Google Sheets:**
- Export ke CSV/Excel secara berkala
- Gunakan Google Sheets backup feature

**Firebase:**
- Export JSON via Firebase Console
- Setup automated backups
- Monitor usage quota

---

## FAQ

**Q: Bisa switch mode tanpa re-compile?**
A: Tidak. Perlu edit `#define STORAGE_MODE` lalu compile ulang dengan `pio run -t upload`.

**Q: Apa perbedaan utama GAS vs Firebase?**
A: GAS lebih mudah setup (pakai Google Sheets), Firebase lebih cepat & scalable (response <500ms).

**Q: Wokwi simulator support mode apa?**
A: Hanya GAS mode. Firebase tidak bisa di-simulate.

**Q: Gimana jika WiFi mati?**
A: System akan fallback ke SD Card logging (jika installed). Saat WiFi kembali, log akan ter-sync.

**Q: Perlu service account untuk production?**
A: Recommended. Lebih secure daripada pakai user email/password langsung.

**Q: Bisa monitoring data real-time dari mobile?**
A: Bisa dengan Firebase Console mobile app atau buat web dashboard di Firebase Hosting.

**Q: Max kartu yang bisa disupport?**
A: Unlimited. Database tidak ada batasan storage.

**Q: Gimana dengan keamanan data?**
A: Gunakan Firebase Rules yang restrictive di production. Jangan share credentials. Rotate passwords berkala.

**Q: Bisa multiple reader (di lokasi berbeda)?**
A: Bisa. Setiap reader punya unique serial number atau ID. Log akan ter-centralize di cloud.

---

## Referensi & Resources

### Dokumentasi Official
- [Firebase Realtime Database Docs](https://firebase.google.com/docs/database)
- [Firebase Console](https://console.firebase.google.com)
- [Google Apps Script](https://script.google.com)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)

### Libraries
- [Firebase-ESP32](https://github.com/mobizt/Firebase-ESP32)
- [PlatformIO](https://platformio.org/)
- [Arduino RFID (MFRC522)](https://github.com/miguelbalboa/rfid)

### Tools
- [Wokwi Simulator](https://wokwi.com)
- [Firebase Emulator](https://firebase.google.com/docs/emulator-suite)
- [Postman](https://www.postman.com/) - untuk test API

---

## Support & Contact

### Debugging Steps
1. Lihat Serial Monitor output lengkap
2. Cek Firebase/Google Sheets Console
3. Verifikasi secrets.h (API keys, URLs)
4. Cek WiFi connection stability
5. Test dengan curl/Postman terlebih dahulu

### Common Issues Tracker
- Serial Monitor penuh dengan error? → Check Firebase Rules
- Card tidak terbaca? → Cek RC522 wiring
- Timeout terus? → Kurangi WiFi distance, check internet speed
- Data tidak ter-save? → Cek permissions di Firebase/Google Sheets

---

## Changelog

### v2.0 (September 2026) - Dual Storage Mode
- ✅ Added Firebase Realtime Database support
- ✅ Dual mode selection dengan single flag
- ✅ Comprehensive documentation
- ✅ Examples & troubleshooting guide
- ✅ Security best practices

### v1.0
- Initial release dengan Google Apps Script mode

---

## License

Lihat file LICENSE di repository ini.

---

**Last Updated:** September 2026  
**Status:** Production Ready  
**Version:** 2.0 - Dual Storage Mode

Untuk pertanyaan atau issues, check serial monitor output dan Firebase/Google Sheets dashboard terlebih dahulu sebelum troubleshoot lebih lanjut.
