#include "transfer_protocol.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "constants.h"

RTC_DATA_ATTR transfer_protocol_type_t *prot_collection[MAX_PROTOCOLS];
uint8_t protocol_count = 0;

uint8_t get_protocol_count() { return protocol_count; }

transfer_protocol_type_t **get_prot_collection() { return prot_collection; }

uint32_t power(int a, int pot) {
  uint32_t ret = 1;
  for (int i = 0; i < pot; ++i) {
    ret = ret * a;
  }
  return ret;
}

void register_protocol(transfer_protocol_type_t *prot) {
  prot_collection[protocol_count] = prot;
  prot->flag = power(2, (int)protocol_count);
  protocol_count++;
}

transfer_protocol_type_t *get_protocol_by_id_byte(uint8_t id) {
  for (int i = 0; i < MAX_PROTOCOLS; ++i) {
    if (prot_collection[i] != NULL) {
      if (prot_collection[i]->identifier_byte == id) {
        return prot_collection[i];
      }
    }
  }
  return NULL;
}
