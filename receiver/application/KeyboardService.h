#ifndef KEYBOARD_SERVICE_H
#define KEYBOARD_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "../domain/TransmitterManager.h"
#include "../shared/messages.h"

#define USB_INIT_DELAY_MS 500
#define KEYBOARD_INIT_DELAY_MS 2000

typedef struct {
  TransmitterManager* manager;
  bool keysPressed[256];
  bool firstPressAfterIdle;  // Track if this is the first press after idle period
} KeyboardService;

void keyboardService_init(KeyboardService* service, TransmitterManager* manager);
void keyboardService_handlePedalEvent(KeyboardService* service, const uint8_t* txMAC, 
                                       const struct_message* msg);

#endif // KEYBOARD_SERVICE_H

