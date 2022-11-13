#pragma once

#include <stdint.h>

#include "esp_attr.h"

#define PROTOCOL_OP_SUCCEEDED 0;
#define PROTOCOL_OP_FAILED -1;
#define PROTOCOL_ERR_SEND_DATA_LENGTH_ZERO -2;

typedef struct {
  uint32_t flag; // Don't set this field

  uint8_t identifier_byte;
  int (*init)(void *self, void *args);
  int (*send)(void *self, const char *data, int len);
  int (*recv)(void *self, char* dest, int* len);
  int (*destroy)(void *self);
} transfer_protocol_type_t;

transfer_protocol_type_t **get_prot_collection();

uint8_t get_protocol_count();

void register_protocol(transfer_protocol_type_t *prot);

transfer_protocol_type_t *get_protocol_by_id_byte(uint8_t id);

void register_protocols();

int init_protocols();

void deinitialize_protocols();
