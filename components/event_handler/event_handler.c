#include "event_handler.h"
#include "util.h"
#include "constants.h"
#include "storage.h"
#include "command.h"
#include "transfer_protocol.h"
#include "config.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"

#include <string.h>

static char* TAG = "EVENT HANDLER";

void send_data();

void run_on_receive_data_event(void *handler_arg, esp_event_base_t base,
                               int32_t id, void *event_data) {
  uint8_t* data = (uint8_t*)event_data;
  switch(data[0]) {
  case(0x01): {
    if (get_config()->toggle_config_mode || !get_config()->toggle_deep_sleep) {
      return;
    }
    ESP_LOGI(TAG, "entering deep sleep mode");
    esp_sleep_enable_timer_wakeup((uint64_t)get_config()->deep_sleep_ms*1000);
    esp_sleep_enable_ext0_wakeup(EXT_WAKEUP_SOURCE, 1);
    esp_deep_sleep_start();
  } break;
  case(0x02): {
    uint16_t len = (((uint16_t)data[1]) << 8) & 0xFF00;
    len |= (uint16_t)data[2];
    ESP_LOGI(TAG, "process command batch, len: %hu", len);
    process_command_batch(&data[3], len);
  } break;
  case(0x03): {
    send_data();
   } break;
  default:
    ESP_LOGE(TAG, "unknown event received"); break;
  }
}

void send_data() {
  transfer_protocol_type_t *prot =
      get_protocol_by_id_byte(get_config()->activated_protocol);
  if (prot == NULL) {
    ESP_LOGE(TAG, "sending data failed, protocol %hhu not found",
             get_config()->activated_protocol);
  } else {
    ESP_LOGI(TAG, "send data event");
    uint8_t to_send[2048];
    uint32_t len;
    if (get_config()->send_last_x_values == 0) {
      for (int i = 0; i < util_get_entry_batch_count(); i += 50) {
        esp_task_wdt_reset();
        len = storage_read_entry_batch(i, i + 50, to_send);
        prot->send(prot, (const char *)to_send, len);
      }
      nvs_flash_erase_partition(STORAGE_DATA_NAME);
      util_set_entry_batch_count(0);
    } else {
      int batch_count = util_get_entry_batch_count();
      len = storage_read_entry_batch(batch_count - get_config()->send_last_x_values,
                                     batch_count, to_send);
      prot->send(prot, (const char *)to_send, len);
    }
  }
}
