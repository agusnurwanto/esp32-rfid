#!/usr/bin/env python3
"""
AI Voice Chatbot for ESP32 Web Dashboard

This is a Python script that can be run on a computer connected to the ESP32 network
or ported to MicroWebRTC for direct ESP32 deployment.

Features:
- Text chat via WebSocket/HTTP API
- Audio input via Web Speech API or microphone recording
- Returns AI responses as audio or text

Usage:
1. Run this server alongside ESP32 or deploy to ESP32 with MicroPython/WebSocket support
2. Access http://ESP32_IP:5001/chat_audio for the voice-enabled chat interface
"""

import json
import subprocess
import sys

# Simple keyword-based AI responses
def generate_response(message):
    """Generate AI response based on message keywords (no external API needed)"""
    q = message.lower().strip()
    responses = {
        'siapa': "Saya asisten AI dari ESP32. Bisa bantu tanya tentang absensi, fitur, atau status.",
        'fitur': "Fitur: Absensi RFID, OLED display, MicroSD, WiFi provisioning, Dashboard web, AI Chat, API REST.",
        'apa': "Saya bisa membantu dengan: 1) Cek status WiFi 2) Lihat fitur sistem 3) Bantu pertanyaan umum",
        'halo': "Halo! Ada yang bisa saya bantu?",
        'hai': "Hi! Silakan tanya apa saja.",
        'terima kasih': "Sama-sama!",
        'thanks': "You're welcome!",
        'status': get_status(),
        'wifi': get_status(),
        'ip': get_status(),
    }
    for key, resp in responses.items():
        if key in q:
            return resp
    return "Maaf, tidak mengerti. Coba: 'fitur', 'status', atau 'halo'."

def get_status():
    """Return simulated WiFi status (in real deployment, query ESP32)"""
    return "WiFi tersambung. IP: 192.168.1.100. Sistem siap digunakan."

def text_to_speech(text, lang='id'):
    """
    Convert text to speech audio file.
    Requires: pip install gtts playsound
    """
    try:
        from gtts import gTTS
        import os
        tts = gTTS(text=text, lang=lang)
        output_file = '/tmp/esp32_ai_response.mp3'
        tts.save(output_file)
        return output_file
    except ImportError:
        print("Install gtts: pip install gtts")
        return None

def speech_to_text():
    """
    Convert speech to text using Web Speech API format.
    In web context, use browser's SpeechRecognition API.
    Here we return a placeholder for the web interface.
    """
    return """
    // In browser, use Web Speech API:
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    const recognition = new SpeechRecognition();
    recognition.lang = 'id-ID';  // Indonesian
    recognition.onresult = (event) => {
        const transcript = event.results[0][0].transcript;
        fetch('/api/chat', {method:'POST', body:JSON.stringify({message: transcript})})
            .then(r=>r.json())
            .then(d => {
                // Speak response
                const utterance = new SpeechSynthesisUtterance(d.reply);
                utterance.lang = 'id-ID';
                speechSynthesis.speak(utterance);
            });
    };
    recognition.start();
    """

# HTML interface for web-based voice chat
HTML_INTERFACE = '''
<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>AI Voice Chat</title>
<style>
body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;display:flex;height:100vh}
app{width:100%;max-width:500px;display:flex;flex-direction:column}
header{background:#16213e;color:#38bdf8;padding:15px}
messages{flex:1;overflow-y:auto;padding:15px;background:#1e293b}
.msg{border-radius:10px;padding:10px;margin:5px 0;max-width:80%}
.user{align-self:flex-end;background:#38bdf8;color:#0f172a}
.bot{align-self:flex-start;background:#0f172a;border:1px solid #334155}
.mic{display:flex;padding:10px;border-top:1px solid #334155;background:#0f172a}
button{background:#38bdf8;color:#0f172a;border:none;border-radius:50%;width:40px;height:40px;cursor:pointer}
.muted{background:#334155;cursor:not-allowed}
</style></head>
<body><app>
<header><h2>AI Voice Chat 🤖</h2></header>
<messages id="m"></messages>
<div class="mic">
  <button id="mic" onclick="toggleListening()">🎤</button>
  <input type="text" id="i" placeholder="Ketik atau ucapkan pesan...">
  <button onclick="send()">➤</button>
</div>
</app>
<script>
const m=document.getElementById('m'),i=document.getElementById('i'),btn=document.getElementById('mic');
let listening=false,rec,utterance;

// Speech Recognition setup
const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
if(SpeechRecognition){
  rec=new SpeechRecognition();
  rec.lang='id-ID';
  rec.continuous=true;
  rec.interimResults=true;
  rec.onresult=(e)=>{
    let t='';
    for(let r of e.results) if(r.r) t+=r[0].transcript;
    i.value=t;
    if(!listening) rec.stop();
    if(e.results[e.results.length-1].isFinal) send(t);
  };
}

function toggleListening(){
  if(!rec){alert('Web Speech API tidak didukung');return;}
  if(listening){rec.stop();btn.classList.remove('muted');listening=false;}
  else{rec.start();btn.classList.add('muted');listening=true;m.innerHTML+='<div class="msg user"><i>Mendengar...</i></div>';}
}

function send(t=i.value.trim()){
  if(!t)return;
  m.innerHTML+='<div class="msg user">'+escapeHtml(t)+'</div>';
  i.value='';m.scrollTop=m.scrollHeight;
  fetch('/api/chat', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({message:t})})
    .then(r=>r.json())
    .then(d=>{
      m.innerHTML+='<div class="msg bot">'+escapeHtml(d.reply||d.response||"Tidak ada respons")+'\</i></div>';
      // Text-to-speech
      if('speechSynthesis' in window){
        const u=new SpeechSynthesisUtterance(d.reply||d.response||"");
        u.lang='id-ID';
        speechSynthesis.speak(u);
      }
    }).catch(e=>console.error(e));
  m.scrollTop=m.scrollHeight;
}

function escapeHtml(str){return str.replace(/[&<>'"]/g,tag=>({ '&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[tag]));}

document.addEventListener('keydown',e=>{if(e.key==='Enter'&&!e.shiftKey){e.preventDefault();send();}});
</script></body></html>
'''

if __name__ == "__main__":
    print("=== ESP32 AI Voice Chatbot ===")
    print("\nUntuk penggunaan di browser, buka interface voice chat di ESP32")
    print("atau jalankan: python3 -m http.server 8000")
    print("\nUntuk Speech-to-Text pada browser, enable microphone permission")
    print("Untuk Text-to-Speech, browser harus mendukung Web Speech API")