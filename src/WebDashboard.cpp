#include "WebDashboard.h"
#include "WiFiProvisioning.h"
#include <ArduinoJson.h>

// HTML templates
const char* SETUP_HTML = R"HTML(<!DOCTYPE html><html><head><title>ESP32 Setup</title>
<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:2rem}.card{background:#1e293b;padding:1.5rem;border-radius:10px;max-width:400px;margin:0 auto}
input{width:100%;padding:.8rem;margin:.5rem 0;background:#0f172a;border:1px solid #334155;color:#e2e8f0;border-radius:4px}
button{width:100%;padding:.8rem;background:#0ea5e9;color:#fff;border:none;border-radius:4px;cursor:pointer}button:hover{background:#0284c7}</style></head>
<body><div class='card'><h1>ESP32 Setup WiFi</h1><form action='/save' method='POST'>
<label>SSID</label><input type='text' name='ssid' placeholder='Nama WiFi' required>
<label>Password</label><input type='password' name='pass' placeholder='Password WiFi' required>
<button type='submit'>Simpan & Sambungkan</button></form></div></body></html>)HTML";

const char* DASHBOARD_HTML = R"HTML(<!DOCTYPE html><html><head><title>ESP32 Dashboard</title>
<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px}.card{background:#1e293b;padding:20px;border-radius:12px;margin:20px 0}h1{color:#38bdf8}.btn{display:inline-block;background:#38bdf8;color:#0f172a;padding:10px 20px;border-radius:6px;margin:5px}.features{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}.feat{background:#0f172a;padding:15px;border-radius:8px}</style></head>
<body><h1>ESP32 Dashboard</h1>
<div class='card'><h2>Fitur</h2><div id='features'></div></div>
<div class='card'><h2>Status</h2><p>WiFi: <span id='wifi'>—</span></p><p>IP: <span id='ip'>—</span></p></div>
<a href='/chat' class='btn'>💬 AI Chat</a>
<a href='/config' class='btn'>📶 Setup WiFi</a>
<a href='/users' class='btn'>👥 Kelola User</a>
<script>fetch('/api/features').then(r=>r.json()).then(d=>document.getElementById('features').innerHTML=d.features.map(f="<div class='feat'><b>"+f.name+"</b><p>"+f.desc+"</p></div>").join(''));
function u(){fetch('/api/status').then(r=>r.json()).then(d=>{document.getElementById('wifi').textContent=d.wifiState;document.getElementById('ip').textContent=d.ip;});}u();setInterval(u,5000);</script></body></html>)HTML";

const char* CHAT_HTML = R"HTML(<!DOCTYPE html><html><head><meta charset='UTF-8'><title>AI Chat</title>
<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;display:flex;height:100vh}app{width:100%;max-width:500px;display:flex;flex-direction:column}header{background:#16213e;color:#38bdf8;padding:15px}.messages{flex:1;overflow-y:auto;padding:15px;background:#1e293b}.msg{border-radius:10px;padding:10px;margin:5px 0;max-width:80%}.user{align-self:flex-end;background:#38bdf8;color:#0f172a}.bot{align-self:flex-start;background:#0f172a;border:1px solid #334155}.input{display:flex;padding:10px;border-top:1px solid #334155;background:#0f172a}input{flex:1;padding:10px;border-radius:20px;border:1px solid #334155;background:#0f172a;color:#e2e8f0}.send{background:#38bdf8;color:#0f172a;border:none;border-radius:50%;width:40px;height:40px;cursor:pointer}.muted{background:#334155;cursor:not-allowed}</style></head>
<body><app><header><h2>🤖 AI Chat Voice</h2></header>
<messages id='m'></messages>
<div class='input'>
<button id='mic' onclick='toggleListening()'>🎤</button>
<input type='text' id='i' placeholder='Ketik atau ucapkan pesan...'>
<button class='send' onclick='send()'>➤</button>
</div></app>
<script>
const m=document.getElementById('m'),i=document.getElementById('i'),btn=document.getElementById('mic');
let listening=false,rec;
const SpeechRecognition=window.SpeechRecognition||window.webkitSpeechRecognition;
if(SpeechRecognition){rec=new SpeechRecognition();rec.lang='id-ID';rec.continuous=true;rec.interimResults=true;
rec.onresult=function(e){let t='';for(let r of e.results){if(r[0])t+=r[0].transcript;}i.value=t;
if(!listening)rec.stop();if(e.results[e.results.length-1].isFinal)send(t);};}
function toggleListening(){if(!rec){alert('Web Speech API tidak didukung');return;}
if(listening){rec.stop();btn.classList.remove('muted');listening=false;}
else{rec.start();btn.classList.add('muted');listening=true;m.innerHTML+='<div class="msg user"><i>Mendengar...</i></div>';}}
function send(t){if(!t)t=i.value.trim();if(!t)return;m.innerHTML+='<div class="msg user">'+t+'</div>';i.value='';
fetch('/api/chat', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({message:t})})
.then(r=>r.json()).then(d=>{m.innerHTML+='<div class="msg bot">'+d.reply+'</div>';
if('speechSynthesis'in window){const u=new SpeechSynthesisUtterance(d.reply);u.lang='id-ID';speechSynthesis.speak(u);}}).catch(()=>{});m.scrollTop=m.scrollHeight;}
document.addEventListener('keydown',function(e){if(e.key==='Enter'&&!e.shiftKey){e.preventDefault();send();}});
</script></body></html>)HTML";

const char* USERS_HTML = R"HTML(<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Kelola User</title>
<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px}.card{background:#1e293b;padding:20px;border-radius:12px;margin:20px 0}h1{color:#38bdf8}.btn{display:inline-block;background:#38bdf8;color:#0f172a;padding:10px 20px;border-radius:6px;margin:5px}.btn-danger{background:#ef4444}.form-group{margin:10px 0}label{display:block;margin-bottom:5px}input,select{width:100%;padding:10px;border-radius:4px;border:1px solid #334155;background:#0f172a;color:#e2e8f0}table{width:100%;border-collapse:collapse}th,td{padding:10px;border-bottom:1px solid #334155;text-align:left}th{color:#94a3b8}</style></head>
<body><h1>👥 Kelola User RFID</h1>
<div class='card'><h2>Tambah User Baru</h2>
<div class='form-group'><label>UID Kartu (scan dulu di halaman utama)</label><input type='text' id='uid' placeholder='Contoh: 04 A2 3F 1B'></div>
<div class='form-group'><label>Nama</label><input type='text' id='name' placeholder='Nama Lengkap'></div>
<div class='form-group'><label>Role</label><select id='role'><option value='siswa'>Siswa</option><option value='guru'>Guru</option><option value='staff'>Staff</option></select></div>
<button class='btn' onclick='registerUser()'>Daftarkan</button>
<p id='msg'></p></div>
<div class='card'><h2>Daftar User</h2><table><thead><tr><th>UID</th><th>Nama</th><th>Role</th><th>Aksi</th></tr></thead><tbody id='userList'></tbody></table></div>
<script>
async function loadUsers(){const r=await fetch('/api/users');const users=await r.json();
const t=document.getElementById('userList');t.innerHTML=users.map(u="<tr><td>"+u.uid+"</td><td>"+u.name+"</td><td>"+(u.role||"-")+"</td><td><button class='btn btn-danger' onclick='deleteUser(\""+u.uid+"\")'>Hapus</button></td></tr>").join('');}
async function registerUser(){const uid=document.getElementById('uid').value.trim().toUpperCase();const name=document.getElementById('name').value.trim();const role=document.getElementById('role').value;
if(!uid||!name){alert('UID dan nama wajib diisi');return}
const r=await fetch('/api/users',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({uid,name,role})});
if(r.ok){document.getElementById('msg').innerText='User berhasil didaftarkan';document.getElementById('uid').value='';document.getElementById('name').value='';loadUsers();}else{const e=await r.text();alert('Gagal: '+e);}}
async function deleteUser(uid){if(!confirm('Hapus user '+uid+'?'))return;const r=await fetch('/api/users?uid='+encodeURIComponent(uid),{method:'DELETE'});if(r.ok)loadUsers();else alert('Gagal hapus');}
loadUsers();
</script></body></html>)HTML";

String generateAIResponse(const String& input) {
  String q = input; q.toLowerCase(); q.trim();
  if (q.indexOf("siapa") != -1 && q.indexOf("kamu") != -1)
    return "Saya asisten AI dari ESP32. Bisa bantu tanya tentang absensi, fitur, atau status.";
  if (q.indexOf("fitur") != -1 || q.indexOf("apa") != -1)
    return "Fitur: Absensi RFID, OLED display, MicroSD, WiFi provisioning, Dashboard web, AI Chat, Audio input, Kelola User.";
  if (q.indexOf("halo") != -1 || q.indexOf("hai") != -1)
    return "Halo! Ada yang bisa saya bantu?";
  if (q.indexOf("terima kasih") != -1)
    return "Sama-sama!";
  if (q.indexOf("status") != -1 || q.indexOf("wifi") != -1) {
    if (WiFi.status() == WL_CONNECTED)
      return "WiFi Connected. IP: " + WiFi.localIP().toString();
    return "WiFi tidak terhubung. Mode AP: buka http://192.168.4.1";
  }
  return "Maaf, tidak mengerti. Coba: 'fitur', 'status', atau 'halo'.";
}

String getFeaturesJSON() {
  return "{\"features\":["
    "{\"icon\":\"📁\",\"name\":\"Absensi RFID\",\"desc\":\"Tempelkan kartu untuk absensi.\"},"
    "{\"icon\":\"🌡️\",\"name\":\"RTC & OLED\",\"desc\":\"Waktu real-time di display.\"},"
    "{\"icon\":\"💾\",\"name\":\"Log MicroSD\",\"desc\":\"Catatan di CSV microSD.\"},"
    "{\"icon\":\"📶\",\"name\":\"WiFi Provisioning\",\"desc\":\"Setup WiFi via hotspot.\"},"
    "{\"icon\":\"🌐\",\"name\":\"Web Dashboard\",\"desc\":\"Status sistem di browser.\"},"
    "{\"icon\":\"🤖\",\"name\":\"AI Chatbot\",\"desc\":\"Chat teks & suara di /chat.\"},"
    "{\"icon\":\"🔊\",\"name\":\"Audio Input\",\"desc\":\"Speech-to-text & text-to-speech.\"},"
    "{\"icon\":\"👥\",\"name\":\"Kelola User\",\"desc\":\"Daftar UID ke nama di /users.\"},"
    "{\"icon\":\"🔌\",\"name\":\"API REST\",\"desc\":\"Endpoint /api/chat, /api/status, /api/features, /api/users.\"}"
    "]}";
}

String getStatusJSON() {
  String state = "Unknown";
  if (WiFi.status() == WL_CONNECTED) state = "Connected";
  else if (wifiProvisioningMode) state = "AP Mode";
  else state = "Disconnected";
  return "{\"uptime\":\"" + String(millis()/1000) + "s\",\"wifiState\":\"" + state + 
         "\",\"ip\":\"" + WiFi.localIP().toString() + 
         "\",\"freeRam\":" + String(ESP.getFreeHeap()) + "}";
}

void handleRoot() {
  if (wifiProvisioningMode) webServer.send(200, "text/html", SETUP_HTML);
  else webServer.send(200, "text/html", DASHBOARD_HTML);
}

void handleChatPage() {
  webServer.send(200, "text/html", CHAT_HTML);
}

void handleUsersPage() {
  webServer.send(200, "text/html", USERS_HTML);
}

void handleAPIChat() {
  if (webServer.method() != HTTP_POST) { webServer.send(405, "text/plain", "Method not allowed"); return; }
  String body = webServer.arg("plain");
  int idx = body.indexOf("\"message\":\"");
  if (idx == -1) { webServer.send(400, "text/plain", "No message"); return; }
  idx += 11; int end = body.indexOf("\"", idx);
  if (end == -1) { webServer.send(400, "text/plain", "Bad format"); return; }
  String msg = body.substring(idx, end);
  String reply = generateAIResponse(msg);
  webServer.send(200, "application/json", "{\"reply\":\"" + reply + "\"}");
}

void handleAPIStatus() {
  String state = WiFi.status() == WL_CONNECTED ? "Connected" : (wifiProvisioningMode ? "AP Mode" : "Disconnected");
  webServer.send(200, "application/json", "{\"uptime\":\"" + String(millis()/1000) + "s\",\"wifiState\":\"" + state + 
         "\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"freeRam\":" + String(ESP.getFreeHeap()) + "}");
}

void handleAPIFeatures() {
  webServer.send(200, "application/json", getFeaturesJSON());
}

void setupWebServer() {
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/chat", HTTP_GET, handleChatPage);
  webServer.on("/users", HTTP_GET, handleUsersPage);
  webServer.on("/config", HTTP_GET, handleConfigPage);
  webServer.on("/save", HTTP_POST, handleSaveConfig);
  webServer.on("/api/chat", HTTP_ANY, handleAPIChat);
  webServer.on("/api/status", HTTP_GET, handleAPIStatus);
  webServer.on("/api/features", HTTP_GET, handleAPIFeatures);
  webServer.on("/api/users", HTTP_GET, handleGetUsers);
  webServer.on("/api/users", HTTP_POST, handleRegisterUser);
  webServer.on("/api/users", HTTP_DELETE, handleDeleteUser);
  webServer.begin();
  Serial.println("[Web] Server ready on port 80");
}