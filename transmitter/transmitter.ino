#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// Clean Architecture: Include shared and domain modules
#include "shared/messages.h"
#include "domain/PairingState.h"
#include "domain/PedalReader.h"
#include "infrastructure/EspNowTransport.h"
#include "infrastructure/DebugMonitor.h"
#include "application/PairingService.h"
#include "application/PedalService.h"

// ============================================================================
// CONFIGURATION
// ============================================================================
#define PEDAL_MODE 1  // 0=DUAL (pins 13 & 14), 1=SINGLE (pin 13 only)
// NOTE: Debug mode is now controlled by the button on pin 27 (FireBeetle).
//       Press the button to enable debug output (both Serial and debug monitor).
//       Debug state is runtime-controlled, not compile-time.
// ============================================================================

#define PEDAL_1_PIN 13
#define PEDAL_2_PIN 14
#define DEBUG_BUTTON_PIN 27  // FireBeetle button pin
#define INACTIVITY_TIMEOUT 600000  // 10 minutes
#define IDLE_DELAY_PAIRED 20  // 20ms delay when paired
#define IDLE_DELAY_UNPAIRED 200  // 200ms delay when not paired
#define SERIAL_INIT_DELAY_MS 100
#define HEARTBEAT_INTERVAL_MS 5000  // 5 seconds
#define DEBUG_BUTTON_DEBOUNCE_MS 50  // Button debounce time

// Domain layer instances
PairingState pairingState;
PedalReader pedalReader;
EspNowTransport transport;
DebugMonitor debugMonitor;

// Application layer instances
PairingService pairingService;
PedalService pedalService;

// System state
unsigned long lastActivityTime = 0;
unsigned long bootTime = 0;
bool debugEnabled = false;  // Runtime debug state (controlled by button toggle)
bool lastButtonState = HIGH;
unsigned long lastButtonChangeTime = 0;
bool buttonPressed = false;  // Track if button press has been processed

// Forward declarations
void onMessageReceived(const uint8_t* senderMAC, const uint8_t* data, int len, uint8_t channel);
void onPaired(const uint8_t* receiverMAC);
void onActivity();
void debugPrint(const char* format, ...);

void debugPrint(const char* format, ...) {
  if (!debugEnabled) return;
  
  char buffer[200];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  // Send to debug monitor only (debugMonitor_print adds [TRANSMITTER] prefix and timestamp)
  if (debugMonitor.paired && debugMonitor.espNowInitialized) {
    debugMonitor_print(&debugMonitor, "%s", buffer);
  }
}

void onPaired(const uint8_t* receiverMAC) {
  if (!debugEnabled) return;
  
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           receiverMAC[0], receiverMAC[1], receiverMAC[2],
           receiverMAC[3], receiverMAC[4], receiverMAC[5]);
  
  debugPrint("[%lu ms] Successfully paired with receiver: %s\n", millis() - bootTime, macStr);
}

void onActivity() {
  lastActivityTime = millis();
}

void sendDeleteRecordMessage(const uint8_t* receiverMAC) {
  struct_message deleteMsg = {MSG_DELETE_RECORD, 0, false, 0};
  espNowTransport_send(&transport, receiverMAC, (uint8_t*)&deleteMsg, sizeof(deleteMsg));
  
  if (!debugEnabled) return;
  
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           receiverMAC[0], receiverMAC[1], receiverMAC[2],
           receiverMAC[3], receiverMAC[4], receiverMAC[5]);
  
  debugPrint("[%lu ms] Sent delete record message to receiver: %s\n", millis() - bootTime, macStr);
}

void onMessageReceived(const uint8_t* senderMAC, const uint8_t* data, int len, uint8_t channel) {
  if (len < 1) return;
  
  uint8_t msgType = data[0];
  
  // Handle debug monitor discovery request (legacy)
  if (msgType == MSG_DEBUG_MONITOR_REQ) {
    debugMonitor_handleDiscoveryRequest(&debugMonitor, senderMAC, channel);
    if (debugEnabled) {
      debugMonitor_print(&debugMonitor, "Debug monitor discovery request received and processed");
    }
    return;
  }
  
  // Handle debug monitor beacon (new proactive discovery)
  if (msgType == MSG_DEBUG_MONITOR_BEACON && len >= sizeof(debug_monitor_beacon_message)) {
    debug_monitor_beacon_message* beacon = (debug_monitor_beacon_message*)data;
    debugMonitor_handleBeacon(&debugMonitor, beacon->monitorMAC, channel);
    if (debugEnabled && !debugMonitor.paired) {
      // Just paired via beacon
      debugMonitor_print(&debugMonitor, "Debug monitor discovered via beacon");
    }
    return;
  }
  
  if (debugEnabled) {
    unsigned long timeSinceBoot = millis() - bootTime;
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             senderMAC[0], senderMAC[1], senderMAC[2],
             senderMAC[3], senderMAC[4], senderMAC[5]);
    debugPrint("[%lu ms] Received ESP-NOW message: len=%d, sender=%s\n", timeSinceBoot, len, macStr);
  }
  
  // Handle beacon message
  if (msgType == MSG_BEACON && len >= sizeof(beacon_message)) {
    beacon_message* beacon = (beacon_message*)data;
    pairingService_handleBeacon(&pairingService, senderMAC, beacon);
    
    if (debugEnabled) {
      unsigned long timeSinceBoot = millis() - bootTime;
      debugPrint("[%lu ms] Received MSG_BEACON: slots=%d/%d\n", 
                 timeSinceBoot, beacon->availableSlots, beacon->totalSlots);
    }
    return;
  }
  
  // Handle other messages
  if (len < sizeof(struct_message)) {
    if (debugEnabled) {
      unsigned long timeSinceBoot = millis() - bootTime;
      debugPrint("[%lu ms] Message too short\n", timeSinceBoot);
    }
    return;
  }
  
  struct_message* msg = (struct_message*)data;
  
  if (debugEnabled) {
    unsigned long timeSinceBoot = millis() - bootTime;
    debugPrint("[%lu ms] Message type=%d, isPaired=%d\n", 
               timeSinceBoot, msg->msgType, pairingState_isPaired(&pairingState));
  }
  
  if (pairingState_isPaired(&pairingState)) {
    // Already paired - check if message is from our paired receiver
    if (macEqual(senderMAC, pairingState.pairedReceiverMAC)) {
      // Message from our paired receiver - accept it
      if (debugEnabled) {
        unsigned long timeSinceBoot = millis() - bootTime;
        debugPrint("[%lu ms] Received message from paired receiver (type=%d)\n", 
                   timeSinceBoot, msg->msgType);
      }
    } else {
      // Message from different receiver - send DELETE_RECORD
      if (msg->msgType == MSG_ALIVE || msg->msgType == MSG_DISCOVERY_RESP) {
        if (debugEnabled) {
          unsigned long timeSinceBoot = millis() - bootTime;
          debugPrint("[%lu ms] Received message from different receiver - sending DELETE_RECORD\n", timeSinceBoot);
        }
        
        espNowTransport_addPeer(&transport, senderMAC, channel);
        sendDeleteRecordMessage(senderMAC);
      }
    }
  } else {
    // Not paired - handle pairing messages
    if (msg->msgType == MSG_DISCOVERY_RESP) {
      pairingService_handleDiscoveryResponse(&pairingService, senderMAC, channel);
    } else if (msg->msgType == MSG_ALIVE) {
      pairingService_handleAlive(&pairingService, senderMAC, channel);
    }
  }
}


void goToDeepSleep() {
  if (debugEnabled) {
    debugPrint("Going to deep sleep...\n");
  }
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PEDAL_1_PIN, LOW);
  esp_deep_sleep_start();
}

void setup() {
  // Initialize Serial (minimal use - only for startup message)
  Serial.begin(115200);
  delay(SERIAL_INIT_DELAY_MS);
  
  // Initialize debug button (pin 27 on FireBeetle) - toggle switch
  pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(DEBUG_BUTTON_PIN);
  lastButtonChangeTime = millis();
  buttonPressed = false;

  // Battery optimization
  setCpuFrequencyMhz(80);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  bootTime = millis();
  lastActivityTime = millis();
  
  // Initialize domain layer
  pairingState_init(&pairingState);
  pedalReader_init(&pedalReader, PEDAL_1_PIN, PEDAL_2_PIN, PEDAL_MODE);
  
  // Initialize infrastructure layer
  espNowTransport_init(&transport);
  
  // Initialize debug monitor (needed before loading debug state)
  debugMonitor_init(&debugMonitor, &transport, bootTime);
  debugMonitor_load(&debugMonitor);
  debugMonitor.espNowInitialized = transport.initialized;
  
  // Get device MAC address now that WiFi is initialized
  // WiFi.mode(WIFI_STA) was called in espNowTransport_init, so MAC should be available
  if (transport.initialized) {
    WiFi.macAddress(debugMonitor.deviceMAC);
    // Verify MAC was read correctly (not all zeros)
    if (!isValidMAC(debugMonitor.deviceMAC)) {
      // Retry after a small delay
      delay(50);
      WiFi.macAddress(debugMonitor.deviceMAC);
    }
  }
  
  // Load saved debug state (restored after deep sleep)
  debugEnabled = debugMonitor_loadDebugState();
  
  // Add broadcast peer (only if initialized)
  if (transport.initialized) {
    uint8_t broadcastMAC[] = BROADCAST_MAC;
    espNowTransport_addPeer(&transport, broadcastMAC, 0);
    
    // Add saved debug monitor as peer (if it was saved)
    if (debugMonitor.paired) {
      espNowTransport_addPeer(&transport, debugMonitor.mac, 0);
      delay(DEBUG_MONITOR_PEER_READY_DELAY_MS);
    }
    
    espNowTransport_registerReceiveCallback(&transport, onMessageReceived);
  }
  
  // Initialize application layer
  pairingService_init(&pairingService, &pairingState, &transport, PEDAL_MODE, bootTime);
  pairingService.onPaired = onPaired;
  
  pedalService_init(&pedalService, &pedalReader, &pairingState, &transport, &lastActivityTime, bootTime);
  pedalService.onActivity = onActivity;
  pedalService_setPairingService(&pairingService);
  
  // Broadcast that we're online
  pairingService_broadcastOnline(&pairingService);
  
  // Only print startup message if debug monitor is paired
  if (debugMonitor.paired && debugMonitor.espNowInitialized) {
    Serial.println("Debug logs are being sent to the debug monitor");
    debugMonitor_print(&debugMonitor, "ESP-NOW initialized");
    debugMonitor_print(&debugMonitor, "=== Transmitter Ready ===");
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check debug button state (toggle on press, with debouncing)
  bool currentButtonState = digitalRead(DEBUG_BUTTON_PIN);
  
  // Detect button press (transition from HIGH to LOW)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    // Button just pressed - start debounce timer
    lastButtonChangeTime = currentTime;
    buttonPressed = false;  // Reset press flag
  }
  
  // Check if button has been held down long enough (debounce)
  if (currentButtonState == LOW && !buttonPressed && 
      (currentTime - lastButtonChangeTime) >= DEBUG_BUTTON_DEBOUNCE_MS) {
    // Button press confirmed - toggle debug state
    buttonPressed = true;
    debugEnabled = !debugEnabled;
    
    // Save debug state to persist across deep sleep
    debugMonitor_saveDebugState(debugEnabled);
    
    if (debugEnabled && debugMonitor.paired && debugMonitor.espNowInitialized) {
      debugMonitor_print(&debugMonitor, "Debug mode enabled via button toggle");
    }
  }
  
  // Update last button state
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    // Button released - reset for next press
    buttonPressed = false;
  }
  lastButtonState = currentButtonState;
  
  // Check inactivity timeout
  if (currentTime - lastActivityTime > INACTIVITY_TIMEOUT) {
    goToDeepSleep();
  }
  
  // Update pedal service (handles pedal reading and events)
  pedalService_update(&pedalService);
  
  // Update debug monitor (checks for beacons, manages connection)
  debugMonitor_update(&debugMonitor, currentTime);
  
  // Periodic heartbeat to confirm loop is running (only when not paired and debug enabled)
  if (debugEnabled) {
    static unsigned long lastHeartbeat = 0;
    if (!pairingState_isPaired(&pairingState) && (currentTime - lastHeartbeat > HEARTBEAT_INTERVAL_MS)) {
      debugPrint("[%lu ms] Waiting for receiver...\n", currentTime - bootTime);
      lastHeartbeat = currentTime;
    }
  }
  
  // Battery optimization: Variable delay based on pairing status
  if (pairingState_isPaired(&pairingState)) {
    delay(IDLE_DELAY_PAIRED);
  } else {
    delay(IDLE_DELAY_UNPAIRED);
  }
}

// Include implementation files (Arduino IDE doesn't auto-compile .cpp files in subdirectories)
#include "domain/PairingState.cpp"
#include "domain/PedalReader.cpp"
#include "infrastructure/EspNowTransport.cpp"
#include "infrastructure/DebugMonitor.cpp"
#include "application/PairingService.cpp"
#include "application/PedalService.cpp"
