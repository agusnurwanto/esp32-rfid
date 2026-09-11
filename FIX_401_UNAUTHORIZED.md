# 🔓 Fix HTTP 401 (Unauthorized) Error

**Error:** `[Firebase] ✗ Unauthorized (401)`

Ini berarti Firebase Rules tidak mengizinkan akses atau API Key tidak valid.

---

## ✅ Quick Fix - 3 Steps

### Step 1: Verify Firebase Console
1. Buka https://console.firebase.google.com/
2. Pilih project Anda
3. Realtime Database → **Rules** tab

### Step 2: Update Firebase Rules

Copy-paste rules ini dan klik **"Publish":**

```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

**⚠️ WARNING:** Ini adalah test mode (public access). Untuk production, gunakan rules yang lebih restrictive!

### Step 3: Rebuild & Upload ESP32

```bash
pio run -t upload -e esp32dev
```

Scan RFID card lagi. Seharusnya sekarang berhasil! ✓

---

## 🔍 Jika Masih Error 401

### Check 1: API Key di secrets.h
```cpp
#define FIREBASE_API_KEY "AIzaSy..."  // Harus tidak kosong!
```

**Cara dapat API Key yang benar:**
1. Firebase Console → Project Settings (gear icon)
2. Tab "General"
3. Scroll ke "Your apps" atau cari "Web API Key"
4. Copy nilai tersebut (dimulai dengan "AIzaSy...")

### Check 2: Database URL di secrets.h
```cpp
#define FIREBASE_DATABASE_URL "https://project-name.firebaseio.com"
```

**Format yang benar:**
- ✅ `https://esp32-rfid-abc123.firebaseio.com`
- ❌ `https://esp32-rfid-abc123.firebaseio.com/` (jangan ada trailing slash)
- ❌ `https://esp32-rfid-abc123.firebaseio.com/students` (jangan include path)

### Check 3: Firebase Rules

Buka Firebase Console → Realtime Database → Rules tab.

**Lihat apa yang sekarang:**
```json
{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}
```

**Masalah:** Ini memerlukan authentication, tapi code kita tidak melakukan auth dengan proper.

**Solusi:** Ubah ke:
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

Klik **"Publish"** dan tunggu sampai hijau ✓

---

## 🚀 Jika Ingin Production-Ready (Secure)

Gunakan rules yang lebih restrictive:

```json
{
  "rules": {
    "students": {
      ".read": true,
      ".write": false
    },
    "attendance": {
      ".read": true,
      ".write": true
    }
  }
}
```

---

## 🔧 Testing dengan Curl

Sebelum upload ESP32 lagi, test API Key Anda dengan curl:

```bash
# Replace dengan nilai Anda
curl "https://YOUR_PROJECT.firebaseio.com/.json?auth=YOUR_API_KEY"
```

Jika tidak error 401, berarti credentials OK!

---

## 📊 Status Code Reference

| Code | Meaning | Fix |
|------|---------|-----|
| 200 | OK | ✅ Success |
| 400 | Bad Request | Check UID format & JSON |
| **401** | **Unauthorized** | **Update Firebase Rules** |
| 403 | Forbidden | Check Rules, not auth issue |
| 404 | Not Found | UID not in database (OK) |

---

## 💡 Serial Monitor Should Show

Setelah fix, Anda akan melihat:

```
[Firebase] POST to: https://PROJECT.firebaseio.com/attendance.json?auth=AIzaSy...
[Firebase] Response Code: 200
[Firebase] ✓ Log berhasil disimpan!
[Firebase] Response: {"name":"-M1234567890abc"}
```

---

**Done! Now rebuild and upload.** 🚀
