#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// PN532 SPI-Pins
#define PN532_SCK  (13)
#define PN532_MISO (14)
#define PN532_MOSI (15)
#define PN532_SS   (16)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// UID-Puffer
uint8_t uid[7];
uint8_t uidLength;
uint8_t lastUID[7] = {0};
uint8_t lastUIDLength = 0;

bool cardPreviouslyDetected = false;

void printUID(uint8_t *uid, uint8_t length) {
  Serial.print("🆔 UID: ");
  for (uint8_t i = 0; i < length; i++) {
    Serial.print(uid[i], HEX);
    if (i < length - 1) Serial.print(":");
  }
  Serial.println();
}

bool compareUIDs(uint8_t *uid1, uint8_t len1, uint8_t *uid2, uint8_t len2) {
  if (len1 != len2) return false;
  for (uint8_t i = 0; i < len1; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void interpretBlock(uint8_t *data) {
  // Ausgabe als ASCII-Zeichenkette (lesbarer Text)
  Serial.print("🔤 ASCII: ");
  for (int i = 0; i < 16; i++) {
    if (isPrintable(data[i])) {
      Serial.print((char)data[i]);
    } else {
      Serial.print('.');
    }
  }
  Serial.println();

  // Beispiel: Interpretiere ersten 4 Byte als uint32_t (Little Endian)
  uint32_t value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  Serial.print("🔢 Zahl (Little Endian, 4 Byte): ");
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("📡 Initialisiere PN532...");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("❌ Kein PN532 gefunden!");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("📲 Warte auf Karte...");
}

void loop() {
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (success) {
    if (!cardPreviouslyDetected || !compareUIDs(uid, uidLength, lastUID, lastUIDLength)) {
      cardPreviouslyDetected = true;
      memcpy(lastUID, uid, uidLength);
      lastUIDLength = uidLength;

      Serial.println("📶 Neue Karte erkannt!");
      printUID(uid, uidLength);

      uint8_t keya[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
      if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
        uint8_t data[16];
        if (nfc.mifareclassic_ReadDataBlock(4, data)) {
          Serial.print("📦 Block 4 (Hex): ");
          for (int i = 0; i < 16; i++) {
            Serial.printf("%02X ", data[i]);
          }
          Serial.println();

          interpretBlock(data); // interpretiere Blockinhalt
        } else {
          Serial.println("❌ Fehler beim Lesen von Block 4");
        }
      } else {
        Serial.println("🔒 Authentifizierung fehlgeschlagen");
      }
    }
  } else {
    if (cardPreviouslyDetected) {
      cardPreviouslyDetected = false;
      Serial.println("📴 Karte entfernt");
    }
  }

  delay(100);
}
