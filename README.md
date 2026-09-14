# ESP32 IoT Absensi dengan AI Chatbot

## Alur Sistem

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ESP32 BOOT SEQUENCE                                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌─────────────────────────┐
                    │   Inisialisasi Hardware  │
                    │  - RFID (MFRC522)       │
                    │  - OLED SSD1306 (I2C)   │
                    │  - RTC DS1307 (I2C)     │
                    │  - MicroSD Card (SPI)   │
                    │  - LittleFS (Flash)     │
                    └───────────┬─────────────┘
                                │
                                ▼
                    ┌─────────────────────────┐
                    │   Cek Konfigurasi WiFi   │
                    │  (LittleFS: /wifi_config)│
                    └───────────┬─────────────┘
                                │
              ┌─────────────────┴─────────────────┐
              ▼                                   ▼
    ┌─────────────────────────┐         ┌─────────────────────────┐
    │   ADA KONFIGURASI       │         │   TIDAK ADA KONFIGURASI │
    │   WiFi Tersimpan        │         │   (First Boot)          │
    └───────────┬─────────────┘         └───────────┬─────────────┘
                │                                   │
                ▼                                   ▼
    ┌─────────────────────────┐         ┌─────────────────────────┐
    │ WiFi.mode(WIFI_STA)     │         │ WiFi.mode(WIFI_AP)      │
    │ WiFi.begin(ssid, pass)  │         │ SSID: ESP32-Setup       │
    │ Tunggu 15 detik         │         │ PASS: 1234567890        │
    │                         │         │ IP: 192.168.4.1         │
    └───────────┬─────────────┘         └───────────┬─────────────┘
                │                                   │
       ┌────────┴────────┐                          │
       ▼                 ▼                          ▼
  ┌─────────┐      ┌──────────┐           ┌──────────────────┐
  │ SUKSES  │      │  GAGAL   │           │ User buka browser │
  │ IP Dapat│      │          │           │ ke 192.168.4.1    │
  └────┬────┘      └────┬─────┘           │ Isi SSID & Pass  │
       │                │                  │ Klik Simpan      │
       │                │                  │ ESP restart      │
       ▼                ▼                  └────────┬─────────┘
                                                      │
       ┌──────────────────────────────────────────────┘
       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SISTEM SIAP (RUNNING)                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        ▼                           ▼                           ▼
┌───────────────┐          ┌───────────────┐          ┌───────────────┐
│  RFID LOOP    │          │  WEB SERVER   │          │  DISPLAY      │
│  (100ms)      │          │  (async)      │          │  (1 detik)    │
├───────────────┤          ├───────────────┤          ├───────────────┤
│ • Scan kartu  │          │ • /           │          │ • Jam & Tanggal│
│ • Cek UID di  │          │   Dashboard   │          │ • Status WiFi  │
│   users.json  │          │ • /chat       │          │ • Mode Setup   │
│ • LED Hijau/  │          │   AI Voice    │          │ • Pesan Absensi│
│   Merah       │          │ • /users      │          │               │
│ • Log ke SD   │          │   Kelola User │          │               │
│ • Update OLED │          │ • /api/*      │          │               │
└───────────────┘          └───────────────┘          └───────────────┘
```

---

## Struktur File

```
esp32-cek-rfid/
├── include/
│   ├── Config.h              # Pin definitions, global objects, common structs
│   ├── WiFiProvisioning.h    # AP mode, WiFi credential handling
│   ├── WebDashboard.h        # Web routes declaration, AI chat API
│   ├── RFIDAttendance.h      # RFID processing, attendance logic
│   ├── DisplayManager.h      # OLED display functions
│   └── UserRegistry.h        # Dynamic UID→Name mapping (JSON in LittleFS)
├── src/
│   ├── main.cpp              # Entry point: setup() & loop()
│   ├── WiFiProvisioning.cpp  # WiFi STA/AP, config save/load
│   ├── WebDashboard.cpp      # HTML pages, REST API, AI chat, User mgmt
│   ├── RFIDAttendance.cpp    # RFID read, verify, SD logging
│   ├── DisplayManager.cpp    # OLED render: idle, setup, messages
│   └── UserRegistry.cpp      # CRUD users.json (register, list, delete)
├── platformio.ini            # PlatformIO config (ArduinoJson added)
└── README.md                 # Dokumentasi ini
```

---

## Fitur Utama

### 1. WiFi Provisioning (First Boot)
- **AP Mode**: `ESP32-Setup` / `1234567890`
- **Config Portal**: `http://192.168.4.1` → Form SSID + Password
- **Persistensi**: Tersimpan di LittleFS (`/wifi_config.txt`)
- **Auto-reconnect**: Boot berikutnya langsung STA mode

### 2. Web Dashboard (`http://[ESP32_IP]`)
| Endpoint | Fungsi |
|----------|--------|
| `/` | Dashboard utama: fitur + status WiFi real-time |
| `/chat` | AI Chatbot dengan **Voice Input/Output** |
| `/users` | Kelola User: daftar UID → Nama + Role |
| `/config` | Ganti WiFi (kembali ke AP mode) |
| `/api/status` | JSON: uptime, wifiState, IP, freeRam |
| `/api/features` | JSON: daftar fitur sistem |
| `/api/chat` | POST: `{message}` → `{reply}` |
| `/api/users` | GET/POST/DELETE: CRUD user registry |

### 3. AI Chatbot (`/chat`)
- **Text Input**: Ketik pertanyaan
- **Voice Input (STT)**: Klik 🎤 → Bicara (Web Speech API, Bahasa Indonesia)
- **Voice Output (TTS)**: Jawaban AI dibaca otomatis (SpeechSynthesis)
- **Contoh pertanyaan**: "fitur", "status wifi", "siapa kamu", "halo"

### 4. Manajemen User Dinamis (`/users`)
- **Scan kartu** di halaman utama → UID muncul di Serial Monitor
- **Buka `/users`** → Isi UID (copy dari serial), Nama, Role (siswa/guru/staff)
- **Simpan** → Tersimpan ke `users.json` di LittleFS
- **Absensi berikutnya**: Nama otomatis muncul di OLED & log SD

### 5. RFID Attendance
- **Hardware**: MFRC522 (SPI)
- **Lookup**: UID → `users.json` → Nama
- **Feedback**: LED Hijau (terdaftar) / Merah (tidak dikenal)
- **Display**: OLED menampilkan "Selamat datang, [Nama]" atau "AKSES DITOLAK"
- **Log**: MicroSD `/absensi.csv` (Waktu, UID, Nama, Status)

### 6. Display OLED (SSD1306 128x64)
- **Idle**: Jam digital, tanggal, "Tempelkan Kartu..."
- **Setup Mode**: SSID, Password, IP AP
- **Absensi**: Pesan selamat datang / ditolak (2.5 detik)

---

## Pinout

| Fungsi | Pin ESP32 |
|--------|-----------|
| RFID SS   | GPIO 5  |
| RFID RST  | GPIO 15 |
| RFID SCK  | GPIO 18 |
| RFID MISO | GPIO 19 |
| RFID MOSI | GPIO 23 |
| SD CS     | GPIO 4  |
| I2C SDA   | GPIO 21 |
| I2C SCL   | GPIO 22 |
| LED Hijau | GPIO 2  |
| LED Merah | GPIO 12 |

---

## Build & Upload

```bash
# Install PlatformIO CLI
pip install platformio

# Build
pio run

# Upload ke ESP32
pio run -t upload

# Monitor Serial (115200 baud)
pio device monitor
```

---

## Dependencies (platformio.ini)

```ini
lib_deps =
    miguelbalboa/MFRC522
    adafruit/Adafruit SSD1306
    adafruit/Adafruit GFX Library
    adafruit/RTClib
    bblanchon/ArduinoJson
```

---

## Catatan Penting

1. **Tidak ada `secrets.h`** - Semua konfigurasi dinamis via Web UI
2. **User Registry** - Disimpan di LittleFS (`/users.json`), survive reboot
3. **Voice Chat** - Butuh browser modern (Chrome/Edge/Firefox) dengan izin mikrofon
4. **RTC** - Jika tidak ada battery, pakai compile time (`__DATE__`, `__TIME__`)
5. **SD Card** - Format FAT32, header CSV otomatis dibuat

---

## API Quick Reference

### Register User
```bash
curl -X POST http://[IP]/api/users \
  -H "Content-Type: application/json" \
  -d '{"uid":"04 A2 3F 1B","name":"Budi Santoso","role":"siswa"}'
```

### List Users
```bash
curl http://[IP]/api/users
```

### Delete User
```bash
curl -X DELETE "http://[IP]/api/users?uid=04%20A2%203F%201B"
```

### Chat AI
```bash
curl -X POST http://[IP]/api/chat \
  -H "Content-Type: application/json" \
  -d '{"message":"fitur apa saja?"}'
```