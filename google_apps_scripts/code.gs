/**
 * SISTEM ABSENSI IoT ESP32 - GOOGLE APPS SCRIPT
 * File ini menangani pembuat tabel database siswa, pencocokan UID RFID,
 * serta pencatatan log absensi secara real-time.
 */

// Fungsi untuk membuat dan me-format struktur Google Sheets secara otomatis
function setupSheet() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  
  // 1. Buat / Format Sheet 'Database Siswa'
  var studentSheet = ss.getSheetByName("Database Siswa");
  if (!studentSheet) {
    studentSheet = ss.insertSheet("Database Siswa");
    studentSheet.appendRow(["UID Kartu", "Nama Siswa", "Kelas / NIS"]);
    
    // Style Header
    studentSheet.getRange("A1:C1")
      .setFontWeight("bold")
      .setBackground("#1B5E20")
      .setFontColor("#FFFFFF");
      
    // Data Siswa Awal (Contoh)
    studentSheet.appendRow(["A3 45 B2 12", "Ahmad Sanusi", "XII IPA 1"]);
    studentSheet.appendRow(["93 FC 88 A1", "Siti Rahma", "XII IPA 2"]);
    studentSheet.appendRow(["12 34 56 78", "Budi Santoso", "XII IPS 1"]);
    studentSheet.appendRow(["E2 8A 91 C3", "Dewi Lestari", "XII IPS 2"]);
    
    studentSheet.autoResizeColumns(1, 3);
  }
  
  // 2. Buat / Format Sheet 'Log Absensi'
  var logSheet = ss.getSheetByName("Log Absensi");
  if (!logSheet) {
    logSheet = ss.insertSheet("Log Absensi");
    logSheet.appendRow(["Tanggal", "Waktu", "UID Kartu", "Nama Siswa", "Status"]);
    
    // Style Header
    logSheet.getRange("A1:E1")
      .setFontWeight("bold")
      .setBackground("#0D47A1")
      .setFontColor("#FFFFFF");
      
    logSheet.autoResizeColumns(1, 5);
  }
  
  // Hapus Sheet1 bawaan Google jika ada
  var defaultSheet = ss.getSheetByName("Sheet1");
  if (defaultSheet && ss.getSheets().length > 1) {
    ss.deleteSheet(defaultSheet);
  }
  
  Logger.log("Setup Sheet Berhasil Dibuat!");
}

// Fungsi HTTP GET yang dipanggil oleh ESP32
function doGet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var studentSheet = ss.getSheetByName("Database Siswa");
  var logSheet = ss.getSheetByName("Log Absensi");
  
  // Jika sheet belum di-setup, jalankan auto setup
  if (!studentSheet || !logSheet) {
    setupSheet();
    studentSheet = ss.getSheetByName("Database Siswa");
    logSheet = ss.getSheetByName("Log Absensi");
  }

  // Ambil parameter dari ESP32
  var uid = (e.parameter.uid || "").trim().toUpperCase();
  var reqDate = e.parameter.date || "";
  var reqTime = e.parameter.time || "";

  if (!uid) {
    return ContentService.createTextOutput(JSON.stringify({
      "registered": false,
      "message": "UID tidak ditemukan dalam request"
    })).setMimeType(ContentService.MimeType.JSON);
  }

  // Cari UID di Sheet 'Database Siswa'
  var studentData = studentSheet.getDataRange().getValues();
  var studentName = "Tidak Dikenal";
  var isRegistered = false;

  // Scan baris tabel (lewati baris 1 / header)
  for (var i = 1; i < studentData.length; i++) {
    var cardUID = String(studentData[i][0]).trim().toUpperCase();
    if (cardUID === uid) {
      studentName = String(studentData[i][1]).trim(); // Kolom Nama Siswa
      isRegistered = true;
      break;
    }
  }

  // Tentukan Tanggal & Waktu (Gunakan Server Time jika ESP32 tidak mengirim)
  var now = new Date();
  var dateStr = reqDate || Utilities.formatDate(now, "Asia/Jakarta", "yyyy-MM-dd");
  var timeStr = reqTime || Utilities.formatDate(now, "Asia/Jakarta", "HH:mm:ss");
  var statusStr = isRegistered ? "Hadir" : "Ditolak";

  // Catat riwayat ke Sheet 'Log Absensi'
  logSheet.appendRow([dateStr, timeStr, uid, studentName, statusStr]);

  // Respon balik ke ESP32 dalam format JSON
  var responsePayload = {
    "registered": isRegistered,
    "uid": uid,
    "name": studentName,
    "status": statusStr,
    "date": dateStr,
    "time": timeStr
  };

  return ContentService.createTextOutput(JSON.stringify(responsePayload))
    .setMimeType(ContentService.MimeType.JSON);
}