#include "transfer_protocol.h"
#include "config.h"
#include "constants.h"

#include "storage.h"
#include "esp_log.h"

// User defined protocols
#include "prot_wifi.h"
#include "prot_zigbee.h"
#include "prot_bluetooth.h"
#include "prot_config.h"

static const char* TAG = "user init";

int init_protocols() {
  uint8_t prot_byte = get_config()->activated_protocol;
  ESP_LOGE(TAG, "activated protocol: %hhu", get_config()->activated_protocol);

  if(get_config()->toggle_config_mode) {
    ESP_LOGI(TAG, "entering maintenance mode");
    prot_byte = (uint8_t)DEFAULT_CONFIG_PROTOCOL;
  }

  transfer_protocol_type_t* prot = get_protocol_by_id_byte(prot_byte);
  if (prot != NULL) {
    ESP_LOGE(TAG, "fetched protocol: %hhu", prot->identifier_byte);
    return prot->init(prot, NULL);
  }
  return 0;
}

void register_protocols() {
  ESP_LOGW(TAG, "register");
  register_protocol(&wifi_impl);
  register_protocol(&zigbee_impl);
  register_protocol(&ble_impl);
  register_protocol(&config_impl);
}

void deinitialize_protocols() {
  transfer_protocol_type_t *prot = get_protocol_by_id_byte(get_config()->activated_protocol);
  if (prot != NULL) {
    return prot->destroy(prot);
  }
}
