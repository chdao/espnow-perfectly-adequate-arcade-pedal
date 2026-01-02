#ifndef RECEIVER_ESPNOW_TRANSPORT_H
#define RECEIVER_ESPNOW_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "../shared/messages.h"

#define ESPNOW_INIT_RETRY_COUNT 3
#define ESPNOW_INIT_RETRY_DELAY_MS 100
#define ESPNOW_DEINIT_DELAY_MS 10
#define WIFI_MODE_DELAY_MS 100

// ESP-NOW transport abstraction for receiver
typedef struct {
  bool initialized;
} ReceiverEspNowTransport;

typedef void (*ReceiverMessageCallback)(const uint8_t* senderMAC, const uint8_t* data, int len, uint8_t channel);

void receiverEspNowTransport_init(ReceiverEspNowTransport* transport);
bool receiverEspNowTransport_send(ReceiverEspNowTransport* transport, const uint8_t* mac, const uint8_t* data, int len);
bool receiverEspNowTransport_addPeer(ReceiverEspNowTransport* transport, const uint8_t* mac, uint8_t channel);
void receiverEspNowTransport_registerReceiveCallback(ReceiverEspNowTransport* transport, ReceiverMessageCallback callback);
void receiverEspNowTransport_broadcast(ReceiverEspNowTransport* transport, const uint8_t* data, int len);

#endif // RECEIVER_ESPNOW_TRANSPORT_H

