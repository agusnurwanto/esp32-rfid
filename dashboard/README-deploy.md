# Deploy Dashboard ke Firebase Hosting

## Prasyarat

- Node.js 18+ (cek dengan `node --version`)
- Akun Google/Firebase
- Firebase project (buat di https://console.firebase.google.com)

---

## Langkah 1: Install Firebase CLI

```bash
npm install -g firebase-tools
```

Verifikasi instalasi:

```bash
firebase --version
```

---

## Langkah 2: Login ke Firebase

```bash
firebase login
```

Browser akan terbuka — login dengan akun Google yang ingin digunakan.

---

## Langkah 3: Isi Konfigurasi Firebase

Buka file `firebase-config.json` dan isi dengan nilai dari Firebase Console:

1. Buka https://console.firebase.google.com
2. Pilih/project yang akan digunakan
3. Klik **Project Settings** (gear icon)
4. Tab **General**
5. Copy **Web API Key** → `apiKey`
6. Copy **Database URL** (dari Realtime Database) → `databaseURL`
7. Copy **Project ID** → `projectId`

Contoh isi `firebase-config.json`:

```json
{
  "apiKey": "AIzaSyDxxxxxxxxxxxxxxxxxxxx",
  "databaseURL": "https://esp32-rfid-attendance.firebaseio.com",
  "projectId": "esp32-rfid-attendance"
}
```

---

## Langkah 4: Deploy

Jalankan dari dalam folder `dashboard/`:

```bash
cd /path/ke/esp32-rfid/dashboard
bash deploy-dashboard.sh
```

Atau secara manual:

```bash
# Masuk ke folder dashboard
cd /e/xampp/htdocs/robotik/esp32-rfid/dashboard

# Inject config ke index.html
bash firebase.sh

# Atau langsung deploy
firebase deploy --only hosting
```

---

## Langkah 5: Verifikasi

Setelah deploy berhasil, Firebase akan menampilkan URL:

```
✔ Deploy complete!

Project Console: https://console.firebase.google.com/project/...
Hosting URL: https://PROJECT_ID.web.app
```

Buka URL tersebut di browser. Dashboard akan langsung live dan terhubung ke Firebase tanpa perlu input manual.

---

## Struktur File (Dashboard Folder)

```
dashboard/
├── index.html              # Dashboard utama (terminal config di-inject)
├── firebase-config.json    # Konfigurasi Firebase (API key, URL)
├── firebase.json           # Konfigurasi Firebase Hosting
├── firebase.sh             # Script deploy otomatis
├── deploy-dashboard.sh     # Script deploy lengkap
└── .gitignore (optional)   # Tambahkan firebase-config.json
```

---

## Firebase Rules (Production)

Di Firebase Console → Realtime Database → Rules, gunakan:

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

> Catatan: dashboard hanya melakukan READ. Dashboard tidak perlu write access ke database.

---

## Troubleshooting

**Error: "No Firebase App named '[DEFAULT]'"**
→ Pastikan `firebase-config.json` diisi dengan benar dan deploy dilakukan ulang.

**Error: "Permission denied"**
→ Cek Firebase Rules. Dashboard butuh `.read: true` di path `students` dan `attendance`.

**Dashboard tidak muncul / blank**
→ Cek console browser (F12) untuk error. Kemungkinan: config tidak diinject, Firebase SDK gagal dimuat, atau network blocked.

**URL tidak bisa diakses**
→ Cek Firebase Hosting → paling baru di console. Pastikan deploy sukses.

---

## Mengupdate Dashboard

Jika ada perubahan di `index.html`:

1. Edit `index.html`
2. Jalankan `firebase deploy --only hosting` lagi
3. Perubahan akan live dalam beberapa detik

Untuk update config (ganti project), edit `firebase-config.json` lalu deploy ulang.
