#pragma once

#include "esp_attr.h"

#include <stdint.h>

#define CONFIG_DEFAULT_DEEP_SLEEP_MS 5000
#define CONFIG_DEFAULT_TOGGLE_DEEP_SLEEP 1
#define CONFIG_DEFAULT_TOGGLE_CONFIG_MODE 1
#define CONFIG_DEFAULT_ACTIVATED_PROTOCOL 0x01
#define CONFIG_DEFAULT_WAKEUP_COUNTER_THRESHOLD 5
#define CONFIG_DEFAULT_TOGGLE_WAKEUP_THRESHOLD 1
#define CONFIG_DEFAULT_SEND_LAST_X_VALUES 2

#define CONFIG_NAME_DEEP_SLEEP_MS "sleep_dur"
#define CONFIG_NAME_WAKEUP_THRESHOLD "wake_thresh"
#define CONFIG_NAME_SEND_LAST_X_VALUES "send_x"
#define CONFIG_NAME_ACTIVE_PROTOCOLS "active_prot"
#define CONFIG_NAME_TOGGLE_DEEP_SLEEP "deep_sleep"
#define CONFIG_NAME_TOGGLE_CONFIG_MODE "config_mode"
#define CONFIG_NAME_TOGGLE_WAKEUP_THRESHOLD "t_wake_thresh"

typedef struct {
  uint32_t deep_sleep_ms;
  uint32_t wakeup_counter_threshold;
  uint8_t send_last_x_values;

  uint8_t activated_protocol;

  uint8_t toggle_deep_sleep;
  uint8_t toggle_config_mode;
  uint8_t toggle_wakeup_threshold;
} config_t;

config_t* get_config();
