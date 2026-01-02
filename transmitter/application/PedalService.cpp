#include "PedalService.h"
#include "../application/PairingService.h"
#include <string.h>
#include <stdarg.h>
#include <Arduino.h>
#include "../shared/messages.h"

// Forward declaration - debugPrint is defined in transmitter.ino
extern void debugPrint(const char* format, ...);
extern bool debugEnabled;

static PedalService* g_pedalService = nullptr;
static PairingService* g_pairingService = nullptr;

void pedalService_setPairingService(PairingService* pairingService) {
  g_pairingService = pairingService;
}

void onPedalPress(char key) {
  if (!g_pedalService) return;
  
  // Always send debug message if enabled (even if debug monitor not paired, it goes to Serial)
  if (debugEnabled) {
    unsigned long timeSinceBoot = millis() - g_pedalService->bootTime;
    const char* pairedStr = pairingState_isPaired(g_pedalService->pairingState) ? "" : " (not paired)";
    debugPrint("[%lu ms] Pedal %c PRESSED%s\n", timeSinceBoot, key, pairedStr);
  }
  
  // If not paired and we have a discovered receiver, initiate pairing
  if (!pairingState_isPaired(g_pedalService->pairingState) && 
      g_pedalService->pairingState->receiverBeaconReceived && g_pairingService) {
    // Determine slots needed based on pedal mode (0=DUAL needs 2, 1=SINGLE needs 1)
    int slotsNeeded = getSlotsNeeded(g_pedalService->reader->pedalMode);
    if (g_pedalService->pairingState->discoveredAvailableSlots >= slotsNeeded) {
      if (debugEnabled) {
        unsigned long timeSinceBoot = millis() - g_pedalService->bootTime;
        debugPrint("[%lu ms] Initiating pairing due to pedal press\n", timeSinceBoot);
      }
      pairingService_initiatePairing(g_pairingService, 
                                     g_pedalService->pairingState->discoveredReceiverMAC, 0);
    }
  }
  
  // Send pedal event if paired
  if (pairingState_isPaired(g_pedalService->pairingState)) {
    pedalService_sendPedalEvent(g_pedalService, key, true);
  }
  
  if (g_pedalService->onActivity) {
    g_pedalService->onActivity();
  }
}

void onPedalRelease(char key) {
  if (!g_pedalService) return;
  
  // Always send debug message if enabled (even if debug monitor not paired, it goes to Serial)
  if (debugEnabled) {
    unsigned long timeSinceBoot = millis() - g_pedalService->bootTime;
    const char* pairedStr = pairingState_isPaired(g_pedalService->pairingState) ? "" : " (not paired)";
    debugPrint("[%lu ms] Pedal %c RELEASED%s\n", timeSinceBoot, key, pairedStr);
  }
  
  // Send pedal event if paired
  if (pairingState_isPaired(g_pedalService->pairingState)) {
    pedalService_sendPedalEvent(g_pedalService, key, false);
  }
  
  if (g_pedalService->onActivity) {
    g_pedalService->onActivity();
  }
}

void pedalService_init(PedalService* service, PedalReader* reader, PairingState* pairingState, 
                       EspNowTransport* transport, unsigned long* lastActivityTime, unsigned long bootTime) {
  service->reader = reader;
  service->pairingState = pairingState;
  service->transport = transport;
  service->lastActivityTime = lastActivityTime;
  service->bootTime = bootTime;
  service->onActivity = nullptr;
  g_pedalService = service;
}

void pedalService_update(PedalService* service) {
  pedalReader_update(service->reader, onPedalPress, onPedalRelease);
}

void pedalService_sendPedalEvent(PedalService* service, char key, bool pressed) {
  if (!pairingState_isPaired(service->pairingState)) {
    return;  // Not paired
  }
  
  // Note: pedalMode field is not used by receiver (it uses transmitterManager data)
  // but we set it for consistency
  // Use designated initializer for better code generation
  struct_message msg = {
    .msgType = MSG_PEDAL_EVENT,
    .key = key,
    .pressed = pressed,
    .pedalMode = service->reader->pedalMode
  };
  
  bool sent = espNowTransport_send(service->transport, service->pairingState->pairedReceiverMAC, 
                                   (uint8_t*)&msg, sizeof(msg));
  
  if (debugEnabled) {
    unsigned long timeSinceBoot = millis() - service->bootTime;
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             service->pairingState->pairedReceiverMAC[0], service->pairingState->pairedReceiverMAC[1],
             service->pairingState->pairedReceiverMAC[2], service->pairingState->pairedReceiverMAC[3],
             service->pairingState->pairedReceiverMAC[4], service->pairingState->pairedReceiverMAC[5]);
    debugPrint("[%lu ms] Sent pedal event: key='%c', %s -> %s (%s)\n",
                timeSinceBoot, key, pressed ? "PRESSED" : "RELEASED", macStr, sent ? "sent" : "FAILED");
  }
  
  if (service->lastActivityTime) {
    *service->lastActivityTime = millis();
  }
}

