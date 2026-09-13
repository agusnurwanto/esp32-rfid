# ESP32 IoT Absensi dengan AI Chatbot

## Fitur Utama

1. **WiFi Provisioning via Hotspot**
   - Saat pertama kali boot, ESP32 menjadi Access Point (AP) dengan SSID: `ESP32-Setup`
   - Password: `1234567890`
   - Buka browser ke `http://192.168.4.1` untuk mengatur WiFi yang ingin digunakan
   - Setelah disimpan, ESP32 akan restart dan tersambung ke jaringan WiFi yang ditentukan

2. **Web Dashboard**
   - Setelah tersambung ke WiFi, akses `http://[ESP32_IP]/dashboard` untuk lihat fitur
   - Endpoint API tersedia:
     - `/api/status` - Status sistem (WiFi, IP, uptime)
     - `/api/features` - Daftar fitur yang tersedia
     - `/api/chat` - Chat dengan AI assistant
     - `/api/voice` - Endpoint untuk input audio (STT belum tersedia pada ESP32)

3. **AI Chatbot dengan Audio Input**
   - Buka `http://[ESP32_IP]/chat` untuk menggunakan chatbot
   - Fitur:
     - **Text Input**: Ketik pesan ke dalam kolom input
     - **Speech-to-Text**: Klik mikrofon untuk merekam suara (menggunakan Web Speech API di browser)
     - **Text-to-Speech**: Respons AI akan diputar lewat speaker browser

## Cara Menggunakan

### Langkah 1: Setup Pertama
1. Hubungkan ESP32 ke laptop/PC via USB
2. Buka Serial Monitor (115200 baud) untuk melihat log
3. Di laptop, buka WiFi yang `ESP32-Setup`
4. Akses `http://192.168.4.1` di browser
5. Isi SSID dan Password WiFi yang ingin digunakan
6. Klik "Simpan & Sambungkan" - ESP32 akan restart dan tersambung ke WiFi

### Langkah 2: Akses Dashboard
1. Setelah tersambung, cari IP ESP32 di Serial Monitor
2. Akses `http://[IP_ESP32]` untuk dashboard utama
3. Klik "💬 AI Chat" untuk menggunakan chatbot
4. Klik "📶 Setup WiFi" untuk mengganti WiFi di lain waktu

### Langkah 3: Pakai AI Chat
Di halaman chat:
- Ketik pesan atau klik tombol mikrofon untuk berbicara
- AI akan merespons dalam Bahasa Indonesia
- Respons akan diputar lewat Text-to-Speech di browser

## Struktur File

```
esp32-cek-rfid/
├── src/
│   └── main.cpp          # Semua kode utama
├── include/
│   └── secrets.h         # Kredensial WiFi (BUAT SENDIRI dari secrets.h.example)
├── ai_voice_chatbot.py   # Python script untuk voice chatbot alternative
├── platformio.ini        # Konfigurasi PlatformIO
└── README.md             # Dokumentasi ini
```

## Modifikasi secrets.h

Salin `include/secrets.h.example` menjadi `include/secrets.h` dan isi dengan nilai Anda:

```c
#define WIFI_SSID "NamaWiFiAnda"
#define WIFI_PASSWORD "PasswordWiFiAnda"
#define FIREBASE_API_KEY "api_key_ini_opsional"
#define FIREBASE_DATABASE_URL "https://project.firebaseio.com"
```

## Catatan Penting

- FirebaseClient tidak diaktifkan untuk penggunaan default (gunakan SD card logging)
- Audio input di ESP32 membutuhkan library STT yang tidak tersedia di Arduino framework
- Untuk pengalaman voice chat yang lebih lengkap, gunakan Python script `ai_voice_chatbot.py`

## Penggunaan Audio di Chat

Fitur audio chat menggunakan Web Speech API browser:
- **Speech-to-Text**: Microphone wajib diizinkan permission-nya
- **Text-to-Speech**: Menggunakan Web Speech Synthesis API
- Kedua fitur ini berjalan di browser, bukan di ESP32

Untuk STT yang berjalan di ESP32, diperlukan library MicroWebRTC + Whisper model yang sangat besar.