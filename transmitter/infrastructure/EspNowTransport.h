#ifndef ESPNOW_TRANSPORT_H
#define ESPNOW_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "../shared/messages.h" // Corrected include path

#define ESPNOW_INIT_RETRY_COUNT 3
#define ESPNOW_INIT_RETRY_DELAY_MS 100
#define ESPNOW_DEINIT_DELAY_MS 10

// ESP-NOW transport abstraction
typedef struct {
  bool initialized;
} EspNowTransport;

typedef void (*MessageReceivedCallback)(const uint8_t* senderMAC, const uint8_t* data, int len, uint8_t channel);

void espNowTransport_init(EspNowTransport* transport);
bool espNowTransport_send(EspNowTransport* transport, const uint8_t* mac, const uint8_t* data, int len);
bool espNowTransport_addPeer(EspNowTransport* transport, const uint8_t* mac, uint8_t channel);
void espNowTransport_registerReceiveCallback(EspNowTransport* transport, MessageReceivedCallback callback);
void espNowTransport_broadcast(EspNowTransport* transport, const uint8_t* data, int len);

#endif // ESPNOW_TRANSPORT_H

