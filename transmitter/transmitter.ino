#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
// Pedal mode: 0=DUAL (pins 13 & 14), 1=SINGLE_1 (pin 13), 2=SINGLE_2 (pin 13)
#define PEDAL_MODE 1
// ============================================================================

#define PEDAL_1_PIN 13
#define PEDAL_2_PIN 14
#define LED_PIN 2
#define INACTIVITY_TIMEOUT 600000  // 10 minutes
#define DEBOUNCE_DELAY 50

#define DUAL_PEDAL 0
#define SINGLE_PEDAL_1 1
#define SINGLE_PEDAL_2 2

// Message structure (must match receiver)
typedef struct __attribute__((packed)) struct_message {
  uint8_t msgType;
  char key;          // '1' for pin 13, '2' for pin 14
  bool pressed;
  uint8_t pedalMode;
} struct_message;

#define MSG_PEDAL_EVENT    0x00
#define MSG_DISCOVERY_REQ  0x01
#define MSG_DISCOVERY_RESP 0x02
#define MSG_ALIVE          0x03

uint8_t pairedReceiverMAC[6] = {0};
bool isPaired = false;
bool discoveryMode = false;
unsigned long discoveryStartTime = 0;
unsigned long lastDiscoverySend = 0;
unsigned long lastActivityTime = 0;
unsigned long lastAliveReceived = 0;
int reconnectAttempts = 0;

// Configurable intervals (adjust for battery life vs responsiveness)
#define DISCOVERY_TIMEOUT 10000
#define DISCOVERY_SEND_INTERVAL 500  // Initial discovery interval (fast for first boot)
#define DISCOVERY_RECONNECT_INTERVAL 5000  // Base reconnection interval (5 seconds)
#define DISCOVERY_RECONNECT_MAX_INTERVAL 30000  // Max reconnection interval (30 seconds)
#define ALIVE_TIMEOUT 35000  // 35 seconds - restart discovery if no message from receiver (should be > ALIVE_INTERVAL on receiver)
uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct PedalState {
  bool lastState;
  unsigned long debounceTime;
  bool debouncing;
};

PedalState pedal1State = {HIGH, 0, false};
PedalState pedal2State = {HIGH, 0, false};

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < sizeof(struct_message)) return;
  
  struct_message msg;
  memcpy(&msg, data, len);
  
  if (discoveryMode) {
    // Only handle discovery responses during discovery mode
    if (msg.msgType == MSG_DISCOVERY_RESP) {
      memcpy(pairedReceiverMAC, info->src_addr, 6);
      isPaired = true;
      discoveryMode = false;
      
      esp_now_unregister_recv_cb();
      
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, pairedReceiverMAC, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
      
      // Re-register callback to receive alive messages
      esp_now_register_recv_cb(OnDataRecv);
      
      Serial.print("Paired: ");
      for (int i = 0; i < 6; i++) {
        Serial.print(pairedReceiverMAC[i], HEX);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
      
      lastAliveReceived = millis();
      reconnectAttempts = 0;  // Reset reconnect attempts on successful pairing
      digitalWrite(LED_PIN, HIGH);
    }
  } else if (isPaired) {
    // Handle any message from receiver as implicit heartbeat (pedal events, alive messages, etc.)
    if (memcmp(info->src_addr, pairedReceiverMAC, 6) == 0) {
      lastAliveReceived = millis();
      reconnectAttempts = 0;  // Reset reconnect attempts on successful communication
    }
  }
}

void sendDiscoveryRequest() {
  struct_message discovery = {MSG_DISCOVERY_REQ, 0, false, PEDAL_MODE};
  esp_now_send(broadcastMAC, (uint8_t*)&discovery, sizeof(discovery));
}

void startDiscovery() {
  discoveryMode = true;
  discoveryStartTime = millis();
  lastDiscoverySend = 0;
  
  // Remove existing receiver peer if any
  esp_now_del_peer(pairedReceiverMAC);
  
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_peer_info_t broadcastPeer = {};
  memcpy(broadcastPeer.peer_addr, broadcastMAC, 6);
  broadcastPeer.channel = 0;
  broadcastPeer.encrypt = false;
  esp_now_add_peer(&broadcastPeer);
  
  Serial.println("Starting discovery...");
}

void handlePedal(uint8_t pin, PedalState& pedal, char key) {
  bool currentState = digitalRead(pin);
  unsigned long currentTime = millis();
  
  if (currentState == LOW && pedal.lastState == HIGH) {
    if (!pedal.debouncing) {
      pedal.debounceTime = currentTime;
      pedal.debouncing = true;
    } else if (currentTime - pedal.debounceTime >= DEBOUNCE_DELAY) {
      if (digitalRead(pin) == LOW) {
        sendPedalEvent(key, true);
        pedal.lastState = LOW;
        resetInactivityTimer();
      }
      pedal.debouncing = false;
    }
  } else if (currentState == HIGH && pedal.lastState == LOW) {
    sendPedalEvent(key, false);
    pedal.lastState = HIGH;
    pedal.debouncing = false;
    resetInactivityTimer();
  } else if (currentState == HIGH && pedal.debouncing) {
    pedal.debouncing = false;
  }
}

void sendPedalEvent(char key, bool pressed) {
  if (!isPaired) return;
  
  struct_message msg = {MSG_PEDAL_EVENT, key, pressed, 0};
  esp_now_send(pairedReceiverMAC, (uint8_t*)&msg, sizeof(msg));
}

void resetInactivityTimer() {
  lastActivityTime = millis();
}

void goToDeepSleep() {
  digitalWrite(LED_PIN, LOW);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PEDAL_1_PIN, LOW);
  Serial.println("Going to deep sleep...");
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP-NOW Pedal Transmitter");
  Serial.print("Mode: ");
  Serial.println(PEDAL_MODE == DUAL_PEDAL ? "DUAL" : "SINGLE");

  pinMode(PEDAL_1_PIN, INPUT_PULLUP);
  if (PEDAL_MODE == DUAL_PEDAL) {
    pinMode(PEDAL_2_PIN, INPUT_PULLUP);
  }
  pinMode(LED_PIN, OUTPUT);
 
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  Serial.println("ESP-NOW initialized");
  startDiscovery();
  lastActivityTime = millis();
}

void loop() {
  // Check if we've lost connection (no message from receiver for ALIVE_TIMEOUT)
  if (isPaired && !discoveryMode) {
    if (millis() - lastAliveReceived > ALIVE_TIMEOUT) {
      Serial.println("Lost connection - restarting discovery");
      isPaired = false;
      reconnectAttempts++;
      startDiscovery();
    }
  }
  
  if (discoveryMode) {
    // Determine discovery interval based on whether we're reconnecting
    uint8_t zeroMAC[6] = {0, 0, 0, 0, 0, 0};
    bool isReconnecting = (memcmp(pairedReceiverMAC, zeroMAC, 6) != 0);
    
    unsigned long interval;
    if (isReconnecting) {
      // Exponential backoff: 5s, 10s, 20s, 30s (max)
      unsigned long backoffInterval = DISCOVERY_RECONNECT_INTERVAL * (1 << (reconnectAttempts - 1));
      interval = (backoffInterval > DISCOVERY_RECONNECT_MAX_INTERVAL) 
                 ? DISCOVERY_RECONNECT_MAX_INTERVAL 
                 : backoffInterval;
    } else {
      // First boot: use fast interval
      interval = DISCOVERY_SEND_INTERVAL;
    }
    
    // Check for timeout - if reconnecting, keep trying; if first boot, give up after timeout
    if (millis() - discoveryStartTime > DISCOVERY_TIMEOUT) {
      if (isReconnecting) {
        // Keep trying with exponential backoff - reset timer for next attempt
        Serial.println("Discovery timeout - retrying with backoff");
        discoveryStartTime = millis();
        lastDiscoverySend = 0;  // Allow immediate retry with new interval
      } else {
        // First boot timeout - give up to save battery
        Serial.println("Discovery timeout - no receiver found");
        discoveryMode = false;
        digitalWrite(LED_PIN, LOW);
      }
    }
    
    if (discoveryMode && millis() - lastDiscoverySend > interval) {
      sendDiscoveryRequest();
      lastDiscoverySend = millis();
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    delay(10);
    return;
  }
  
  if (isPaired && millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    goToDeepSleep();
  }

  if (!isPaired) {
    digitalWrite(LED_PIN, LOW);
  }

  handlePedal(PEDAL_1_PIN, pedal1State, '1');
  
  if (PEDAL_MODE == DUAL_PEDAL) {
    handlePedal(PEDAL_2_PIN, pedal2State, '2');
  }
}
