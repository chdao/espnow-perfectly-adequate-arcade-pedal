#ifndef DEBUG_MONITOR_H
#define DEBUG_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

#define DEBUG_MONITOR_PEER_READY_DELAY_MS 50
#define DEBUG_MONITOR_BEACON_INTERVAL_MS 5000  // Monitor broadcasts every 5 seconds
#define DEBUG_MONITOR_BEACON_CHECK_INTERVAL_MS 1000  // Check for beacons every 1 second

// Forward declarations for message structures
#ifndef MSG_DEBUG
#define MSG_DEBUG 0x04
#define MSG_DEBUG_MONITOR_REQ 0x05
#define MSG_DEBUG_MONITOR_BEACON 0x08
#endif

// Function pointer types for transport abstraction
typedef bool (*DebugMonitor_SendFunc)(void* transport, const uint8_t* mac, const uint8_t* data, int len);
typedef bool (*DebugMonitor_AddPeerFunc)(void* transport, const uint8_t* mac, uint8_t channel);

typedef struct {
  uint8_t mac[6];  // Debug monitor MAC
  uint8_t deviceMAC[6];  // This device's MAC
  bool paired;
  void* transport;  // Generic transport pointer (cast to specific type in implementation)
  DebugMonitor_SendFunc sendFunc;  // Function to send data
  DebugMonitor_AddPeerFunc addPeerFunc;  // Function to add peer
  unsigned long bootTime;
  bool espNowInitialized;
  unsigned long lastBeaconTime;  // Last time we saw a beacon from monitor
  bool statusSent;  // Track if status has been sent to prevent duplicates
  const char* devicePrefix;  // "[T]" or "[R]" prefix for messages
} DebugMonitor;

// Callback type for sending status on beacon
typedef void (*DebugMonitor_StatusCallback)(DebugMonitor*);

void debugMonitor_init(DebugMonitor* monitor, void* transport, 
                       DebugMonitor_SendFunc sendFunc, DebugMonitor_AddPeerFunc addPeerFunc,
                       const char* devicePrefix, unsigned long bootTime);
void debugMonitor_handleDiscoveryRequest(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel);
void debugMonitor_handleBeacon(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel);
void debugMonitor_handleBeaconWithCallback(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel, DebugMonitor_StatusCallback callback);
void debugMonitor_print(DebugMonitor* monitor, const char* format, ...);
void debugMonitor_update(DebugMonitor* monitor, unsigned long currentTime);
void debugMonitor_load(DebugMonitor* monitor);
void debugMonitor_save(DebugMonitor* monitor);
void debugMonitor_saveDebugState(bool debugEnabled);
bool debugMonitor_loadDebugState();

#endif // DEBUG_MONITOR_H

