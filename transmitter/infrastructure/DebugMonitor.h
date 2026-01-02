#ifndef DEBUG_MONITOR_H
#define DEBUG_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "../infrastructure/EspNowTransport.h"
#include "../shared/messages.h"

#define DEBUG_MONITOR_PEER_READY_DELAY_MS 50
#define DEBUG_MONITOR_BEACON_INTERVAL_MS 5000  // Monitor broadcasts every 5 seconds
#define DEBUG_MONITOR_BEACON_CHECK_INTERVAL_MS 1000  // Check for beacons every 1 second

typedef struct {
  uint8_t mac[6];  // Debug monitor MAC
  uint8_t deviceMAC[6];  // This device's (transmitter) MAC
  bool paired;
  EspNowTransport* transport;
  unsigned long bootTime;
  bool espNowInitialized;
  unsigned long lastBeaconTime;  // Last time we saw a beacon from monitor
} DebugMonitor;

void debugMonitor_init(DebugMonitor* monitor, EspNowTransport* transport, unsigned long bootTime);
void debugMonitor_handleDiscoveryRequest(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel);
void debugMonitor_handleBeacon(DebugMonitor* monitor, const uint8_t* monitorMAC, uint8_t channel);
void debugMonitor_print(DebugMonitor* monitor, const char* format, ...);
void debugMonitor_update(DebugMonitor* monitor, unsigned long currentTime);
void debugMonitor_load(DebugMonitor* monitor);
void debugMonitor_save(DebugMonitor* monitor);
void debugMonitor_saveDebugState(bool debugEnabled);
bool debugMonitor_loadDebugState();

#endif // DEBUG_MONITOR_H

