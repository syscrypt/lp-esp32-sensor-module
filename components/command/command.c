#include "command.h"
#include "config.h"
#include "constants.h"
#include "storage.h"
#include "util.h"
#include "config.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define COMMAND_PRINT_STRING 0x01
#define COMMAND_SYNC_TIME 0x02
#define COMMAND_WRITE_CONFIG 0x03
#define COMMAND_RESTART 0x04
#define COMMAND_ERASE_DATA 0x05

static const char *TAG = "command";

int process_command_batch(const uint8_t *args, const uint16_t length) {
  for (uint16_t i = 0; i < length; ++i) {
    if (args[i + 1] == 0) {
      return COMMAND_ERR_BAD_REQUEST;
    }

    const uint8_t len = args[i + 1];
    uint8_t buf[len];
    memcpy(buf, &args[i+2], len);

    switch (args[i]) {
    case (COMMAND_PRINT_STRING): {
      ESP_LOGI(TAG, "Command Print String");
      uint8_t print_buf[len + 1];
      memcpy(print_buf, buf, len);
      print_buf[len] = (char)'\0';
      printf("Cmd 0x01 [%d bytes]: %s\n", len, print_buf);
      fflush(stdout);
      break;
    }
    case (COMMAND_SYNC_TIME): {
      ESP_LOGI(TAG, "Command Sync Time");
      struct tm timeinfo;
      timeinfo.tm_sec = buf[0];
      timeinfo.tm_min = buf[1];
      timeinfo.tm_hour = buf[2];
      timeinfo.tm_mday = buf[3];
      timeinfo.tm_mon  = buf[4];
      timeinfo.tm_year = buf[5];
      timeinfo.tm_wday = buf[6];
      util_set_current_time(&timeinfo);
      break;
    }
    case (COMMAND_WRITE_CONFIG): {
      ESP_LOGI(TAG, "config changed");
      uint32_t sd = 0;
      if(buf[0] == 0x01 || buf[0] == 0x02) {
        sd = ((buf[1] | sd) << 24) | ((buf[2] | sd) << 16) | ((buf[3] | sd) << 8) | (buf[4] | sd);
      }

      switch(buf[0]) {
        case(0x01): get_config()->deep_sleep_ms = sd;
          break;
        case(0x02):
          get_config()->wakeup_counter_threshold = sd;
          break;
        case(0x03):
          get_config()->activated_protocol = buf[1];
          break;
        case(0x04):
          get_config()->toggle_config_mode = buf[1];
          break;
        case(0x05):
          get_config()->toggle_deep_sleep = buf[1];
          break;
        case(0x06):
          get_config()->send_last_x_values = buf[1];
          break;
        case(0x07):
          get_config()->toggle_wakeup_threshold = buf[1];
          break;
        default: {
          ESP_LOGE(TAG, "unknown config property: %hhu", buf[0]);
        }
      }
      if(storage_write_config() != 0) {
        ESP_LOGE(TAG, "error writing config to flash");
      }
    } break;
    case(COMMAND_RESTART): {
      ESP_LOGI(TAG, "Command restart");
      vTaskDelay(100/portTICK_RATE_MS);
      esp_restart();
    } break;
    case(COMMAND_ERASE_DATA): {
      ESP_LOGI(TAG, "Command erase partition");
      nvs_flash_erase_partition(STORAGE_DATA_NAME);
    } break;
    default:
      return 0;
    }
    i += args[i + 1] + 1;
  }

  return 0;
}

