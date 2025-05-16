#include <TM1637Display.h>

// ==== Fencer 1 Pin Definitions ======
const int A1_PIN = 22;
const int B1_PIN = 24;
const int C1_PIN = 26;

// ==== Fencer 2 Pin Definitions ====
const int A2_PIN = 23;
const int B2_PIN = 25;
const int C2_PIN = 27;

// ==== Output Devices ====
const int LED1_PIN = 30;
const int LED2_PIN = 32;
const int BUZZER_PIN = 34;

// ==== Timer Display Pins ====
#define CLK 4
#define DIO 5
TM1637Display display(CLK, DIO);

// ==== Timing Config ====
const int TOUCH_DELAY = 2000;
const int DOUBLE_TOUCH_WINDOW = 40;
const int SERIAL_PRINT_INTERVAL = 50;

// ==== Button Config ====
const int BUTTON_PIN = 33;
unsigned long lastButtonPress = 0;
bool timerRunning = true;
int pressCount = 0;
unsigned long lastSerialPrint = 0;

// Timer Variables
unsigned long previousMillis = 0;
const long interval = 1000;
int remainingSeconds = 180;

// Output hold management
bool outputActive = false;
unsigned long outputStartTime = 0;
bool buzzerAlreadyTriggered = false;

void setup() {
  Serial.begin(115200);      // Serial Monitor for debugging
  Serial1.begin(115200);   // UART for ESP32 communication
  Serial.println("Listening for ESP32 commands...");

  pinMode(B1_PIN, OUTPUT); digitalWrite(B1_PIN, LOW);
  pinMode(B2_PIN, OUTPUT); digitalWrite(B2_PIN, LOW);

  pinMode(A1_PIN, INPUT_PULLUP);
  pinMode(C2_PIN, INPUT_PULLUP);
  pinMode(A2_PIN, INPUT_PULLUP);
  pinMode(C1_PIN, INPUT_PULLUP);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  display.setBrightness(0x0f);
}

void loop() {
// ===== UART Command Listener (Improved with CR/LF Support) =====
static String esp32Message = "";

while (Serial1.available()) {
  char incomingByte = Serial1.read();

  if (incomingByte == '\n' || incomingByte == '\r') {
    if (esp32Message.length() > 0) {
      Serial.print("Received message from ESP32: ");
      Serial.println(esp32Message);
      esp32Message = "";  // Clear buffer for next message
    }
  } else {
    esp32Message += incomingByte;  // Add to buffer
  }
}


  // ===== Existing Timer Logic =====
  handleButton();

  if (timerRunning && remainingSeconds > 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      updateDisplay();
      remainingSeconds--;
      if (remainingSeconds == 0 && !buzzerAlreadyTriggered) {
        triggerEndBuzzer();
      }
    }
  }

  if (outputActive && millis() - outputStartTime >= TOUCH_DELAY) {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    outputActive = false;
  }

  handleFencingLogic();
  
   // Example: Read incoming message from Serial1 (connected to ESP32)
if (Serial1.available()) {
    String incomingMessage = Serial1.readStringUntil('\n');
    incomingMessage.trim();              // Remove whitespace/newlines
    incomingMessage.toLowerCase();       // Convert to lowercase
    incomingMessage.trim();  // Remove any whitespace or newline characters

    if (incomingMessage == "start") {
        timerRunning = true;
        Serial.println("Timer Started");
        Serial1.println("ACK: <start timer>");
    } 
    else if (incomingMessage == "stop") {
        timerRunning = false;
        Serial.println("Timer Stopped");
        Serial1.println("ACK: <stop timer>");
    } 
    else if (incomingMessage.startsWith("reset:")) {
        timerRunning = false;             // Stop the timer on reset
        
        // Extract the time value after "reset:" (in milliseconds)
        String timeStr = incomingMessage.substring(6);
        long newTimeMs = timeStr.toInt();
        
        // Validate time (must be between 1000ms and 3600000ms / 1 sec to 60 minutes)
        if (newTimeMs >= 1000 && newTimeMs <= 3600000) {
            remainingSeconds = newTimeMs / 1000;  // Convert to seconds
            int minutes = remainingSeconds / 60;
            int seconds = remainingSeconds % 60;
            Serial.print("Timer Reset to ");
            Serial.print(minutes);
            Serial.print(":");
            if (seconds < 10) Serial.print("0");
            Serial.print(seconds);
            Serial.print(" (");
            Serial.print(newTimeMs);
            Serial.println("ms)");
            Serial1.print("ACK: <reset timer to ");
            Serial1.print(newTimeMs);
            Serial1.println("ms>");
        } else {
            // Invalid time value
            Serial.print("Invalid time value: ");
            Serial.print(newTimeMs);
            Serial.println("ms. Must be between 1000-3600000ms");
            Serial1.println("ERROR: <reset timer> (invalid time)");
        }
        
        updateDisplay();                  // Update the display
    } 
    else {
        // Optional: Print unrecognized commands
        Serial.print("Unrecognized Command: ");
        Serial.println(incomingMessage);
    }
}

}


// ==== Button, Display, and Scoring Functions ====

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long now = millis();
    if (now - lastButtonPress < 500) {
      remainingSeconds = 180;
      timerRunning = false;
      buzzerAlreadyTriggered = false;
      updateDisplay();
      pressCount = 0;
    } else {
      timerRunning = !timerRunning;
      pressCount = 1;
    }
    lastButtonPress = now;
    while (digitalRead(BUTTON_PIN) == LOW);
    delay(50);
  }
}

void updateDisplay() {
  int minutes = remainingSeconds / 60;
  int seconds = remainingSeconds % 60;
  int displayTime = minutes * 100 + seconds;
  display.showNumberDecEx(displayTime, 0b01000000, true);
}

void triggerEndBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  outputActive = true;
  outputStartTime = millis();
  buzzerAlreadyTriggered = true;
  timerRunning = false;
  Serial.println(">>> Timer ended. Buzzer activated.");
}

void handleFencingLogic() {
  int a1 = digitalRead(A1_PIN);
  int c2 = digitalRead(C2_PIN);
  int a2 = digitalRead(A2_PIN);
  int c1 = digitalRead(C1_PIN);

  bool fencer1Hit = (a1 == LOW && c2 == HIGH);
  bool fencer2Hit = (a2 == LOW && c1 == HIGH);

  if (fencer1Hit || fencer2Hit) {
    bool fencer1Scores = false;
    bool fencer2Scores = false;

    if (fencer1Hit && fencer2Hit) {
      Serial.println(">>> Double touch!");
      fencer1Scores = true;
      fencer2Scores = true;
    } else if (fencer1Hit) {
      delay(DOUBLE_TOUCH_WINDOW);
      int a2b = digitalRead(A2_PIN);
      int c1b = digitalRead(C1_PIN);
      if (a2b == LOW && c1b == HIGH) {
        Serial.println(">>> Double touch (Fencer 2 delay)!");
        fencer1Scores = true;
        fencer2Scores = true;
      } else {
        Serial.println(">>> Fencer 1 scores solo.");
        fencer1Scores = true;
      }
    } else if (fencer2Hit) {
      delay(DOUBLE_TOUCH_WINDOW);
      int a1b = digitalRead(A1_PIN);
      int c2b = digitalRead(C2_PIN);
      if (a1b == LOW && c2b == HIGH) {
        Serial.println(">>> Double touch (Fencer 1 delay)!");
        fencer1Scores = true;
        fencer2Scores = true;
      } else {
        Serial.println(">>> Fencer 2 scores solo.");
        fencer2Scores = true;
      }
    }

    // Send scoring messages to ESP32
    if (fencer1Scores && fencer2Scores) {
      Serial1.println("SCORE:DOUBLE");
    } else if (fencer1Scores) {
      Serial1.println("SCORE:FENCER1");
    } else if (fencer2Scores) {
      Serial1.println("SCORE:FENCER2");
    }

    if (fencer1Scores) digitalWrite(LED1_PIN, HIGH);
    if (fencer2Scores) digitalWrite(LED2_PIN, HIGH);
    if (fencer1Scores || fencer2Scores) digitalWrite(BUZZER_PIN, HIGH);

    outputActive = true;
    outputStartTime = millis();
    timerRunning = false;
  }
}
