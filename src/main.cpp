#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

namespace {
constexpr uint8_t RFID_SS_PIN = 5;
constexpr uint8_t RFID_RST_PIN = 4;
constexpr uint8_t LED_PIN = 2;

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

void printUid() {
  Serial.print("UID: ");
  for (byte index = 0; index < rfid.uid.size; index++) {
    if (rfid.uid.uidByte[index] < 0x10) {
      Serial.print('0');
    }
    Serial.print(rfid.uid.uidByte[index], HEX);
    if (index + 1 < rfid.uid.size) {
      Serial.print(':');
    }
  }
  Serial.println();
}

void blinkCardDetected() {
  for (uint8_t count = 0; count < 3; count++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
}
} // namespace

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(500);
  Serial.println();
  Serial.println("=== TES MODUL RFID RC522 ===");
  Serial.println("SPI: SCK=18, MISO=19, MOSI=23, SS=5, RST=4");

  SPI.begin(18, 19, 23, RFID_SS_PIN);
  rfid.PCD_Init();
  delay(100);

  Serial.print("Versi firmware RC522: 0x");
  rfid.PCD_DumpVersionToSerial();
  Serial.println("Tempelkan kartu RFID...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.println("Kartu terdeteksi!");
  printUid();
  blinkCardDetected();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  Serial.println("Tempelkan kartu RFID berikutnya...");
  delay(500);
}
