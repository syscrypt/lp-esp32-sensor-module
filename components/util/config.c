#include "esp_attr.h"
#include "config.h"

RTC_DATA_ATTR config_t config = {
    .deep_sleep_ms = CONFIG_DEFAULT_DEEP_SLEEP_MS,
    .wakeup_counter_threshold = CONFIG_DEFAULT_WAKEUP_COUNTER_THRESHOLD,
    .activated_protocol = CONFIG_DEFAULT_ACTIVATED_PROTOCOL,

    .send_last_x_values = CONFIG_DEFAULT_SEND_LAST_X_VALUES,
    .toggle_config_mode = CONFIG_DEFAULT_TOGGLE_CONFIG_MODE,
    .toggle_deep_sleep = CONFIG_DEFAULT_TOGGLE_DEEP_SLEEP,
    .toggle_wakeup_threshold = CONFIG_DEFAULT_TOGGLE_WAKEUP_THRESHOLD
};

config_t* get_config() {
  return &config;
}
