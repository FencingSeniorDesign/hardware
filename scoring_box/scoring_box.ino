#include <TM1637Display.h>
#include <Adafruit_NeoPixel.h>

// ==== Fencer 1 Pin Definitions =====
const int A1_PIN = 22;
const int B1_PIN = 26;
const int C1_PIN = 30;

// ==== Fencer 2 Pin Definitions ====
const int A2_PIN = 25;
const int B2_PIN = 29;
const int C2_PIN = 33;

// ==== Output Devices ====
#define MATRIX_PIN_FENCER1 37
#define MATRIX_PIN_FENCER2 38
#define NUM_LEDS 64  // For 8x8 matrix

Adafruit_NeoPixel fencer1Matrix(NUM_LEDS, MATRIX_PIN_FENCER1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel fencer2Matrix(NUM_LEDS, MATRIX_PIN_FENCER2, NEO_GRB + NEO_KHZ800);
const int BUZZER_PIN = 34;

// ==== Timer Display Pins ====
#define CLK 42
#define DIO 41
TM1637Display display(CLK, DIO);

// ==== Fencer Score Display Pins ====
#define CLK1 46  // Fencer 1 Display Clock
#define DIO1 45  // Fencer 1 Display Data
#define CLK2 50  // Fencer 2 Display Clock
#define DIO2 49  // Fencer 2 Display Data
TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

// ==== Timing Config ====
const int TOUCH_DELAY = 2000;
const int DOUBLE_TOUCH_WINDOW = 40;
const int SERIAL_PRINT_INTERVAL = 50;

// ==== Button Config ====
const int BUTTON_PIN = 51;
unsigned long lastButtonPress = 0;
bool timerRunning = true;
int pressCount = 0;
unsigned long lastSerialPrint = 0;

// Timer Variables
unsigned long previousMillis = 0;
const long interval = 1000;
int remainingSeconds = 180;

// Passivity (non-combativity) timer
int passivitySeconds = 60;
bool passivityTimerRunning = false;

// Output hold management
bool outputActive = false;
unsigned long outputStartTime = 0;
bool buzzerAlreadyTriggered = false;

// Touch state tracking
bool fencer1NotificationSent = false;
bool fencer2NotificationSent = false;
bool doubleNotificationSent = false;

// Score tracking for status messages
int fencer1Score = 0;
int fencer2Score = 0;

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


  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize main timer display
  display.setBrightness(0x0f);
  
  // === Initialize NeoPixel Matrices ===
  fencer1Matrix.begin();
  fencer1Matrix.setBrightness(50); // adjust 0–255 as needed
  fencer1Matrix.show();            // Clear all LEDs

  fencer2Matrix.begin();
  fencer2Matrix.setBrightness(50);
  fencer2Matrix.show();            // Clear all LEDs
  
  // Initialize fencer score displays
  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  
  // Show initial scores
  updateScoreDisplays();
}

// Process commands received from ESP32
void processCommand(String message) {
  message.trim();  // Remove any whitespace
  
  // Special case for SET_SCORE which is case-sensitive
  if (message.startsWith("SET_SCORE:")) {
    Serial.println("Detected SET_SCORE command");
    // Handle score updates from the app (format: "SET_SCORE:1:2")
    // where 1 is fencer1Score and 2 is fencer2Score
    String scoreStr = message.substring(10);
    int colonPos = scoreStr.indexOf(':');
    
    Serial.print("Score string: ");
    Serial.println(scoreStr);
    Serial.print("Colon position: ");
    Serial.println(colonPos);
    
    if (colonPos > 0) {
      String score1Str = scoreStr.substring(0, colonPos);
      String score2Str = scoreStr.substring(colonPos + 1);
      
      Serial.print("Score1 string: ");
      Serial.println(score1Str);
      Serial.print("Score2 string: ");
      Serial.println(score2Str);
      
      int score1 = score1Str.toInt();
      int score2 = score2Str.toInt();
      
      Serial.print("Parsed scores: ");
      Serial.print(score1);
      Serial.print(":");
      Serial.println(score2);
      
      // Validate score values (must be non-negative)
      if (score1 >= 0 && score2 >= 0) {
        // Update local scores
        Serial.println("Updating scores...");
        fencer1Score = score1;
        fencer2Score = score2;
        
        // Update displays
        Serial.println("Calling updateScoreDisplays()...");
        updateScoreDisplays();
        
        Serial.print("Scores updated from app: ");
        Serial.print(fencer1Score);
        Serial.print(":");
        Serial.println(fencer2Score);
        
        Serial1.print("ACK: <scores updated to ");
        Serial1.print(fencer1Score);
        Serial1.print(":");
        Serial1.print(fencer2Score);
        Serial1.println(">");
      } else {
        Serial.println("Invalid score values (must be non-negative)");
        Serial1.println("ERROR: <invalid score values>");
      }
    } else {
      Serial.println("Invalid score format (should be SET_SCORE:X:Y)");
      Serial1.println("ERROR: <invalid score format>");
    }
    return;
  }
  
  // Convert to lowercase for case-insensitive command comparison
  String lowerMessage = message;
  lowerMessage.toLowerCase();
  
  if (lowerMessage == "start") {
    timerRunning = true;
    passivityTimerRunning = true;  // Start passivity timer with main timer
    Serial.println("Timer Started");
    Serial1.println("ACK: <start timer>");
    
    // Send the current timer value when starting
    long currentTimeMs = (long)remainingSeconds * 1000;
    String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":RUNNING";
    Serial1.println(timerStatus);
    
    // Send passivity timer status
    long passivityTimeMs = (long)passivitySeconds * 1000;
    String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":RUNNING";
    Serial1.println(passivityStatus);
  } 
  else if (lowerMessage == "stop") {
    timerRunning = false;
    passivityTimerRunning = false;  // Stop passivity timer with main timer
    Serial.println("Timer Stopped");
    Serial1.println("ACK: <stop timer>");
    
    // Send timer status
    long currentTimeMs = (long)remainingSeconds * 1000;
    String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":STOPPED";
    Serial1.println(timerStatus);
    
    // Send passivity timer status
    long passivityTimeMs = (long)passivitySeconds * 1000;
    String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":STOPPED";
    Serial1.println(passivityStatus);
  } 
  else if (lowerMessage.startsWith("reset:")) {
    timerRunning = false;             // Stop the timer on reset
    
    // Extract the time value after "reset:" (in milliseconds)
    String timeStr = lowerMessage.substring(6);
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
  else if (lowerMessage == "status") {
    // Send current state to app
    Serial.println("Sending status...");
    Serial.print("Current remainingSeconds: ");
    Serial.println(remainingSeconds);
    Serial.print("Timer running: ");
    Serial.println(timerRunning ? "YES" : "NO");
    
    // Send timer status
    long currentTimeMs = (long)remainingSeconds * 1000;
    String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":" + (timerRunning ? "RUNNING" : "STOPPED");
    Serial1.println(timerStatus);
    Serial.println("Sent: " + timerStatus);
    
    // Send passivity timer status
    long passivityTimeMs = (long)passivitySeconds * 1000;
    String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":" + (passivityTimerRunning ? "RUNNING" : "STOPPED");
    Serial1.println(passivityStatus);
    Serial.println("Sent: " + passivityStatus);
    
    // Send score status
    String scoreStatus = "STATUS:SCORE:" + String(fencer1Score) + ":" + String(fencer2Score);
    Serial1.println(scoreStatus);
    Serial.println("Sent: " + scoreStatus);
    
    Serial1.println("ACK: <status>");
  }
  else {
    // Optional: Print unrecognized commands
    Serial.print("Unrecognized Command: ");
    Serial.println(message);
  }
}

void loop() {
  // ===== UART Command Listener (Improved with CR/LF Support) =====
  static String esp32Message = "";
  
  // Process incoming Serial1 messages
  while (Serial1.available()) {
    char incomingByte = Serial1.read();
  
    if (incomingByte == '\n' || incomingByte == '\r') {
      if (esp32Message.length() > 0) {
        // Log the raw message
        Serial.print("Received raw message from ESP32: ");
        Serial.println(esp32Message);
        
        // Process the message based on its content
        processCommand(esp32Message);
        
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
      
      // Decrement first
      remainingSeconds--;
      
      // Update passivity timer if running
      if (passivityTimerRunning && passivitySeconds > 0) {
        passivitySeconds--;
      }
      
      // Update display to show the new value
      updateDisplay();
      
      // Send timer status with the decremented value
      long currentTimeMs = (long)remainingSeconds * 1000;
      String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":RUNNING";
      Serial1.println(timerStatus);
      
      // Send passivity timer status
      long passivityTimeMs = (long)passivitySeconds * 1000;
      String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":" + (passivityTimerRunning ? "RUNNING" : "STOPPED");
      Serial1.println(passivityStatus);
      
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
}

// ==== Button, Display, and Scoring Functions ====

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long now = millis();
    if (now - lastButtonPress < 500) {
      remainingSeconds = 180;
      timerRunning = false;
      passivitySeconds = 60;  // Reset passivity timer
      passivityTimerRunning = false;
      buzzerAlreadyTriggered = false;
      updateDisplay();
      pressCount = 0;
      
      // Send timer status after reset
      long currentTimeMs = (long)remainingSeconds * 1000;
      String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":STOPPED";
      Serial1.println(timerStatus);
      
      // Send passivity timer status
      long passivityTimeMs = (long)passivitySeconds * 1000;
      String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":STOPPED";
      Serial1.println(passivityStatus);
    } else {
      timerRunning = !timerRunning;
      passivityTimerRunning = timerRunning;  // Toggle passivity timer with main timer
      pressCount = 1;
      
      // Send timer status after toggle
      long currentTimeMs = (long)remainingSeconds * 1000;
      String timerStatus = "STATUS:TIMER:" + String(currentTimeMs) + ":" + (timerRunning ? "RUNNING" : "STOPPED");
      Serial1.println(timerStatus);
      
      // Send passivity timer status  
      long passivityTimeMs = (long)passivitySeconds * 1000;
      String passivityStatus = "STATUS:PASSIVITY:" + String(passivityTimeMs) + ":" + (passivityTimerRunning ? "RUNNING" : "STOPPED");
      Serial1.println(passivityStatus);
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

void updateScoreDisplays() {
  // Show scores on fencer displays
  display1.showNumberDec(fencer1Score, false);
  display2.showNumberDec(fencer2Score, false);
  
  // Print for debugging
  Serial.print("Updated score displays: Fencer1=");
  Serial.print(fencer1Score);
  Serial.print(", Fencer2=");
  Serial.println(fencer2Score);
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
  // Skip touch detection during active LED/buzzer period
  if (outputActive) {
    return;
  }
  
  int a1 = digitalRead(A1_PIN);
  int c2 = digitalRead(C2_PIN);
  int a2 = digitalRead(A2_PIN);
  int c1 = digitalRead(C1_PIN);

  bool fencer1Hit = (a1 == LOW && c2 == HIGH);
  bool fencer2Hit = (a2 == LOW && c1 == HIGH);

  // Check for new touches
  bool needsProcessing = false;
  
  if (fencer1Hit && fencer2Hit) {
    // Double touch - check if already notified
    if (!doubleNotificationSent) {
      needsProcessing = true;
    }
  } else if (fencer1Hit && !fencer1NotificationSent) {
    // Fencer 1 single touch - check if already notified
    needsProcessing = true;
  } else if (fencer2Hit && !fencer2NotificationSent) {
    // Fencer 2 single touch - check if already notified
    needsProcessing = true;
  }

  if (needsProcessing) {
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

    // Always set notification flags to prevent duplicate processing
    if (fencer1Scores && fencer2Scores) {
      doubleNotificationSent = true;
    } else if (fencer1Scores) {
      fencer1NotificationSent = true;
    } else if (fencer2Scores) {
      fencer2NotificationSent = true;
    }

    // Only send scores to app and increment score counter if timer is running
    if (timerRunning) {
      if (fencer1Scores && fencer2Scores) {
        Serial1.println("SCORE:DOUBLE");
        fencer1Score++;
        fencer2Score++;
      } else if (fencer1Scores) {
        Serial1.println("SCORE:FENCER1");
        fencer1Score++;
      } else if (fencer2Scores) {
        Serial1.println("SCORE:FENCER2");
        fencer2Score++;
      }
      
      // Update score displays whenever scores change
      updateScoreDisplays();
    }

    if (fencer1Scores) digitalWrite(LED1_PIN, HIGH);
    if (fencer2Scores) digitalWrite(LED2_PIN, HIGH);
    if (fencer1Scores || fencer2Scores) digitalWrite(BUZZER_PIN, HIGH);

    outputActive = true;
    outputStartTime = millis();
    timerRunning = false;
    
    // Reset and stop passivity timer on score
    passivitySeconds = 60;
    passivityTimerRunning = false;
  }
  
  // Reset notification flags when touches are released
  if (!fencer1Hit) {
    fencer1NotificationSent = false;
  }
  if (!fencer2Hit) {
    fencer2NotificationSent = false;
  }
  if (!fencer1Hit || !fencer2Hit) {
    doubleNotificationSent = false;
  }
}
