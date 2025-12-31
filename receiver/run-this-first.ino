/*
 * Run This First - Get Receiver MAC Address
 * 
 * Upload this sketch to your ESP32-S2/S3 receiver first to get its MAC address.
 * The MAC address will be displayed in the Serial Monitor.
 * Copy this MAC address and update it in transmitter/transmitter.ino
 */

#include <WiFi.h>

// Helper function to print byte as lowercase hex
void printHexLowercase(uint8_t value) {
  if (value < 0x10) Serial.print("0");
  // Convert to lowercase hex
  char hexChars[] = "0123456789abcdef";
  Serial.print(hexChars[(value >> 4) & 0x0F]);
  Serial.print(hexChars[value & 0x0F]);
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Give Serial time to initialize
  
  Serial.println("========================================");
  Serial.println("ESPNow Pedal Receiver - MAC Address");
  Serial.println("========================================");
  Serial.println();
  
  // Initialize WiFi to get MAC address
  WiFi.mode(WIFI_STA);
  delay(100); // Give WiFi time to initialize before reading MAC address
  
  // Get and display MAC address
  String macAddress = WiFi.macAddress();
  Serial.print("Receiver MAC Address: ");
  Serial.println(macAddress);
  Serial.println();
  
  // Get MAC address as bytes
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  // Display in exact format needed for transmitter (lowercase hex)
  Serial.println("Copy this line to transmitter/transmitter.ino (around line 45):");
  Serial.print("uint8_t broadcastAddress[] = {");
  
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (mac[i] < 0x10) Serial.print("0");
    // Print in lowercase hex to match transmitter format
    char hexChar = (mac[i] & 0xF0) >> 4;
    Serial.print((char)(hexChar < 10 ? '0' + hexChar : 'a' + hexChar - 10));
    hexChar = mac[i] & 0x0F;
    Serial.print((char)(hexChar < 10 ? '0' + hexChar : 'a' + hexChar - 10));
    if (i < 5) Serial.print(", ");
  }
  Serial.println("};");
  Serial.println();
  
  // Also show just the values for easy copy-paste
  Serial.println("Or copy just the values:");
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (mac[i] < 0x10) Serial.print("0");
    char hexChar = (mac[i] & 0xF0) >> 4;
    Serial.print((char)(hexChar < 10 ? '0' + hexChar : 'a' + hexChar - 10));
    hexChar = mac[i] & 0x0F;
    Serial.print((char)(hexChar < 10 ? '0' + hexChar : 'a' + hexChar - 10));
    if (i < 5) Serial.print(", ");
  }
  Serial.println("}");
  Serial.println();
  Serial.println("========================================");
}

void loop() {
  // Nothing to do here
  delay(1000);
}

