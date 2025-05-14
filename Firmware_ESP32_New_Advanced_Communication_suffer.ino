#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ==== UUID Definitions ===
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CONTROL_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
#define STATUS_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef2"

BLEServer* pServer = nullptr;
BLECharacteristic* controlCharacteristic = nullptr;
BLECharacteristic* statusCharacteristic = nullptr;

// ==== BLE Write Callback Class ====
class ControlCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String rxValue = String(pCharacteristic->getValue().c_str());
    if (rxValue.length() > 0) {
      Serial.print("Forwarded to Mega: ");
      Serial.println(rxValue);
      Serial2.println(rxValue);
    }
  }
};

void setup() {
  // Begin Serial Monitor for debug output
  Serial.begin(115200);
  Serial.println(">>> Firmware setup started <<<");

  // Communication to Mega via Serial2 (TX2=GPIO17, RX2=GPIO16)
  Serial2.begin(115200);  // Corrected baud rate to match Arduino Mega

  // Initialize BLE device
  BLEDevice::init("FencBox_01");

  // Create BLE Server
  pServer = BLEDevice::createServer();

  // Create BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic with Notify and Write properties
  controlCharacteristic = pService->createCharacteristic(
    CONTROL_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  // Set the callback for the characteristic
  controlCharacteristic->setCallbacks(new ControlCharacteristicCallbacks());

  // Start the BLE service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println(">>> BLE Server started. Waiting for connections... <<<");
}

void loop() {
  // Optional: Listen for responses from Mega and print them to Serial Monitor
  if (Serial2.available() > 0) {
    String megaResponse = Serial2.readString();
    Serial.print("Response from Mega: ");
    Serial.println(megaResponse);

    // Optional: Send responses to BLE clients (uncomment if needed)
    // statusCharacteristic->setValue(megaResponse.c_str());
    // statusCharacteristic->notify();
  }

  delay(10);  // Small delay for stability
    // ===== Heartbeat to Mega =====
  Serial2.println("ESP Detected");

  // Listen for responses from Mega
  if (Serial2.available()) {
    String megaResponse = Serial2.readString();
    Serial.print("Response from Mega: ");
    Serial.println(megaResponse);
  }

  delay(1000);  // Wait 1 second
}

