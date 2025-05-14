void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX2 = GPIO16, TX2 = GPIO17
  Serial.println("ESP32 Serial2 Loopback Test Started");
}

void loop() {
  Serial2.println("Loopback Test Message");

  if (Serial2.available()) {
    String received = Serial2.readStringUntil('\n');
    Serial.print("Received on Serial2: ");
    Serial.println(received);
  }

  delay(1000); // Repeat every second
}
