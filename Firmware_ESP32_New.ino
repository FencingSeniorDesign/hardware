#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ==== UUID Definitions ====
#define SERVICE_UUID                 "12345678-1234-5678-1234-56789abcdef0"
#define CONTROL_CHARACTERISTIC_UUID  "12345678-1234-5678-1234-56789abcde01"
#define STATUS_CHARACTERISTIC_UUID   "12345678-1234-5678-1234-56789abcde02"

// BLE server and characteristics
BLEServer* pServer = nullptr;
BLECharacteristic* pControlCharacteristic = nullptr;
BLECharacteristic* pStatusCharacteristic = nullptr;

// ==== BLE Write Callback Class (using Arduino String) ====
class ControlCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String rxValue = String(pCharacteristic->getValue().c_str());
    if (rxValue.length() > 0) {
      Serial1.println(rxValue);  // Forward the value to the Mega
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println(">>> Firmware setup started <<<");

  Serial1.begin(9600);
  Serial.println(">>> Serial1 initialized <<<");

  Serial.println(">>> Initializing BLE Device <<<");
  BLEDevice::init("FenceBox_01");

  Serial.println(">>> Creating BLE Server <<<");
  pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(SERVICE_UUID);

  Serial.println(">>> Creating Control Characteristic <<<");
  pControlCharacteristic = pService->createCharacteristic(
                             CONTROL_CHARACTERISTIC_UUID,
                             BLECharacteristic::PROPERTY_WRITE
                           );
  pControlCharacteristic->setCallbacks(new ControlCallback());

  Serial.println(">>> Creating Status Characteristic <<<");
  pStatusCharacteristic = pService->createCharacteristic(
                            STATUS_CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_NOTIFY
                          );

  Serial.println(">>> Starting BLE Service <<<");
  pService->start();

  Serial.println(">>> Starting BLE Advertising <<<");
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println(">>> BLE Server started. Waiting for connections... <<<");
}

void loop() {
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      pStatusCharacteristic->setValue(msg.c_str());
      pStatusCharacteristic->notify();
      Serial.print("Forwarded to app: ");
      Serial.println(msg);
    }
  }
  delay(10);
}
