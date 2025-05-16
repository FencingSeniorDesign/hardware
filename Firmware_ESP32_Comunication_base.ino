#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ==== UUID Definitions ====
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CONTROL_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
#define STATUS_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef2"

BLEServer* pServer = nullptr;
BLECharacteristic* controlCharacteristic = nullptr;
BLECharacteristic* statusCharacteristic = nullptr;
bool deviceConnected = false;

// ==== BLE Server Callbacks ====
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Client Connected");
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Client Disconnected");
    deviceConnected = false;
    // Restart advertising immediately when disconnected
    delay(500); // Give the BLE stack time to clean up
    pServer->startAdvertising();
    Serial.println("Restarted advertising");
  }
};

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
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // Explicit pin assignment

  // Initialize BLE device
  BLEDevice::init("FencBox_01");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

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
  // Listen for messages from Mega
  if (Serial2.available()) {
    String megaMessage = Serial2.readStringUntil('\n');
    megaMessage.trim();
    
    Serial.print("Received from Mega: ");
    Serial.println(megaMessage);
    
    // Check if this is a scoring message and forward it via BLE
    if (megaMessage.startsWith("SCORE:")) {
      if (deviceConnected && controlCharacteristic != nullptr) {
        controlCharacteristic->setValue(megaMessage.c_str());
        controlCharacteristic->notify();
        Serial.print("BLE Notification sent: ");
        Serial.println(megaMessage);
      }
    }
    // Forward ACK messages as well
    else if (megaMessage.startsWith("ACK:")) {
      if (deviceConnected && controlCharacteristic != nullptr) {
        controlCharacteristic->setValue(megaMessage.c_str());
        controlCharacteristic->notify();
        Serial.print("BLE ACK forwarded: ");
        Serial.println(megaMessage);
      }
    }
  }

  // Optional: small delay to reduce CPU usage
  delay(10);
}




