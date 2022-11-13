#pragma once

#define DEVELOPMENT_SSID "SomeSSID"
#define DEVELOPMENT_PASSWORD "SomeWifiPassword"

#include "transfer_protocol.h"

#include <stdint.h>
#include <string.h>

int wifi_init(void *self, void *args);

int wifi_send(void *self, const char *data, int len);

int wifi_recv(void *self, char* dest, int* len);

int wifi_destroy(void *self);

RTC_DATA_ATTR static const transfer_protocol_type_t wifi_impl = {
    .identifier_byte = 0x01,
    .init = wifi_init,
    .send = wifi_send,
    .recv = wifi_recv,
    .destroy = wifi_destroy,
};
