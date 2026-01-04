#include "KeyboardService.h"
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <string.h>
#include <Arduino.h>

USBHIDKeyboard Keyboard;

void keyboardService_init(KeyboardService* service, TransmitterManager* manager) {
  service->manager = manager;
  memset(service->keysPressed, 0, sizeof(service->keysPressed));
  service->firstPressAfterIdle = false;
  
  USB.begin();
  delay(USB_INIT_DELAY_MS);
  Keyboard.begin();
  delay(KEYBOARD_INIT_DELAY_MS);
}

void keyboardService_handlePedalEvent(KeyboardService* service, const uint8_t* txMAC, 
                                       const struct_message* msg) {
  int transmitterIndex = transmitterManager_findIndex(service->manager, txMAC);
  if (transmitterIndex < 0) {
    return;  // Unknown transmitter
  }
  
  // Update last seen and mark as seen on boot (treat as if transmitter never went offline)
  service->manager->transmitters[transmitterIndex].lastSeen = millis();
  service->manager->transmitters[transmitterIndex].seenOnBoot = true;
  
  // Validate key based on pedal mode
  uint8_t pedalMode = service->manager->transmitters[transmitterIndex].pedalMode;
  if (pedalMode == 0) {
    // DUAL pedal: key must be '1' or '2'
    if (msg->key != '1' && msg->key != '2') {
      return;  // Invalid key - already logged in receiver.ino
    }
  } else {
    // SINGLE pedal: key must be '1'
    if (msg->key != '1') {
      return;  // Invalid key - already logged in receiver.ino
    }
  }
  
  // Determine key to press
  char keyToPress;
  if (pedalMode == 0) {
    // DUAL pedal: '1' -> 'l', '2' -> 'r'
    keyToPress = (msg->key == '1') ? 'l' : 'r';
  } else {
    // SINGLE pedal: '1' -> assigned key based on pairing order
    keyToPress = transmitterManager_getAssignedKey(service->manager, transmitterIndex);
  }
  
  uint8_t keyIndex = (uint8_t)keyToPress;
  
  if (msg->pressed) {
    if (!service->keysPressed[keyIndex]) {
      // Ensure key is released first to clear any stale state
      Keyboard.release(keyToPress);
      delay(5);
      // Now press the key
      Keyboard.press(keyToPress);
      delay(10);  // Small delay to ensure USB HID report is sent
      service->keysPressed[keyIndex] = true;
    }
  } else {
    if (service->keysPressed[keyIndex]) {
      Keyboard.release(keyToPress);
      delay(5);  // Small delay to ensure release is sent
      service->keysPressed[keyIndex] = false;
    }
  }
}

