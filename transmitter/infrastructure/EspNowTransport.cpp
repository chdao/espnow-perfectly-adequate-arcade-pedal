#include "EspNowTransport.h"
#include <esp_now.h>
#include <WiFi.h>
#include <string.h>
#include <Arduino.h>
#include "../shared/messages.h"

static MessageReceivedCallback g_receiveCallback = nullptr;

void OnDataRecvWrapper(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (g_receiveCallback) {
    uint8_t* senderMAC = (uint8_t*)info->src_addr;
    uint8_t channel = info->rx_ctrl ? info->rx_ctrl->channel : 0;
    g_receiveCallback(senderMAC, data, len, channel);
  }
}

void espNowTransport_init(EspNowTransport* transport) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Retry ESP-NOW initialization
  int retries = ESPNOW_INIT_RETRY_COUNT;
  esp_err_t result = ESP_FAIL;
  
  while (retries > 0 && result != ESP_OK) {
    result = esp_now_init();
    if (result == ESP_OK) {
      transport->initialized = true;
      return;
    }
    // If ESP-NOW was already initialized, deinit and retry
    if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      esp_now_deinit();
      delay(ESPNOW_DEINIT_DELAY_MS);
    } else {
      delay(ESPNOW_INIT_RETRY_DELAY_MS);
    }
    retries--;
  }
  
  // If all retries failed, mark as not initialized
  transport->initialized = false;
}

bool espNowTransport_send(EspNowTransport* transport, const uint8_t* mac, const uint8_t* data, int len) {
  if (!transport->initialized) return false;
  
  esp_err_t result = esp_now_send(mac, data, len);
  return (result == ESP_OK);
}

bool espNowTransport_addPeer(EspNowTransport* transport, const uint8_t* mac, uint8_t channel) {
  if (!transport->initialized) return false;
  
  esp_now_peer_info_t peerInfo = {};
  macCopy(peerInfo.peer_addr, mac);
  peerInfo.channel = channel;
  peerInfo.encrypt = false;
  
  esp_err_t result = esp_now_add_peer(&peerInfo);
  return (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST);
}

void espNowTransport_registerReceiveCallback(EspNowTransport* transport, MessageReceivedCallback callback) {
  if (!transport->initialized) return;
  
  g_receiveCallback = callback;
  esp_now_register_recv_cb(OnDataRecvWrapper);
}

void espNowTransport_broadcast(EspNowTransport* transport, const uint8_t* data, int len) {
  uint8_t broadcastMAC[] = BROADCAST_MAC;
  espNowTransport_send(transport, broadcastMAC, data, len);
}

