#pragma once

#define DEVELOPMENT_SSID "SomeSSID"
#define DEVELOPMENT_PASSWORD "SomePassword"

#include "transfer_protocol.h"

#include <stdint.h>

int config_init(void *self, void *args);

int config_send(void *self, const char *data, int len);

int config_recv(void *self, char *dest, int *len);

int config_destroy(void *self);

RTC_FAST_ATTR static const transfer_protocol_type_t config_impl = {
    .identifier_byte = 0xFF,
    .init = config_init,
    .send = config_send,
    .recv = config_recv,
    .destroy = config_destroy,
};
