# 🔧 Firebase Troubleshooting Guide

Panduan mengatasi error HTTP saat menggunakan Firebase Realtime Database dengan ESP32.

---

## ❌ Error: HTTP 400 (Bad Request)

### Penyebab Umum

1. **Firebase Rules tidak mengizinkan read/write**
2. **Database URL tidak benar**
3. **UID memiliki karakter khusus yang tidak valid**
4. **JSON payload tidak valid**

### Solusi

#### Step 1: Verifikasi Database URL di secrets.h
```cpp
// ✅ Format yang benar:
#define FIREBASE_DATABASE_URL "https://project-name.firebaseio.com"

// ❌ Jangan gunakan:
#define FIREBASE_DATABASE_URL "https://project-name.firebaseio.com/"  // Ada trailing /
#define FIREBASE_DATABASE_URL "https://project-name.firebaseio.com/students"  // Sudah include path
```

#### Step 2: Update Firebase Security Rules

Di Firebase Console → Realtime Database → Rules tab, gunakan rules ini:

**Untuk DEVELOPMENT (testing):**
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

**Untuk PRODUCTION (secure):**
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

#### Step 3: Pastikan UID Valid

UID dari RFID card tidak boleh memiliki karakter khusus. Contoh valid:
- `12345678AB` ✅
- `12 34 56 78 AB CD` (space OK, tapi avoid space di database)
- `UID_12345` ✅

**Jangan gunakan:**
- `12:34:56:78:AB:CD` (colon not allowed in path)
- `12/34/56/78` (slash not allowed in path)

Jika UID dari RFID punya spaces, replace dengan underscore:
```cpp
String cleanUID = uid;
cleanUID.replace(" ", "_");  // Ganti space dengan underscore
```

#### Step 4: Monitor Serial Output

Lakukan scan kartu dan lihat message di Serial Monitor:

```
[Firebase] GET request ke: https://PROJECT.firebaseio.com/students/...
[Firebase] HTTP Response Code: 400
[Firebase] Bad Request (400). Check:
  - Database URL format
  - UID format (no spaces or special chars)
  - Firebase rules allow read access
```

---

## ❌ Error: HTTP 401 (Unauthorized)

### Penyebab
- Credentials (API Key, email, password) tidak valid
- Firebase project tidak aktif

### Solusi

1. **Verifikasi API Key di secrets.h**
   ```cpp
   // Cek di Firebase Console > Project Settings > General tab
   #define FIREBASE_API_KEY "AIzaSy..."
   ```

2. **Cek Firebase Authentication**
   - Buka Firebase Console → Authentication
   - Pastikan ada user dengan email yang di-setup
   - Email/password harus valid

3. **Test API Key**
   ```bash
   # Di cmd/terminal, test API key:
   curl "https://PROJECT.firebaseio.com/.json?auth=YOUR_API_KEY"
   ```

---

## ❌ Error: HTTP 500 (Server Error)

### Penyebab
- Firebase service sedang down
- Database structure tidak sesuai dengan rules

### Solusi
- Tunggu beberapa menit
- Cek status Firebase di [status.firebase.google.com](https://status.firebase.google.com)
- Verifikasi database structure di Firebase Console

---

## ✅ Testing Firebase Secara Manual

### Test GET (Query Student)

**Menggunakan Curl:**
```bash
curl "https://PROJECT.firebaseio.com/students/12345678AB.json"
```

**Expected Response:**
```json
{
  "name": "Budi Santoso",
  "registered": true,
  "status": "Hadir"
}
```

### Test POST (Save Attendance)

**Menggunakan Curl:**
```bash
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{"timestamp":"2026-09-09 10:30:45","uid":"12345678AB","name":"Budi","status":"Hadir"}' \
  "https://PROJECT.firebaseio.com/attendance.json"
```

**Expected Response:**
```json
{
  "name": "-GENERATED_KEY_ID_12345"
}
```

---

## 🔍 Debug Checklist

Saat troubleshooting HTTP error, cek hal ini secara berurutan:

- [ ] WiFi terhubung? (Serial monitor lihat `[WiFi] Terhubung`)
- [ ] Database URL format benar di secrets.h?
- [ ] API Key ada dan valid?
- [ ] Firebase Rules sudah di-update?
- [ ] UID tidak punya karakter khusus?
- [ ] Tidak ada trailing slash di URL?
- [ ] JSON payload valid? (gunakan [jsonlint.com](https://jsonlint.com))
- [ ] SSL certificate issue? (cek `setInsecure()` di code)
- [ ] Firestore atau Realtime Database yang dipakai?

---

## 📊 Database Structure yang Benar

Pastikan struktur database di Firebase Console seperti ini:

```
esp32-attendance/
├── students/
│   ├── 12345678AB
│   │   ├── name: "Budi Santoso"
│   │   ├── registered: true
│   │   └── status: "Hadir"
│   └── ABCDEF123456
│       ├── name: "Siti Nurhaliza"
│       ├── registered: true
│       └── status: "Hadir"
└── attendance/
    ├── -M1234567890abc
    │   ├── timestamp: "2026-09-09 10:30:45"
    │   ├── uid: "12345678AB"
    │   ├── name: "Budi Santoso"
    │   └── status: "Hadir"
    └── -M1234567890def
        ├── timestamp: "2026-09-09 10:35:12"
        ├── uid: "ABCDEF123456"
        ├── name: "Siti Nurhaliza"
        └── status: "Hadir"
```

---

## 🚨 HTTP Status Code Reference

| Code | Meaning | Action |
|------|---------|--------|
| 200 | OK | ✅ Success |
| 400 | Bad Request | Check URL format & JSON |
| 401 | Unauthorized | Check credentials |
| 403 | Forbidden | Check Firebase rules |
| 404 | Not Found | Data tidak ada (normal untuk UID baru) |
| 500 | Server Error | Firebase down, retry later |
| 503 | Service Unavailable | Firebase maintenance |

---

## 💡 Tips Penting

### 1. UID Handling
```cpp
// Jika UID dari RFID punya spaces, bersihkan:
String cleanUID = uid;
cleanUID.replace(" ", "_");  // Replace space dengan underscore

// Atau gunakan langsung tanpa spaces
String uid = "12345678ABCD";  // Tanpa space dari awal
```

### 2. JSON Validation
Sebelum push ke Firebase, pastikan JSON valid:
- Gunakan [jsonlint.com](https://jsonlint.com) untuk test
- Lihat di Serial Monitor: `[Firebase] JSON payload: {...}`

### 3. Debugging Mode
Edit `src/main.cpp` dan uncomment debug logging:
```cpp
#define DEBUG_FIREBASE  // Tambah ini di atas
```

### 4. Test Data Import
Gunakan Firebase Console untuk import test data:
```json
{
  "students": {
    "12345678AB": {
      "name": "Test Student",
      "registered": true,
      "status": "Hadir"
    }
  }
}
```

---

## 📞 Jika Masih Error

1. **Lihat Serial Monitor output lengkap**
   - Copy paste error message di sini
   - Cek timestamp dan sequence of events

2. **Test dengan Curl/Postman dulu**
   - Pastikan database bisa diakses dari komputer
   - Jika Curl berhasil tapi ESP32 gagal, masalah di code/SSL

3. **Firebase Console Logs**
   - Buka Firebase Console → Database
   - Lihat apakah data terkirim walau ada error?
   - Cek struktur data di browser

4. **Cek Internet Stability**
   - Test WiFi dengan speedtest.net
   - Pastikan latency/ping rendah
   - Jangan terlalu jauh dari router

---

## ✅ Quick Fixes

### Untuk HTTP 400:
```cpp
// Tambah di awal setup():
Serial.println("[Firebase] Database URL: " + String(FIREBASE_DATABASE_URL));
Serial.println("[Firebase] API Key: " + String(FIREBASE_API_KEY));
```

### Untuk UID validation:
```cpp
void printUID(const String& uid) {
  Serial.print("[UID] Bytes: ");
  for (int i = 0; i < uid.length(); i++) {
    Serial.printf("%02X ", (byte)uid[i]);
  }
  Serial.println();
  Serial.println("[UID] Length: " + String(uid.length()));
  Serial.println("[UID] String: " + uid);
}
```

---

**Last Updated:** September 2026  
**Status:** Ready for Production  
**Firebase Library:** FirebaseClient (REST API)
