#include "constants.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_event.h"

static char *TAG = "storage";
RTC_DATA_ATTR uint32_t entry_batch_count = 0;
RTC_DATA_ATTR uint32_t wake_up_counter = 0;

void util_setup_nv_storage() {
  esp_err_t err = nvs_flash_init();
  ESP_ERROR_CHECK(err);

  err = nvs_flash_init_partition(STORAGE_DATA_NAME);
  ESP_ERROR_CHECK(err);
}

void util_get_current_time(struct tm *timeinfo) {
  time_t rawtime;
  time(&rawtime);
  struct tm *loctime = localtime(&rawtime);
  *timeinfo = *loctime;
}

void util_set_current_time(struct tm *timeinfo) {
  struct timeval val;
  char strftime_buf[64];
  const time_t sec = mktime(timeinfo);
  val.tv_sec = sec;
  localtime(&sec);
  settimeofday(&val, NULL);

  strftime(strftime_buf, sizeof(strftime_buf), "%c", timeinfo);
  ESP_LOGI("Timestamp", "The current date/time is set to: %s", strftime_buf);
}

uint32_t util_get_entry_batch_count() {
  return entry_batch_count;
}

uint32_t util_increment_entry_batch_count() {
  return ++entry_batch_count;
}

void util_set_entry_batch_count(uint32_t count) {
  entry_batch_count = count;
}

uint32_t util_get_wake_up_counter() { return wake_up_counter; }

uint32_t util_increment_wake_up_counter() { return ++wake_up_counter; }

void util_set_wake_up_counter(uint32_t count) { wake_up_counter = count; }
