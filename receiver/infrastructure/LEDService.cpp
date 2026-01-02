#include "LEDService.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void ledService_init(LEDService* service, unsigned long bootTime) {
  service->bootTime = bootTime;
  service->ledState = false;  // Start with LED off
  pixels.begin();
  pixels.clear();
  pixels.show();
}

void ledService_update(LEDService* service, unsigned long currentTime) {
  unsigned long timeSinceBoot = currentTime - service->bootTime;
  bool inGracePeriod = (timeSinceBoot < TRANSMITTER_TIMEOUT);
  
  // Only update LED when state changes to avoid unnecessary updates
  if (inGracePeriod && !service->ledState) {
    // Grace period - set LED to blue
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    service->ledState = true;
  } else if (!inGracePeriod && service->ledState) {
    // After grace period - turn LED off
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    service->ledState = false;
  }
}

