#include "sensor.h"
#include "routines.h"
#include "event_handler.h"
#include "util.h"
#include "storage.h"
#include "transfer_protocol.h"
#include "constants.h"
#include "nvs_flash.h"
#include "config.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#include <stddef.h>

static const char* TAG = "main";
static uint8_t send_data_command = EVENT_SEND_DATA;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

#define RTC_ERROR_THRESHOLD 16777216

void wakeup_reason(void);

static void IRAM_ATTR handleButtonInterrupt(void *arg) {
  ESP_LOGI(TAG, "inside isr");
  portENTER_CRITICAL_ISR(&mux);
  get_config()->toggle_config_mode = 0x01;
  util_set_wake_up_counter(0);
  esp_restart();
  portEXIT_CRITICAL_ISR(&mux);
}

void app_main()
{
  gpio_install_isr_service(0);
  gpio_set_intr_type(EXT_WAKEUP_SOURCE, GPIO_INTR_NEGEDGE);
  gpio_isr_handler_add(EXT_WAKEUP_SOURCE, handleButtonInterrupt, NULL);

  util_setup_nv_storage();

  esp_event_loop_args_t event_loop_receive_data_args = {
      .queue_size = EVENT_RECEIVE_DATA_QUEUE_SIZE,
      .task_name = EVENT_RECEIVE_DATA_TASK_NAME,
      .task_priority = 10,
      .task_stack_size = 8192,
      .task_core_id = 0};

  esp_event_loop_create(&event_loop_receive_data_args,
                        &event_loop_receive_data_handle);
  esp_event_handler_register_with(
      event_loop_receive_data_handle, EVENT_BASE_RECEIVE_DATA,
      EVENT_RECEIVE_DATA_ID, run_on_receive_data_event, NULL);

  esp_event_loop_create_default();

  storage_read_config();

  if ((get_config()->toggle_wakeup_threshold && util_get_wake_up_counter() + 1 >= get_config()->wakeup_counter_threshold) ||
      get_config()->toggle_config_mode) {
    register_protocols();
    init_protocols();
  }

  wakeup_reason();

  create_task();

  if(get_config()->toggle_config_mode) {
    create_config_task();
  }
}

void wakeup_reason(void) {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  if(util_get_entry_batch_count() >= RTC_ERROR_THRESHOLD) {
    util_set_entry_batch_count(storage_compute_entry_batch_count());
  }

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    ESP_LOGI(TAG, "external wakeup");
    get_config()->toggle_config_mode = 1;
    storage_write_config();
    util_set_wake_up_counter(0);
    esp_restart();
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    ESP_LOGI(TAG, "wake up counter: %u", util_increment_wake_up_counter());
    if(get_config()->toggle_wakeup_threshold && util_get_wake_up_counter() >= get_config()->wakeup_counter_threshold) {
      util_set_wake_up_counter(0);
      esp_event_post_to(event_loop_receive_data_handle, EVENT_BASE_RECEIVE_DATA,
                        EVENT_RECEIVE_DATA_ID, (void *)&send_data_command, 1, 1);
    }
    break;
  default:
    ESP_LOGW(TAG, "not registered wakeup source");
    util_set_entry_batch_count(storage_compute_entry_batch_count());
    ESP_LOGI(TAG, "initialize entry batch count, value: %d", util_get_entry_batch_count());
  }
}

