#include "routines.h"
#include "sensor.h"
#include "event_handler.h"
#include "constants.h"
#include "storage.h"
#include "util.h"
#include "transfer_protocol.h"

#include "bmp280.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2cdev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

static uint8_t sleep_command = EVENT_TASKS_COMPLETED;
static char data[32] = {0};
static const char* TAG = "routines";
static int toggle_wakeup = 0;

void init_and_read_sensor_values(void* pvParameter) {
  ESP_ERROR_CHECK(i2cdev_init());
  float temp, pressure, humidity;
  bmp280_t dev;
  init_sensor(&dev);

  // some delay to give the sensor enough time for initialization (min 41ms)
  vTaskDelay(45 / portTICK_PERIOD_MS);
  read_all_values(&dev, &temp, &humidity, &pressure);
  ESP_LOGI("main", "%.2f°C, %.2f%%, %.2fhP", temp, humidity, pressure/100);

  sprintf(data, "%.2f;%.2f;%.2f", temp, humidity, pressure/100.f);
  int len = (int)strlen(data);
  int err = storage_write_data_batch(data, len);
  if(err != ESP_OK) {
    ESP_LOGE(TAG, "store data batch in flash failed, errno: %d", err);
  } else {
    ESP_LOGI(TAG, "successfully stored sensor values in flash");
  }

  esp_event_post_to(event_loop_receive_data_handle, EVENT_BASE_RECEIVE_DATA,
                    EVENT_RECEIVE_DATA_ID, (void *)&sleep_command, 1, 1);

  vTaskDelete(NULL);
}

void create_task() {
  xTaskCreatePinnedToCore(*(init_and_read_sensor_values), "read vlaues", 4096,
                          NULL, 10, NULL, 1);
}

void create_config_task_wrapper() {
  transfer_protocol_type_t *config_protocol = get_protocol_by_id_byte((uint8_t)DEFAULT_CONFIG_PROTOCOL);
  if(config_protocol == NULL) {
    ESP_LOGE(TAG, "unable to find configuration protocol with byte: %hhu", config_protocol->identifier_byte);
    return;
  }
  config_protocol->recv(config_protocol, NULL, NULL);
}

void create_config_task() {
  xTaskCreatePinnedToCore(*(create_config_task_wrapper), "config mode", 8192,
                          NULL, 10, NULL, 0);
}
