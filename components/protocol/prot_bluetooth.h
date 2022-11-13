#pragma once

#include "transfer_protocol.h"

int ble_init(void *self, void *args);

int ble_send(void *self, const char *data, int len);

int ble_recv(void *self, char* dest, int* len);

int ble_destroy(void *self);

RTC_FAST_ATTR static const transfer_protocol_type_t ble_impl = {
    .identifier_byte = 0x03,
    .init = ble_init,
    .send = ble_send,
    .recv = ble_recv,
    .destroy = ble_destroy,
};
