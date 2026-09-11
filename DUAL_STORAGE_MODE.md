# Panduan Sistem Penyimpanan Dual Mode
## ESP32 RFID Attendance System

Sistem absensi RFID ini sekarang mendukung **dua opsi penyimpanan data** yang dapat dipilih melalui konfigurasi variabel di awal kode.

---

## 📋 Daftar Isi
1. [Konfigurasi Mode](#konfigurasi-mode)
2. [Mode 1: Google Apps Script (GAS)](#mode-1-google-apps-script-gas)
3. [Mode 2: Firebase Realtime Database](#mode-2-firebase-realtime-database)
4. [Switching Antar Mode](#switching-antar-mode)
5. [Troubleshooting](#troubleshooting)

---

## 🔧 Konfigurasi Mode

### Lokasi Pengaturan
File: `src/main.cpp` (Baris 18-19)

```cpp
// Ubah ke "FIREBASE" untuk menggunakan Firebase Realtime Database
// atau "GAS" untuk menggunakan Google Apps Script (default)
#define STORAGE_MODE "GAS"
```

### Cara Memilih Mode

**Untuk menggunakan Google Apps Script:**
```cpp
#define STORAGE_MODE "GAS"
```

**Untuk menggunakan Firebase Realtime Database:**
```cpp
#define STORAGE_MODE "FIREBASE"
```

---

## 📊 Mode 1: Google Apps Script (GAS)

### Karakteristik
- ✅ **Default mode** - tidak perlu konfigurasi tambahan
- ✅ Menyimpan data langsung ke Google Sheets
- ✅ Respon otomatis untuk verifikasi kartu
- ✅ Kompatibel dengan Wokwi simulator
- ⚠️ Memerlukan deployment Web App Google Apps Script

### Setup Google Apps Script

1. **Buat Google Apps Script Project:**
   - Buka [script.google.com](https://script.google.com)
   - Klik "New project"
   - Rename menjadi "ESP32 Attendance"

2. **Buat Spreadsheet:**
   - Buat Google Sheets baru
   - Rename sheet menjadi "Students" dan "Attendance"
   - Struktur kolom untuk "Students":
     ```
     | UID                | Name      | Registered |
     |--------------------|-----------|------------|
     | 12 34 56 78 AB CD  | Nama Siswa| TRUE       |
     ```
   - Struktur kolom untuk "Attendance":
     ```
     | Waktu               | UID                | Nama      | Status |
     |---------------------|-------------------|-----------|--------|
     | 2026-09-09 10:30:45 | 12 34 56 78 AB CD | Nama Siswa| Hadir  |
     ```

3. **Kode Google Apps Script** (contoh dasar):
   ```javascript
   function doGet(e) {
     const uid = e.parameter.uid;
     const date = e.parameter.date;
     const time = e.parameter.time;
     
     const ss = SpreadsheetApp.getActiveSpreadsheet();
     const studentsSheet = ss.getSheetByName("Students");
     const attendanceSheet = ss.getSheetByName("Attendance");
     
     // Cari UID di Students sheet
     const studentsData = studentsSheet.getDataRange().getValues();
     let found = false;
     let studentName = "Tidak Dikenal";
     
     for (let i = 1; i < studentsData.length; i++) {
       if (studentsData[i][0] === uid) {
         found = true;
         studentName = studentsData[i][1];
         break;
       }
     }
     
     // Catat ke Attendance sheet
     attendanceSheet.appendRow([
       date + " " + time,
       uid,
       studentName,
       found ? "Hadir" : "Ditolak"
     ]);
     
     // Kembalikan respon
     return ContentService.createTextOutput(JSON.stringify({
       registered: found,
       name: studentName,
       status: found ? "Hadir" : "Ditolak"
     })).setMimeType(ContentService.MimeType.JSON);
   }
   ```

4. **Deploy sebagai Web App:**
   - Klik "Deploy" → "New Deployment"
   - Pilih type "Web app"
   - Execute as: pilih akun Anda
   - Who has access: "Anyone"
   - Klik "Deploy"
   - Copy URL deployment

5. **Update secrets.h:**
   ```cpp
   #define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"
   ```

### Kredensial yang Diperlukan (secrets.h)
```cpp
#define WIFI_SSID        "NAMA_WIFI_ANDA"
#define WIFI_PASSWORD    "PASSWORD_WIFI_ANDA"
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/SCRIPT_ID/exec"
```

---

## 🔥 Mode 2: Firebase Realtime Database

### Karakteristik
- ✅ Database realtime NoSQL
- ✅ Akses langsung tanpa Web App perantara
- ✅ Scalable untuk sistem besar
- ✅ API RESTful modern
- ❌ Tidak kompatibel dengan Wokwi simulator
- ⚠️ Memerlukan setup Firebase project

### Setup Firebase

#### Step 1: Buat Firebase Project
1. Buka [Firebase Console](https://console.firebase.google.com/)
2. Klik "Create a new project"
3. Masukkan nama project (contoh: "ESP32-Attendance")
4. Klik "Create project"

#### Step 2: Setup Realtime Database
1. Di sidebar kiri, klik "Realtime Database"
2. Klik "Create Database"
3. Pilih lokasi (contoh: asia-southeast1)
4. Mulai dalam mode "Test" (untuk development)
5. Copy URL database: `https://YOUR_PROJECT_NAME.firebaseio.com`

#### Step 3: Setup Authentication
1. Di sidebar kiri, klik "Authentication"
2. Klik tab "Users"
3. Klik "Create user" atau aktifkan "Anonymous Authentication"
4. Jika menggunakan Email/Password:
   - Email: `rfid@example.com`
   - Password: `SecurePassword123!`

#### Step 4: Dapatkan API Key
1. Klik "Project Settings" (gear icon)
2. Tab "General"
3. Cari "Web API Key"
4. Copy API key tersebut

#### Step 5: Setup Database Structure
Gunakan Firebase Console untuk membuat struktur:

```
{
  "students": {
    "12 34 56 78 AB CD": {
      "registered": true,
      "name": "Nama Siswa",
      "status": "Hadir"
    }
  },
  "attendance": {
    "12 34 56 78 AB CD": {
      "1609459200": {
        "timestamp": "2026-09-09 10:30:45",
        "uid": "12 34 56 78 AB CD",
        "name": "Nama Siswa",
        "status": "Hadir"
      }
    }
  }
}
```

#### Step 6: Setup Firebase Rules (Keamanan)
1. Di "Realtime Database", klik tab "Rules"
2. Gunakan rules berikut untuk development:

```json
{
  "rules": {
    "students": {
      ".read": true,
      ".write": "auth != null"
    },
    "attendance": {
      ".read": "auth != null",
      ".write": "auth != null"
    }
  }
}
```

### Kredensial yang Diperlukan (secrets.h)
```cpp
#define WIFI_SSID           "NAMA_WIFI_ANDA"
#define WIFI_PASSWORD       "PASSWORD_WIFI_ANDA"
#define FIREBASE_API_KEY    "YOUR_WEB_API_KEY"
#define FIREBASE_DATABASE_URL "https://YOUR_PROJECT_NAME.firebaseio.com"
#define FIREBASE_USER_EMAIL "rfid@example.com"
#define FIREBASE_USER_PASSWORD "SecurePassword123!"
```

### Library Dependencies
Pastikan Firebase library sudah di-install via platformio.ini:
```ini
[env:esp32dev]
lib_deps =
    mobizt/Firebase-ESP32
```

---

## 🔄 Switching Antar Mode

### Procedure Lengkap

1. **Edit src/main.cpp:**
   ```cpp
   // Baris 18-19, ubah:
   #define STORAGE_MODE "GAS"      // ke "FIREBASE"
   // menjadi:
   #define STORAGE_MODE "FIREBASE"
   ```

2. **Update secrets.h** (jika belum ada):
   - Salin dari `include/secrets.h.example`
   - Isi kredensial yang sesuai dengan mode pilihan
   - Pastikan tidak commit ke Git

3. **Build dan Upload:**
   ```bash
   pio run -t upload -e esp32dev
   ```

4. **Monitor Serial Output:**
   ```bash
   pio device monitor -b 115200
   ```

### Verifikasi Mode Aktif
Lihat output serial saat startup:
```
[System] Storage Mode: GAS        # atau FIREBASE
[WiFi] Terhubung! IP: 192.168.x.x
[Firebase] Inisialisasi Firebase...  # hanya saat FIREBASE mode
```

---

## 📝 Struktur Fungsi

### Fungsi Dispatch (Router)
```cpp
// Menerima data dari RFID scan dan mengarahkan ke backend yang sesuai
ServerResponse sendAndVerifyCard(const String& date, const String& time, const String& uid) {
  #if defined(STORAGE_MODE) && STORAGE_MODE == "FIREBASE"
    return checkAndLogToFirebase(date, time, uid);
  #else
    return checkAndLogToGoogle(date, time, uid);
  #endif
}
```

### Fungsi Penyimpanan (Wrapper)
```cpp
// Menyimpan log ke backend yang dipilih
void saveAttendanceLog(const String& timestamp, const String& uid, const String& name, const String& status) {
  #if defined(STORAGE_MODE) && STORAGE_MODE == "FIREBASE"
    logToFirebase(timestamp, uid, name, status);
  #else
    logToSDCard(timestamp, uid, name, status);
  #endif
}
```

### Alur Pemrosesan
```
RFID Card Scan
    ↓
Format Data (UID, Date, Time)
    ↓
Panggil sendAndVerifyCard()
    ↓
    ├─→ Jika STORAGE_MODE = "FIREBASE" → checkAndLogToFirebase()
    │   └─→ Query /students/{uid}
    │   └─→ Return response
    │
    └─→ Jika STORAGE_MODE = "GAS" → checkAndLogToGoogle()
        └─→ Call Google Apps Script
        └─→ Return response
    ↓
Check isRegistered
    ├─→ True → LED Hijau, Tampil Nama, Save Log
    └─→ False → LED Merah, Tampil "Ditolak"
    ↓
Call saveAttendanceLog()
    ├─→ Firebase Mode → logToFirebase()
    └─→ GAS Mode → logToSDCard()
```

---

## ⚙️ Perbandingan Fitur

| Fitur | Google Apps Script | Firebase |
|-------|-------------------|----------|
| Setup Mudah | ✅ Mudah | ⚠️ Medium |
| Real-time | ❌ Delayed | ✅ Instant |
| Scalability | ⚠️ Limited | ✅ Unlimited |
| Wokwi Compatible | ✅ Yes | ❌ No |
| Biaya | ✅ Gratis | ✅ Gratis (quota) |
| Kompleksitas Maintenance | ✅ Sederhana | ⚠️ Medium |
| Verifikasi Offline | ❌ Tidak | ❌ Tidak |

---

## 🐛 Troubleshooting

### Firebase Mode - Error Koneksi
**Problem:** `[Firebase] GAGAL terhubung!`

**Solusi:**
- Pastikan WiFi terhubung (cek output `[WiFi] Terhubung`)
- Verifikasi API Key dan Database URL di secrets.h
- Pastikan email/password Firebase benar
- Cek Firebase Rules memperbolehkan akses

### Firebase Mode - Timeout
**Problem:** `Firebase.begin()` timeout

**Solusi:**
- Kurangi `max_token_generation_retry` di code
- Pastikan Firebase project aktif
- Cek koneksi internet stabil

### GAS Mode - No Response
**Problem:** Kartu ditap tapi tidak ada respon

**Solusi:**
- Verifikasi Google Script URL di secrets.h
- Cek Google Sheets dapat diakses
- Deploy ulang Google Apps Script
- Monitor serial untuk error HTTP code

### SD Card Logging
**Catatan:** 
- Mode GAS: Log ke SD Card + Google Sheets
- Mode Firebase: Log hanya ke Firebase (bisa tambah SD Card manual)
- Untuk menambah SD Card di Firebase mode, uncomment `logToSDCard()` di `saveAttendanceLog()`

---

## 📚 Referensi

### Library Firebase-ESP32
- Dokumentasi: [GitHub mobizt/Firebase-ESP32](https://github.com/mobizt/Firebase-ESP32)
- Library Manager: Cari "Firebase-ESP32"

### Google Apps Script
- Dokumentasi: [script.google.com/docs](https://developers.google.com/apps-script)
- REST API: [Google Apps Script API](https://developers.google.com/apps-script/api)

### Firebase Console
- URL: https://console.firebase.google.com/
- Dokumentasi: https://firebase.google.com/docs

---

## 💡 Tips & Best Practices

1. **Production Deployment:**
   - Ubah Firebase Rules dari "Test" ke restrictive rules
   - Gunakan App Check untuk keamanan tambahan
   - Setup Service Account untuk otomasi backend

2. **Monitoring:**
   - Gunakan Firebase Console untuk melihat real-time data
   - Aktifkan logging di Serial Monitor
   - Setup alerts untuk error

3. **Development vs Production:**
   - Development: Gunakan `WOKWI_SIMULATION` untuk test tanpa cloud
   - Production: Disable `WOKWI_SIMULATION` dan gunakan hardware asli

4. **Backup Data:**
   - GAS mode: Export Google Sheets ke CSV/Excel
   - Firebase mode: Download backup via Firebase Console

---

## 📞 Support

Untuk pertanyaan atau error, check:
1. Serial Monitor output
2. Firebase Console (jika Firebase mode)
3. Google Sheets & Apps Script dashboard (jika GAS mode)
4. Dokumentasi library di GitHub

---

**Last Updated:** September 2026
**Version:** 2.0 (Dual Storage Mode)
