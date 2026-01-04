#include "DebugMonitor.h"
#include "messages.h"  // For MAC helper functions
#include <stdarg.h>
#include <string.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

// Helper function to remove trailing newline
static void removeTrailingNewline(char* str) {
  int len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[len - 1] = '\0';
    len--;
  }
}

// Note: isValidMAC, macCopy, and macEqual are now in messages.h

void debugMonitor_init(DebugMonitor* monitor, void* transport, 
                       DebugMonitor_SendFunc sendFunc, DebugMonitor_AddPeerFunc addPeerFunc,
                       const char* devicePrefix, unsigned long bootTime) {
  monitor->transport = transport;
  monitor->sendFunc = sendFunc;
  monitor->addPeerFunc = addPeerFunc;
  monitor->devicePrefix = devicePrefix;
  monitor->bootTime = bootTime;
  monitor->cumulativeTime = 0;  // Default to 0 (receiver doesn't use this)
  monitor->paired = false;
  monitor->espNowInitialized = false;
  monitor->lastBeaconTime = 0;
  monitor->statusSent = false;
  // Fast zero using 32-bit and 16-bit writes
  uint32_t* mac32 = (uint32_t*)monitor->mac;
  uint16_t* mac16 = (uint16_t*)(monitor->mac + 4);
  *mac32 = 0;
  *mac16 = 0;
  // Initialize device MAC to zero - will be set after WiFi is initialized
  uint32_t* devMac32 = (uint32_t*)monitor->deviceMAC;
  uint16_t* devMac16 = (uint16_t*)(monitor->deviceMAC + 4);
  *devMac32 = 0;
  *devMac16 = 0;
}

void debugMonitor_handleDiscoveryRequest(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel) {
  // Legacy support: if monitor sends discovery request, pair immediately
  debugMonitor_handleBeacon(monitor, monitorMAC, channel);
}

void debugMonitor_handleBeacon(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel) {
  debugMonitor_handleBeaconWithCallback(monitor, monitorMAC, channel, NULL);
}

void debugMonitor_handleBeaconWithCallback(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel, DebugMonitor_StatusCallback callback) {
  if (!isValidMAC(monitorMAC)) return;
  
  bool isSavedMonitor = macEqual(monitorMAC, monitor->mac);
  bool isNewPairing = (!monitor->paired || !isSavedMonitor);
  
  if (isNewPairing) {
    macCopy(monitor->mac, monitorMAC);
    monitor->paired = true;
    monitor->lastBeaconTime = millis();
    monitor->espNowInitialized = true;  // Assume initialized if we're handling beacons
    monitor->statusSent = false;  // Reset status sent flag for new pairing
    
    monitor->addPeerFunc(monitor->transport, monitorMAC, channel);
    
    // Small delay to ensure peer is ready
    delay(DEBUG_MONITOR_PEER_READY_DELAY_MS);
    
    debugMonitor_save(monitor);
    
    // Send initialization message now that monitor is connected (only if not already sent)
    if (!monitor->statusSent) {
      if (callback) {
        // Use callback for custom status (receiver)
        callback(monitor);
      }
      debugMonitor_print(monitor, "%s Ready", 
                        strcmp(monitor->devicePrefix, "[T]") == 0 ? "Transmitter" : "Receiver");
      
      // For transmitter, show debug state
      if (strcmp(monitor->devicePrefix, "[T]") == 0) {
        bool debugEnabled = debugMonitor_loadDebugState();
        debugMonitor_print(monitor, "Debug mode: %s", debugEnabled ? "ENABLED" : "DISABLED");
      }
      
      monitor->statusSent = true;
    }
  } else {
    // Update channel and reset beacon time - ensure peer is updated with correct channel
    monitor->lastBeaconTime = millis();
    monitor->addPeerFunc(monitor->transport, monitorMAC, channel);
    
    // Send status on first beacon after boot if not already sent (reconnection scenario)
    if (!monitor->statusSent) {
      delay(DEBUG_MONITOR_PEER_READY_DELAY_MS);  // Small delay to ensure peer is ready
      if (callback) {
        // Use callback for custom status (receiver)
        callback(monitor);
      }
      debugMonitor_print(monitor, "%s Ready", 
                        strcmp(monitor->devicePrefix, "[T]") == 0 ? "Transmitter" : "Receiver");
      
      // For transmitter, show debug state
      if (strcmp(monitor->devicePrefix, "[T]") == 0) {
        bool debugEnabled = debugMonitor_loadDebugState();
        debugMonitor_print(monitor, "Debug mode: %s", debugEnabled ? "ENABLED" : "DISABLED");
      }
      
      monitor->statusSent = true;
    }
  }
}

void debugMonitor_print(DebugMonitor* monitor, const char* format, ...) {
  if (!monitor->espNowInitialized) {
    return;  // ESP-NOW not initialized yet
  }
  if (!monitor->paired) {
    return;  // Debug monitor not paired
  }
  
  // Check if MAC is valid (not all zeros)
  if (!isValidMAC(monitor->mac)) {
    return;  // No valid MAC address
  }
  
  char buffer[200];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  // Remove trailing newline from buffer (debug monitor handles line breaks)
  removeTrailingNewline(buffer);
  
  // Calculate time from boot in seconds
  // For transmitter: add cumulativeTime to account for time before deep sleep
  // For receiver: cumulativeTime is 0, so this is just (currentTime - bootTime)
  unsigned long currentTime = millis();
  float timeFromBoot = (monitor->cumulativeTime + (currentTime - monitor->bootTime)) / 1000.0f;
  
  // Format message with device prefix and timestamp (MAC will be added by debug monitor from packet sender)
  typedef struct __attribute__((packed)) {
    uint8_t msgType;
    char message[200];
  } debug_message;
  
  debug_message msg = {MSG_DEBUG, {0}};
  int written = snprintf(msg.message, sizeof(msg.message), "%s [%.3fs] %s", 
                        monitor->devicePrefix, timeFromBoot, buffer);
  if (written >= sizeof(msg.message)) {
    msg.message[sizeof(msg.message) - 1] = '\0';  // Ensure null termination
  }
  
  // Calculate actual message length (msgType + string length + null terminator)
  size_t messageLen = 1 + strlen(msg.message) + 1;  // msgType + message + null
  
  // Always try to send - no timeout/disconnect logic
  // If send fails, we'll keep trying (monitor will reconnect via beacon)
  monitor->sendFunc(monitor->transport, monitor->mac, (uint8_t*)&msg, messageLen);
  
  // Small delay to prevent ESP-NOW from dropping messages sent in quick succession
  // This helps prevent message fragmentation/corruption when multiple messages are sent rapidly
  delay(5);
}

void debugMonitor_update(DebugMonitor* monitor, unsigned long currentTime) {
  // Check if we haven't seen a beacon in a while (monitor might be offline)
  // But don't unpair - just mark as potentially disconnected
  // Monitor will reconnect via beacon when it comes back online
  if (monitor->paired && monitor->lastBeaconTime > 0) {
    unsigned long timeSinceLastBeacon = currentTime - monitor->lastBeaconTime;
    // If no beacon for 30 seconds, monitor might be offline
    // But keep paired state so we can reconnect quickly when beacon returns
    if (timeSinceLastBeacon > 30000) {
      // Monitor might be offline, but keep paired state for quick reconnect
      // This allows messages to queue and be sent when monitor comes back
    }
  }
}

void debugMonitor_load(DebugMonitor* monitor) {
  Preferences preferences;
  preferences.begin("debugmon", true);  // Read-only
  bool dbgMonSaved = preferences.getBool("paired", false);
  if (dbgMonSaved) {
    bool allZero = true;
    for (int j = 0; j < 6; j++) {
      char key[10];
      snprintf(key, sizeof(key), "mac_%d", j);
      monitor->mac[j] = preferences.getUChar(key, 0);
      if (monitor->mac[j] != 0) allZero = false;
    }
    monitor->paired = !allZero;
  } else {
    monitor->paired = false;
  }
  preferences.end();
}

void debugMonitor_save(DebugMonitor* monitor) {
  if (monitor->paired) {
    Preferences preferences;
    preferences.begin("debugmon", false);  // Read-write
    for (int j = 0; j < 6; j++) {
      char key[10];
      snprintf(key, sizeof(key), "mac_%d", j);
      preferences.putUChar(key, monitor->mac[j]);
    }
    preferences.putBool("paired", true);
    preferences.end();
  }
}

// Save debug enabled state
void debugMonitor_saveDebugState(bool debugEnabled) {
  Preferences preferences;
  preferences.begin("debugmon", false);  // Read-write
  preferences.putBool("debugEnabled", debugEnabled);
  preferences.end();
}

// Load debug enabled state
bool debugMonitor_loadDebugState() {
  Preferences preferences;
  preferences.begin("debugmon", true);  // Read-only
  bool debugEnabled = preferences.getBool("debugEnabled", false);  // Default to false
  preferences.end();
  return debugEnabled;
}

