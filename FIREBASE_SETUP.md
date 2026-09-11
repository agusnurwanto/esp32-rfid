# 🔥 Firebase Setup - Step by Step

Panduan lengkap setup Firebase Realtime Database untuk ESP32 RFID System.

---

## 📋 Prerequisites

- Google Account (untuk Firebase Console)
- ESP32 dengan WiFi
- Arduino IDE atau VS Code + PlatformIO
- Secrets.h sudah siap

---

## ✅ Step-by-Step Setup

### Step 1: Buat Firebase Project

1. Buka [https://console.firebase.google.com/](https://console.firebase.google.com/)
2. Klik **"Create a new project"**
3. **Project name:** `ESP32-RFID-Attendance` (atau nama pilihan Anda)
4. Uncheck "Enable Google Analytics" (opsional)
5. Klik **"Create project"**
6. Tunggu sampai selesai (~1-2 menit)

---

### Step 2: Setup Realtime Database

1. Di Firebase Console, sidebar kiri → **"Realtime Database"**
2. Klik **"Create Database"**
3. **Location:** Pilih yang paling dekat (contoh: `asia-southeast1` untuk Indonesia)
4. **Security rules:** Pilih **"Start in test mode"** (untuk development)
5. Klik **"Enable"**
6. **Copy Database URL** dari halaman Realtime Database:
   ```
   https://PROJECT_ID.firebaseio.com
   ```
   → Simpan ini untuk `FIREBASE_DATABASE_URL` di `secrets.h`

---

### Step 3: Setup Authentication

1. Di sidebar kiri → **"Authentication"**
2. Klik tab **"Users"**
3. Klik **"Create user"**
4. **Email:** `rfid-esp32@example.com`
5. **Password:** `SecurePass123!@` (gunakan password yang kuat)
6. Klik **"Create"**

*(Untuk production, lebih baik gunakan anonymous auth atau service account)*

---

### Step 4: Dapatkan API Key

1. Di sidebar kiri → **"Project Settings"** (gear icon)
2. Klik tab **"General"**
3. Cari section **"Your apps"** atau scroll ke **"Web API Key"**
4. Copy value dari **"Web API Key"**
   → Simpan ini untuk `FIREBASE_API_KEY` di `secrets.h`

---

### Step 5: Update secrets.h

File: `include/secrets.h`

```cpp
#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// WiFi Credentials
#define WIFI_SSID     "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// ─────────────────────────────────────────────────────────────────────────────
// Firebase Realtime Database Credentials
#define FIREBASE_API_KEY       "AIzaSyDxZ..."  // Dari Step 4
#define FIREBASE_DATABASE_URL  "https://esp32-rfid-xyz.firebaseio.com"  // Dari Step 2
#define FIREBASE_USER_EMAIL    "rfid-esp32@example.com"  // Dari Step 3
#define FIREBASE_USER_PASSWORD "SecurePass123!@"  // Dari Step 3

// ─────────────────────────────────────────────────────────────────────────────
// Google Apps Script (opsional, untuk GAS mode)
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_ID/exec"
```

---

### Step 6: Setup Database Structure

Di Firebase Console, Realtime Database tab, klik **"⋮"** (menu) → **"Import JSON"**

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
  }
}
```

*(Ganti UID dengan UID RFID card Anda yang sebenarnya)*

---

### Step 7: Update Firebase Security Rules

**PENTING:** Default test mode akan expired. Ubah ke rules yang sesuai.

Di **Realtime Database → Rules** tab:

**Untuk DEVELOPMENT/TESTING:**
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

**Untuk PRODUCTION (Recommended):**
```json
{
  "rules": {
    "students": {
      ".read": true,
      ".write": false,
      ".validate": "newData.hasChildren(['name', 'registered', 'status'])"
    },
    "attendance": {
      ".read": "auth != null",
      ".write": "auth != null",
      ".validate": "newData.hasChildren(['timestamp', 'uid', 'name', 'status'])"
    }
  }
}
```

Klik **"Publish"** untuk save rules.

---

### Step 8: Configure ESP32 Code

File: `src/main.cpp`

**Pastikan mode Firebase aktif:**
```cpp
// Line 18-19:
// #define USE_STORAGE_GAS
#define USE_STORAGE_FIREBASE    // ← Uncomment ini
```

---

### Step 9: Build & Upload

```bash
# Terminal
cd C:\xampp\htdocs\IoT\esp32-rfid

# Build
pio run -e esp32dev

# Upload
pio run -t upload -e esp32dev

# Monitor Serial
pio device monitor -b 115200
```

---

### Step 10: Test Koneksi

Di Serial Monitor, Anda seharusnya melihat:
```
[System] Storage Mode: FIREBASE
[WiFi] Menghubungkan ke Your_WiFi_SSID
[WiFi] Terhubung! IP: 192.168.x.x
[Firebase] Inisialisasi Firebase Realtime Database dengan FirebaseClient...
[Firebase] FirebaseClient siap. Tunggu test koneksi...
```

**Test dengan mengetuk RFID card:**
```
[RFID] Kartu Terdeteksi! UID: 12 34 56 78 AB CD
[Firebase] Mencari UID di database...
[Firebase] GET request ke: https://PROJECT.firebaseio.com/students/...
[Firebase] HTTP Response Code: 200
[Firebase] Response body: {"name":"Budi Santoso","registered":true,"status":"Hadir"}
[AKSES] DITERIMA: Budi Santoso
[Firebase] Menyimpan log ke database...
[Firebase] POST request ke: https://PROJECT.firebaseio.com/attendance.json
[Firebase] HTTP Response Code: 200
[Firebase] Log berhasil disimpan!
```

---

## 🔄 Data Format Reference

### Students Node
Struktur data di `/students/{uid}`:
```json
{
  "name": "String - Nama Siswa",
  "registered": "Boolean - Terdaftar atau tidak",
  "status": "String - Status (Hadir, Izin, dll)"
}
```

### Attendance Node
Struktur data di `/attendance/` (auto-generated keys):
```json
{
  "-M1234567890abc": {
    "timestamp": "String - Format YYYY-MM-DD HH:MM:SS",
    "uid": "String - UID Kartu",
    "name": "String - Nama Siswa",
    "status": "String - Status (Hadir, Ditolak, dll)"
  }
}
```

---

## 📊 Monitoring Data

### View Data di Firebase Console
1. Buka Firebase Console
2. Realtime Database → Browse
3. Lihat students & attendance nodes
4. Real-time updates akan terlihat setiap scan kartu

### Export Data
**Untuk backup/analysis:**
1. Realtime Database → ⋮ → Export JSON
2. Save file `.json` ke komputer

---

## 🔐 Security Best Practices

### 1. API Key Protection
- **Jangan** commit `secrets.h` ke Git
- Gunakan `.gitignore`:
  ```
  include/secrets.h
  .env
  ```
- Copy `secrets.h.example` untuk sharing

### 2. Password Management
- Gunakan password yang kuat dan unik
- Jangan gunakan password yang sama dengan akun lain
- Consider menggunakan Google Secret Manager untuk production

### 3. Firebase Rules
- Development: Gunakan test mode (temporary)
- Production: Selalu update rules ke yang lebih restrictive
- Monitor akses di Firebase Console

### 4. SSL/TLS
ESP32 code sudah menggunakan SSL:
```cpp
ssl.setInsecure();  // OK untuk development, tapi untuk production gunakan certificate pinning
```

---

## 🚨 Common Issues & Fixes

| Issue | Cause | Fix |
|-------|-------|-----|
| HTTP 400 | Bad request / Rules deny access | Verify URL, UID, rules |
| HTTP 401 | Wrong credentials | Check API key & auth |
| Connection timeout | Network issue | Check WiFi signal |
| "Null" response | UID not in database | Add test data |
| SSL error | Certificate issue | Use `setInsecure()` |

Lihat **FIREBASE_TROUBLESHOOTING.md** untuk detail penyelesaian masalah.

---

## 📞 Firebase Console Navigation

```
Firebase Console
├── Project Settings (gear icon)
│   ├── General → Web API Key
│   ├── Service Accounts
│   └── Billing
├── Realtime Database
│   ├── Data → Browse/Edit
│   ├── Rules → Security rules
│   ├── ⋮ → Import JSON / Export JSON
│   └── Backups
├── Authentication
│   ├── Users → Create user
│   ├── Sign-in method
│   └── Custom claims
└── Hosting (optional - untuk web dashboard)
```

---

## 🧪 Testing Checklist

- [ ] Firebase project created
- [ ] Realtime Database enabled
- [ ] Test user created in Authentication
- [ ] API Key copied
- [ ] Database URL copied
- [ ] Credentials added to secrets.h
- [ ] Database structure imported
- [ ] Security rules updated
- [ ] ESP32 code set to USE_STORAGE_FIREBASE
- [ ] Code compiled & uploaded
- [ ] WiFi connected (check Serial)
- [ ] RFID card tested (should show success/error)
- [ ] Data visible in Firebase Console

---

## 📈 Next Steps

1. **Test dengan multiple RFID cards**
2. **Monitor data di Firebase Console**
3. **Setup backup/export schedule**
4. **Create web dashboard** (optional, menggunakan Firebase Hosting + Realtime DB)
5. **Setup alerts** untuk anomali kehadiran

---

## 📚 Reference Links

- [Firebase Console](https://console.firebase.google.com)
- [Firebase Realtime Database Docs](https://firebase.google.com/docs/database)
- [Firebase REST API](https://firebase.google.com/docs/database/rest/start)
- [Firebase Security Rules](https://firebase.google.com/docs/database/security)
- [FirebaseClient Library](https://github.com/mobizt/Firebase-ESP32)

---

**Created:** September 2026  
**Status:** Production Ready  
**Last Updated:** 2026-09-09
