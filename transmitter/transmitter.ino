#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// ============================================================================
// CONFIGURATION - Set your pedal mode here
// ============================================================================
// Pedal mode options:
//   0 = DUAL_PEDAL (both LEFT and RIGHT pedals active)
//   1 = SINGLE_PEDAL_1 (LEFT pedal only)
//   2 = SINGLE_PEDAL_2 (RIGHT pedal only)
#define PEDAL_MODE 1  // Change this to 0, 1, or 2
// ============================================================================

#define SINGLE_PEDAL_PIN 13  // GPIO pin for single pedal mode (used in SINGLE_PEDAL_1 and SINGLE_PEDAL_2)
#define DUAL_LEFT_PIN 13     // GPIO pin for LEFT pedal in dual pedal mode
#define DUAL_RIGHT_PIN 14   // GPIO pin for RIGHT pedal in dual pedal mode
#define LED_PIN 2 // Pin that controls our board's built-in LED.  Your board may not have this feature.  I recommend the FireBeetle ESP32 for maximum battery life.
#define INACTIVITY_TIMEOUT 600000  // 10 Minute inactivity timeout in milliseconds.  Change as you like.
#define DEBOUNCE_DELAY 50  // Debounce delay in milliseconds to prevent contact bounce
#define LEFT_PEDAL_KEY 'l'   // Key to send for left pedal
#define RIGHT_PEDAL_KEY 'r'  // Key to send for right pedal

// Pedal mode constants
#define DUAL_PEDAL 0
#define SINGLE_PEDAL_1 1
#define SINGLE_PEDAL_2 2

// Structure to send pedal events
typedef struct __attribute__((packed)) struct_message {
  char key;       // Key character to press ('l' or 'r')
  bool pressed;   // true = press, false = release
} struct_message;

unsigned long lastActivityTime = 0;  // Last activity timestamp

// Pedal state tracking structure
struct PedalState {
  bool lastState;
  unsigned long debounceTime;
  bool debouncing;
};

PedalState leftPedal = {HIGH, 0, false};
PedalState rightPedal = {HIGH, 0, false};

// ESPNOW peer address - MUST match the receiver's MAC address
// Receiver MAC: a0:85:e3:e0:8e:a8
uint8_t broadcastAddress[] = {0xa0, 0x85, 0xe3, 0xe0, 0x8e, 0xa8};

uint64_t ext1_wakeup_mask = 1ULL << DUAL_LEFT_PIN | (1ULL << DUAL_RIGHT_PIN); // IF your board is an ESP32-S2, ESP32-S3, ESP32-C6 or ESP32-H2, this will allow a wakeup from BOTH pedals.

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP-NOW Pedal Transmitter");
  
  // Print pedal mode configuration
  Serial.print("Pedal Mode: ");
  if (PEDAL_MODE == DUAL_PEDAL) {
    Serial.println("DUAL PEDAL (both LEFT and RIGHT)");
  } else if (PEDAL_MODE == SINGLE_PEDAL_1) {
    Serial.println("SINGLE PEDAL 1 (LEFT only)");
  } else if (PEDAL_MODE == SINGLE_PEDAL_2) {
    Serial.println("SINGLE PEDAL 2 (RIGHT only)");
  }

  // Configure pins based on pedal mode
  if (PEDAL_MODE == DUAL_PEDAL) {
    pinMode(DUAL_LEFT_PIN, INPUT_PULLUP);   // Pin 13 - LEFT pedal in dual mode
    pinMode(DUAL_RIGHT_PIN, INPUT_PULLUP);  // Pin 14 - RIGHT pedal in dual mode
  } else {
    pinMode(SINGLE_PEDAL_PIN, INPUT_PULLUP);  // Pin 13 - single pedal mode
  }
  pinMode(LED_PIN, OUTPUT);  // Controls built-in LED.  Your board may not have this feature.
 
  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW.");
    return;
  }
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  // Default channel
  peerInfo.encrypt = false;  // No encryption
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW initialized successfully");
  Serial.print("Receiver MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(broadcastAddress[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Set initial activity time
  lastActivityTime = millis();
}

// Helper function to handle pedal state changes
void handlePedal(uint8_t pin, PedalState& pedal, char key, const char* pedalName) {
  bool currentState = digitalRead(pin);
  unsigned long currentTime = millis();
  
  if (currentState == LOW && pedal.lastState == HIGH) {
    // Pressed - start debounce
    if (!pedal.debouncing) {
      pedal.debounceTime = currentTime;
      pedal.debouncing = true;
    } else if (currentTime - pedal.debounceTime >= DEBOUNCE_DELAY) {
      // Debounce complete, verify still LOW
      if (digitalRead(pin) == LOW) {
        sendPedalEvent(key, true, pedalName);
        pedal.lastState = LOW;
        resetInactivityTimer();
      }
      pedal.debouncing = false;
    }
  } else if (currentState == HIGH && pedal.lastState == LOW) {
    // Released - send immediately
    sendPedalEvent(key, false, pedalName);
    pedal.lastState = HIGH;
    pedal.debouncing = false;
    resetInactivityTimer();
  } else if (currentState == HIGH && pedal.debouncing) {
    // Bounced back to HIGH during debounce - cancel
    pedal.debouncing = false;
  }
}

// Helper function to send ESP-NOW message
void sendPedalEvent(char key, bool pressed, const char* pedalName) {
  struct_message msg;
  msg.key = key;
  msg.pressed = pressed;
  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (result == ESP_OK) {
    Serial.print("Sent ");
    Serial.print(pedalName);
    Serial.println(pressed ? " PRESS" : " RELEASE");
  } else {
    Serial.print("Error sending ");
    Serial.print(pedalName);
    Serial.print(pressed ? " PRESS: " : " RELEASE: ");
    Serial.println(result);
  }
}

void loop() {
  // Handle inactivity timeout
  if (millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    Serial.println("Inactivity timeout. Going to deep sleep.");
    goToDeepSleep();
  }

  // Turn OFF LED (LOW = OFF)
  digitalWrite(LED_PIN, LOW);

  // Handle LEFT pedal (active in DUAL_PEDAL or SINGLE_PEDAL_1 mode)
  if (PEDAL_MODE == DUAL_PEDAL || PEDAL_MODE == SINGLE_PEDAL_1) {
    uint8_t pin = (PEDAL_MODE == DUAL_PEDAL) ? DUAL_LEFT_PIN : SINGLE_PEDAL_PIN;
    handlePedal(pin, leftPedal, LEFT_PEDAL_KEY, "LEFT pedal");
  }
  
  // Handle RIGHT pedal (active in DUAL_PEDAL or SINGLE_PEDAL_2 mode)
  if (PEDAL_MODE == DUAL_PEDAL || PEDAL_MODE == SINGLE_PEDAL_2) {
    uint8_t pin = (PEDAL_MODE == DUAL_PEDAL) ? DUAL_RIGHT_PIN : SINGLE_PEDAL_PIN;
    // In SINGLE_PEDAL_2 mode, use leftPedal state tracking since it's the same pin
    PedalState& pedalState = (PEDAL_MODE == DUAL_PEDAL) ? rightPedal : leftPedal;
    handlePedal(pin, pedalState, RIGHT_PEDAL_KEY, "RIGHT pedal");
  }
}

// Function to reset the inactivity timer
void resetInactivityTimer() {
  lastActivityTime = millis();
}

// Function to put the ESP32 into deep sleep
void goToDeepSleep() {
  // Turn OFF LED
  digitalWrite(LED_PIN, LOW);
  // Wakeup from deep sleep when PEDAL is pushed/LOW.
  // Always use pin 13 for wakeup since it's used in all modes
  // For DUAL_PEDAL mode with ESP32-S2/S3/C6/H2, you could use ext1_wakeup to wake from both pins:
  //esp_sleep_enable_ext1_wakeup(ext1_wakeup_mask, ESP_EXT1_WAKEUP_ANY_LOW);
  uint8_t wakeupPin = (PEDAL_MODE == DUAL_PEDAL) ? DUAL_LEFT_PIN : SINGLE_PEDAL_PIN;
  esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeupPin, LOW);
  Serial.println("Going to deep sleep...");
  esp_deep_sleep_start();
}
