#pragma once

#include "transfer_protocol.h"

#include <stdint.h>
#include <string.h>

#define ZIGBEE_ADDRESS_LENGTH 8

int zigbee_init(void *self, void *args);

int zigbee_send(void *self, const char *data, int len);

int zigbee_recv(void *self, char* dest, int* len);

int zigbee_destroy(void *self);

RTC_FAST_ATTR static const transfer_protocol_type_t zigbee_impl = {
    .identifier_byte = 0x02,
    .init = zigbee_init,
    .send = zigbee_send,
    .recv = zigbee_recv,
    .destroy = zigbee_destroy,
};
