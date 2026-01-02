#include "PairingState.h"
#include <string.h>
#include <Arduino.h>
#include "../shared/messages.h"

void pairingState_init(PairingState* state) {
  // Fast zero initialization using 32-bit and 16-bit writes
  uint32_t* mac32_1 = (uint32_t*)state->pairedReceiverMAC;
  uint16_t* mac16_1 = (uint16_t*)(state->pairedReceiverMAC + 4);
  uint32_t* mac32_2 = (uint32_t*)state->discoveredReceiverMAC;
  uint16_t* mac16_2 = (uint16_t*)(state->discoveredReceiverMAC + 4);
  *mac32_1 = 0;
  *mac16_1 = 0;
  *mac32_2 = 0;
  *mac16_2 = 0;
  state->discoveredAvailableSlots = 0;
  state->isPaired = false;
  state->waitingForDiscoveryResponse = false;
  state->receiverBeaconReceived = false;
  state->discoveryRequestTime = 0;
}

void pairingState_setPaired(PairingState* state, const uint8_t* receiverMAC) {
  macCopy(state->pairedReceiverMAC, receiverMAC);
  state->isPaired = true;
  state->waitingForDiscoveryResponse = false;
  state->receiverBeaconReceived = false;
  state->discoveryRequestTime = 0;
}

void pairingState_setDiscoveredReceiver(PairingState* state, const uint8_t* receiverMAC, uint8_t availableSlots) {
  macCopy(state->discoveredReceiverMAC, receiverMAC);
  state->discoveredAvailableSlots = availableSlots;
  state->receiverBeaconReceived = true;
}

void pairingState_clearDiscoveredReceiver(PairingState* state) {
  // Fast zero using 32-bit and 16-bit writes
  uint32_t* mac32 = (uint32_t*)state->discoveredReceiverMAC;
  uint16_t* mac16 = (uint16_t*)(state->discoveredReceiverMAC + 4);
  *mac32 = 0;
  *mac16 = 0;
  state->discoveredAvailableSlots = 0;
  state->receiverBeaconReceived = false;
}

