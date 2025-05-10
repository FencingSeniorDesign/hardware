#include <TM1637Display.h>

// ==== Fencer 1 Pin Definition ====
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
bool buzzerAlreadyTriggered = false;  // Ensure buzzer only triggers once at timer end

void setup() {
  Serial.begin(9600);

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
  handleButton();

  // Timer countdown if running and not paused
  if (timerRunning && remainingSeconds > 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      updateDisplay();
      remainingSeconds--;

      // Check if timer reached 0 after decrement
      if (remainingSeconds == 0 && !buzzerAlreadyTriggered) {
        triggerEndBuzzer();
      }
    }
  }

  // Manage output duration without blocking
  if (outputActive && millis() - outputStartTime >= TOUCH_DELAY) {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    outputActive = false;
  }

  handleFencingLogic();
}

// Handle button press for start/stop and reset functionality
void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long now = millis();

    if (now - lastButtonPress < 500) {
      // Double press detected: Reset timer
      remainingSeconds = 180;
      timerRunning = false;
      buzzerAlreadyTriggered = false;
      updateDisplay();
      pressCount = 0;
    } else {
      // Single press toggles timer state
      timerRunning = !timerRunning;
      pressCount = 1;
    }

    lastButtonPress = now;

    // Debounce
    while (digitalRead(BUTTON_PIN) == LOW);
    delay(50);
  }
}

// Update the display with the remaining time
void updateDisplay() {
  int minutes = remainingSeconds / 60;
  int seconds = remainingSeconds % 60;
  int displayTime = minutes * 100 + seconds;
  display.showNumberDecEx(displayTime, 0b01000000, true);
}

// Trigger buzzer when timer hits 0
void triggerEndBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  outputActive = true;
  outputStartTime = millis();
  buzzerAlreadyTriggered = true;
  timerRunning = false;  // Ensure timer stays stopped
  Serial.println(">>> Timer ended. Buzzer activated.");
}

// Evaluate and process fencing scoring logic
void handleFencingLogic() {
  int a1 = digitalRead(A1_PIN);
  int c2 = digitalRead(C2_PIN);
  int a2 = digitalRead(A2_PIN);
  int c1 = digitalRead(C1_PIN);

  if (millis() - lastSerialPrint >= SERIAL_PRINT_INTERVAL) {
    Serial.print("A1: "); Serial.print(a1);
    Serial.print(" C2: "); Serial.print(c2);
    Serial.print(" || A2: "); Serial.print(a2);
    Serial.print(" C1: "); Serial.println(c1);
    lastSerialPrint = millis();
  }

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

    // Activate outputs and pause timer
    if (fencer1Scores) digitalWrite(LED1_PIN, HIGH);
    if (fencer2Scores) digitalWrite(LED2_PIN, HIGH);
    if (fencer1Scores || fencer2Scores) digitalWrite(BUZZER_PIN, HIGH);

    outputActive = true;
    outputStartTime = millis();

    timerRunning = false;  // Pause timer after scoring
  }
}
