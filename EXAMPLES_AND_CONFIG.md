# Contoh Konfigurasi & Kode

File ini berisi contoh-contoh konfigurasi dan kode untuk membantu implementasi.

---

## 1. Contoh secrets.h untuk GAS Mode

```cpp
#pragma once

// Kredensial WiFi
#define WIFI_SSID     "MyWiFiNetwork"
#define WIFI_PASSWORD "MyWiFiPassword123"

// Google Apps Script - URL Web App deployment
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycbwvk0nFeojCH7EsveCTpVXJl3VKGwP9rlsMMS_iB30Fg9Kp_goH7gtZh6AQg3Kaa4VQig/exec"
```

---

## 2. Contoh secrets.h untuk Firebase Mode

```cpp
#pragma once

// Kredensial WiFi
#define WIFI_SSID     "MyWiFiNetwork"
#define WIFI_PASSWORD "MyWiFiPassword123"

// Firebase Realtime Database
#define FIREBASE_API_KEY       "AIzaSyDxZ1234567890abcdefghijk_example"
#define FIREBASE_DATABASE_URL  "https://esp32-attendance-12345.firebaseio.com"
#define FIREBASE_USER_EMAIL    "rfid@example.com"
#define FIREBASE_USER_PASSWORD "FirebasePassword123!"
```

---

## 3. Firebase Security Rules (Production)

Simpan di Firebase Console → Realtime Database → Rules tab:

```json
{
  "rules": {
    "students": {
      ".read": "auth != null",
      ".write": "root.child('admins').child(auth.uid).exists()",
      ".validate": "newData.hasChildren(['name', 'registered', 'status'])"
    },
    "attendance": {
      ".read": "auth != null",
      ".write": "auth != null",
      ".validate": "newData.hasChildren(['timestamp', 'uid', 'name', 'status'])"
    },
    "admins": {
      ".read": false,
      ".write": false
    }
  }
}
```

---

## 4. Firebase Rules (Development/Testing)

```json
{
  "rules": {
    ".read": true,
    ".write": "auth != null"
  }
}
```

---

## 5. Google Apps Script - Kode Lengkap

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
    
    // Cari siswa di Students sheet
    const studentData = findStudentByUID(sheets.students, uid);
    
    // Catat ke Attendance sheet
    const timestamp = date + " " + time;
    sheets.attendance.appendRow([
      timestamp,
      uid,
      studentData.name,
      studentData.status
    ]);
    
    // Log ke server apps script
    Logger.log(`[${timestamp}] UID: ${uid} | Name: ${studentData.name} | Status: ${studentData.status}`);
    
    // Return response
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
      const registered = data[i][2]; // Column C: Registered
      return {
        found: registered === true || registered === "TRUE" || registered === "true",
        name: data[i][1] || "Unknown",
        status: registered ? "Hadir" : "Ditolak"
      };
    }
  }
  
  return {
    found: false,
    name: "Tidak Dikenal",
    status: "Ditolak"
  };
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

// Deploy as Web App
// 1. Click Deploy > New Deployment
// 2. Type: Web app
// 3. Execute as: [Your Account]
// 4. Who has access: Anyone
// 5. Copy the URL and use as GOOGLE_SCRIPT_URL
```

---

## 6. Firebase Database Structure (JSON Import)

Gunakan fitur "Import JSON" di Firebase Console untuk import struktur ini:

```json
{
  "students": {
    "12 34 56 78 AB CD": {
      "name": "Budi Santoso",
      "registered": true,
      "status": "Hadir"
    },
    "AB CD 12 34 56 78": {
      "name": "Siti Nurhaliza",
      "registered": true,
      "status": "Hadir"
    },
    "FF EE DD CC BB AA": {
      "name": "Kartu Invalid",
      "registered": false,
      "status": "Ditolak"
    }
  },
  "attendance": {
    "12 34 56 78 AB CD": {
      "1694280645000": {
        "name": "Budi Santoso",
        "status": "Hadir",
        "timestamp": "2023-09-09 10:30:45",
        "uid": "12 34 56 78 AB CD"
      },
      "1694284245000": {
        "name": "Budi Santoso",
        "status": "Hadir",
        "timestamp": "2023-09-09 11:30:45",
        "uid": "12 34 56 78 AB CD"
      }
    }
  }
}
```

---

## 7. Fungsi Callback Firebase di main.cpp

Contoh meng-extend kode untuk custom logic:

```cpp
// Custom callback untuk handle Firebase response
void onFirebaseDataReceived(const String& path, const String& data) {
  Serial.println("[Callback] Firebase path: " + path);
  Serial.println("[Callback] Data: " + data);
  
  // Tambah custom logic di sini
  // Contoh: trigger relay, send SMS, log ke server, dll
}

// Modifikasi logToFirebase untuk custom logic
void logToFirebase(const String& timestamp, const String& uid, 
                   const String& name, const String& status) {
  if (!firebaseReady || !Firebase.ready()) {
    Serial.println("[Firebase] Tidak terhubung.");
    return;
  }

  String path = "/attendance/" + uid + "/" + String(millis());
  
  FirebaseJson json;
  json.set("timestamp", timestamp);
  json.set("uid", uid);
  json.set("name", name);
  json.set("status", status);
  
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("[Firebase] Log berhasil disimpan!");
    
    // Call custom callback
    onFirebaseDataReceived(path, json.raw());
    
  } else {
    Serial.println("[Firebase] Gagal: " + fbdo.errorReason());
  }
}
```

---

## 8. Sistem Recovery & Fallback

Contoh kode untuk fallback ke SD Card jika cloud offline:

```cpp
// Mode hybrid: Cloud + SD Card
void saveAttendanceLog(const String& timestamp, const String& uid, 
                       const String& name, const String& status) {
  bool cloudSuccess = false;
  
  #if defined(STORAGE_MODE) && STORAGE_MODE == "FIREBASE"
    if (firebaseReady && Firebase.ready()) {
      logToFirebase(timestamp, uid, name, status);
      cloudSuccess = true;
    } else {
      Serial.println("[Warning] Firebase offline, fallback to SD");
    }
  #else
    if (WiFi.status() == WL_CONNECTED) {
      logToSDCard(timestamp, uid, name, status);
      cloudSuccess = true;
    } else {
      Serial.println("[Warning] Network offline, fallback to SD");
    }
  #endif
  
  // Selalu simpan ke SD Card untuk backup
  if (!cloudSuccess) {
    logToSDCard(timestamp, uid, name, status);
    Serial.println("[SD] Fallback log saved to SD card");
  }
}
```

---

## 9. Testing Script untuk Firebase

Gunakan Firebase Console → Realtime Database → Console tab:

```javascript
// Test query siswa
firebase.database().ref('students/12 34 56 78 AB CD').once('value')
  .then(function(snapshot) {
    console.log("Student data:", snapshot.val());
  });

// Test insert attendance (untuk testing)
firebase.database().ref('attendance/12 34 56 78 AB CD/' + Date.now())
  .set({
    timestamp: new Date().toLocaleString(),
    uid: "12 34 56 78 AB CD",
    name: "Test User",
    status: "Hadir"
  })
  .then(() => console.log("Data written"))
  .catch((err) => console.log("Error:", err));

// Monitor real-time attendance
firebase.database().ref('attendance').on('child_added', function(snapshot) {
  console.log("New attendance:", snapshot.val());
});
```

---

## 10. HTTP Raw Request Format

Jika ingin test GAS mode dengan curl/Postman:

### Request (GET)
```
GET /macros/s/YOUR_SCRIPT_ID/exec?uid=12%2034%2056%2078%20AB%20CD&date=2026-09-09&time=10:30:45 HTTP/1.1
Host: script.google.com
Content-Type: application/json
```

### Response
```json
{
  "registered": true,
  "name": "Budi Santoso",
  "status": "Hadir"
}
```

---

## 11. Monitoring & Debugging

### Firebase - Monitor Data Changes
```cpp
// Tambah di setup() untuk debug
void setupFirebaseMonitoring() {
  Firebase.RTDB.setReadTimeout(&fbdo, 1000 * 60 * 5); // 5 menit
  Firebase.RTDB.enableClassicRequest(&fbdo, true);
  
  Serial.println("[Firebase] Monitoring setup complete");
}

// Di loop untuk periodic check
unsigned long lastCheck = 0;
void checkFirebaseStatus() {
  if (millis() - lastCheck > 30000) { // Check setiap 30 detik
    lastCheck = millis();
    
    if (Firebase.ready()) {
      Serial.println("[Firebase] ✓ Connection OK");
    } else {
      Serial.println("[Firebase] ✗ Connection FAILED");
    }
  }
}
```

### GAS - Monitor via Apps Script
```javascript
function viewRecentLogs(hours = 1) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Attendance");
  const data = sheet.getDataRange().getValues();
  const cutoffTime = new Date(Date.now() - hours * 60 * 60 * 1000);
  
  console.log("Recent logs dari " + hours + " jam terakhir:");
  
  for (let i = data.length - 1; i >= 1; i--) {
    const rowTime = new Date(data[i][0]);
    if (rowTime >= cutoffTime) {
      console.log(`${data[i][0]} | ${data[i][1]} | ${data[i][2]} | ${data[i][3]}`);
    }
  }
}

// Jalankan dari Apps Script console
viewRecentLogs(1); // Recent 1 jam
```

---

## 12. Common Issues & Solutions

### Firebase - CORS Error
**Problem:** Firebase request blocked by CORS

**Solution:** 
- Pastikan library Firebase-ESP32 versi terbaru
- Update Firebase config di main.cpp
- Restart board ESP32

### GAS - Intermittent Failures  
**Problem:** Kadang berhasil, kadang timeout

**Solution:**
- Naikkan timeout di checkAndLogToGoogle()
- Cek quota Google Apps Script
- Monitor server logs

### Both Modes - WiFi Disconnection
**Problem:** Koneksi terputus saat scanning kartu

**Solution:**
```cpp
// Di loop(), tambah WiFi check
if (WiFi.status() != WL_CONNECTED) {
  Serial.println("[WiFi] Reconnecting...");
  WiFi.reconnect();
  delay(1000);
}
```

---

**Tips:** 
- Selalu lihat Serial Monitor untuk debug message
- Test dengan kartu fisik di hardware asli (bukan Wokwi)
- Backup database Firebase & Google Sheets secara berkala
