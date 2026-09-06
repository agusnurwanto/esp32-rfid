#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace {
constexpr uint8_t RFID_SS_PIN = 5;
constexpr uint8_t RFID_RST_PIN = 4;
constexpr uint8_t LED_PIN = 2;
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void showDisplay(const char *line1, const char *line2 = "", const char *line3 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(line1);
  oled.setCursor(0, 20);
  oled.println(line2);
  oled.setCursor(0, 40);
  oled.println(line3);
  oled.display();
}

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

void showUidOnDisplay() {
  char uidText[24] = {};
  size_t offset = 0;

  for (byte index = 0; index < rfid.uid.size && offset < sizeof(uidText) - 1; index++) {
    int written = snprintf(uidText + offset, sizeof(uidText) - offset,
                           index == 0 ? "%02X" : ":%02X", rfid.uid.uidByte[index]);
    if (written < 0) {
      break;
    }
    offset += static_cast<size_t>(written);
  }

  char firstLine[22] = {};
  char secondLine[22] = {};
  strncpy(firstLine, uidText, 21);
  if (strlen(uidText) > 21) {
    strncpy(secondLine, uidText + 21, sizeof(secondLine) - 1);
  }
  showDisplay("Kartu terdeteksi!", firstLine, secondLine);
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

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED gagal diinisialisasi.");
  } else {
    showDisplay("TES RFID RC522", "Memulai...");
  }

  SPI.begin(18, 19, 23, RFID_SS_PIN);
  rfid.PCD_Init();
  delay(100);

  Serial.print("Versi firmware RC522: 0x");
  rfid.PCD_DumpVersionToSerial();
  showDisplay("RC522 siap", "Tempelkan kartu");
  Serial.println("Tempelkan kartu RFID...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.println("Kartu terdeteksi!");
  printUid();
  showUidOnDisplay();
  blinkCardDetected();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  Serial.println("Tempelkan kartu RFID berikutnya...");
  showDisplay("RC522 siap", "Tempelkan kartu");
  delay(500);
}
