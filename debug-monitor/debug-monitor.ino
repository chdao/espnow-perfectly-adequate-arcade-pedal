/*
 * ESP-NOW Debug Monitor for Pedal Receiver
 * 
 * This sketch runs on a secondary ESP32-S3-DevKit to receive and display
 * debug messages from the pedal receiver via ESP-NOW.
 * 
 * The receiver sends debug messages that are displayed on Serial (USB).
 */

#include <WiFi.h>
#include <esp_now.h>
#include <string.h>

// Debug message structure
typedef struct __attribute__((packed)) debug_message {
  uint8_t msgType;   // 0x04 = debug message
  char message[200]; // Debug message text (null-terminated)
} debug_message;

// Debug monitor beacon message structure
typedef struct __attribute__((packed)) debug_monitor_beacon_message {
  uint8_t msgType;        // 0x08 = MSG_DEBUG_MONITOR_BEACON
  uint8_t monitorMAC[6];
} debug_monitor_beacon_message;

#define MSG_DEBUG 0x04
#define MSG_DEBUG_MONITOR_BEACON 0x08

uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define BEACON_SEND_INTERVAL 5000  // Broadcast beacon every 5 seconds
unsigned long lastBeaconSend = 0;

// Message queue structure
#define QUEUE_SIZE 50  // Hold up to 50 messages in queue
typedef struct {
  uint8_t senderMAC[6];
  char message[256];  // Full formatted message with MAC prefix
  bool valid;
} QueuedMessage;

QueuedMessage messageQueue[QUEUE_SIZE];
volatile int queueWriteIndex = 0;  // Next position to write (callback modifies this)
int queueReadIndex = 0;  // Next position to read (main loop modifies this)
volatile int queueCount = 0;  // Number of messages in queue

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  // Extract sender MAC from packet info
  const uint8_t* senderMAC = info->src_addr;
  
  // Add sender as peer if not already added (needed for reliable communication)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, senderMAC, 6);
  peerInfo.channel = info->rx_ctrl ? info->rx_ctrl->channel : 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);  // Will return ESP_ERR_ESPNOW_EXIST if already exists, which is fine
  
  if (len < 1) return;
  
  uint8_t msgType = data[0];
  
  // Handle debug monitor beacon (ignore our own beacons)
  if (msgType == MSG_DEBUG_MONITOR_BEACON) {
    return;
  }
  
  // Check if this is a debug message (minimum size: msgType + at least 1 byte of message)
  if (msgType == MSG_DEBUG && len >= 2) {
    // Copy data immediately - ESP-NOW callback buffer may be reused by next message
    // Allocate buffer on stack to hold the message
    uint8_t messageBuffer[sizeof(debug_message)];
    memset(messageBuffer, 0, sizeof(messageBuffer));
    
    // Copy only the actual received length (don't copy beyond what we received)
    size_t copyLen = (len < sizeof(messageBuffer)) ? len : sizeof(messageBuffer);
    memcpy(messageBuffer, data, copyLen);
    
    // Now parse from our local copy
    debug_message* msg = (debug_message*)messageBuffer;
    
    // Safety: ensure null termination at end of buffer
    msg->message[sizeof(msg->message) - 1] = '\0';
    
    // Validate: message should have msgType (1 byte) + message string + null terminator
    // Check that we received at least the msgType and a null terminator
    if (len < 2) {
      // Too short - skip
      return;
    }
    
    // Find actual message length by looking for null terminator within received data
    size_t maxMsgLen = len - 1;  // Subtract 1 for msgType
    if (maxMsgLen > sizeof(msg->message)) {
      maxMsgLen = sizeof(msg->message);
    }
    
    // Ensure we have a null terminator within the received data
    bool hasNullTerminator = false;
    for (size_t i = 0; i < maxMsgLen; i++) {
      if (msg->message[i] == '\0') {
        hasNullTerminator = true;
        break;
      }
    }
    
    if (!hasNullTerminator) {
      // No null terminator found - message appears corrupted/truncated
      return;
    }
    
    // Add message to queue (non-blocking, fast operation)
    // Check if queue has space
    if (queueCount >= QUEUE_SIZE) {
      // Queue full - drop oldest message (overwrite)
      queueReadIndex = (queueReadIndex + 1) % QUEUE_SIZE;
      queueCount--;
    }
    
    // Build the complete output string
    char output[256];
    int offset = 0;
    
    // Add MAC prefix
    output[offset++] = '[';
    for (int i = 0; i < 6; i++) {
      if (senderMAC[i] < 0x10) {
        output[offset++] = '0';
      }
      offset += sprintf(output + offset, "%02X", senderMAC[i]);
      if (i < 5) {
        output[offset++] = ':';
      }
    }
    offset += sprintf(output + offset, "] %s", msg->message);
    
    // Copy to queue
    int writePos = queueWriteIndex;
    memcpy(messageQueue[writePos].senderMAC, senderMAC, 6);
    strncpy(messageQueue[writePos].message, output, sizeof(messageQueue[writePos].message) - 1);
    messageQueue[writePos].message[sizeof(messageQueue[writePos].message) - 1] = '\0';
    messageQueue[writePos].valid = true;
    
    // Update queue indices (atomic operations for thread safety)
    queueWriteIndex = (writePos + 1) % QUEUE_SIZE;
    queueCount++;
  }
}

void sendBeacon() {
  uint8_t monitorMAC[6];
  WiFi.macAddress(monitorMAC);
  
  debug_monitor_beacon_message beacon;
  beacon.msgType = MSG_DEBUG_MONITOR_BEACON;
  memcpy(beacon.monitorMAC, monitorMAC, 6);
  
  esp_now_send(broadcastMAC, (uint8_t*)&beacon, sizeof(beacon));
}

void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(8192);  // Large RX buffer for high-volume debug output
  Serial.setTxBufferSize(8192);  // Large TX buffer to handle burst messages
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("ESP-NOW Debug Monitor");
  Serial.println("========================================");
  Serial.println();
  Serial.println("This monitor receives debug messages from the pedal receiver.");
  Serial.println("Waiting for receiver to start sending debug messages...");
  Serial.println();
  
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.disconnect();
  delay(100);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  Serial.println("ESP-NOW initialized");
  Serial.println();
  
  // Print MAC address
  Serial.print("Debug Monitor MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  
  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);
  
  // Add broadcast peer
  esp_now_peer_info_t broadcastPeer = {};
  memcpy(broadcastPeer.peer_addr, broadcastMAC, 6);
  broadcastPeer.channel = 0;
  broadcastPeer.encrypt = false;
  esp_now_add_peer(&broadcastPeer);
  
  Serial.println("Broadcasting beacon - clients will connect automatically");
  Serial.println();
  
  // Initialize message queue
  memset(messageQueue, 0, sizeof(messageQueue));
  queueWriteIndex = 0;
  queueReadIndex = 0;
  queueCount = 0;
}

void loop() {
  // Broadcast beacon periodically so clients can discover and connect
  unsigned long currentTime = millis();
  if (currentTime - lastBeaconSend > BEACON_SEND_INTERVAL) {
    sendBeacon();
    lastBeaconSend = currentTime;
  }
  
  // Process message queue - output to Serial and remove from queue
  while (queueCount > 0) {
    int readPos = queueReadIndex;
    if (messageQueue[readPos].valid) {
      // Output message to Serial
      Serial.println(messageQueue[readPos].message);
      
      // Mark as invalid and remove from queue
      messageQueue[readPos].valid = false;
      queueReadIndex = (readPos + 1) % QUEUE_SIZE;
      queueCount--;
    } else {
      // Invalid entry - skip it
      queueReadIndex = (readPos + 1) % QUEUE_SIZE;
      queueCount--;
    }
  }
  
  delay(10);  // Small delay to prevent tight loop
}

