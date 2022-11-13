#include "storage.h"
#include "util.h"
#include "config.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "constants.h"
#include "mbedtls/base64.h"
#include "esp_task_wdt.h"

#include <string.h>

static const char* TAG = "storage";

int storage_read_config() {
  nvs_handle_t nvs_handle;
  int err = nvs_open(STORAGE_NAME, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    return err;
  }

  int failed = 0;
  err = nvs_get_u32(nvs_handle, CONFIG_NAME_DEEP_SLEEP_MS, &get_config()->deep_sleep_ms);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u32(nvs_handle, CONFIG_NAME_WAKEUP_THRESHOLD, &get_config()->wakeup_counter_threshold);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u8(nvs_handle, CONFIG_NAME_SEND_LAST_X_VALUES, &get_config()->send_last_x_values);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u8(nvs_handle, CONFIG_NAME_ACTIVE_PROTOCOLS, &get_config()->activated_protocol);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u8(nvs_handle, CONFIG_NAME_TOGGLE_DEEP_SLEEP, &get_config()->toggle_deep_sleep);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u8(nvs_handle, CONFIG_NAME_TOGGLE_CONFIG_MODE, &get_config()->toggle_config_mode);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  err = nvs_get_u8(nvs_handle, CONFIG_NAME_TOGGLE_WAKEUP_THRESHOLD, &get_config()->toggle_wakeup_threshold);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    failed = err;
  }

  nvs_close(nvs_handle);
  return failed;
}

int storage_write_config() {
  nvs_handle_t nvs_handle;
  int err = nvs_open(STORAGE_NAME, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u32(nvs_handle, CONFIG_NAME_DEEP_SLEEP_MS, get_config()->deep_sleep_ms);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u32(nvs_handle, CONFIG_NAME_WAKEUP_THRESHOLD, get_config()->wakeup_counter_threshold);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u8(nvs_handle, CONFIG_NAME_SEND_LAST_X_VALUES, get_config()->send_last_x_values);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u8(nvs_handle, CONFIG_NAME_ACTIVE_PROTOCOLS, get_config()->activated_protocol);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u8(nvs_handle, CONFIG_NAME_TOGGLE_DEEP_SLEEP, get_config()->toggle_deep_sleep);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u8(nvs_handle, CONFIG_NAME_TOGGLE_CONFIG_MODE, get_config()->toggle_config_mode);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_u8(nvs_handle, CONFIG_NAME_TOGGLE_WAKEUP_THRESHOLD, get_config()->toggle_wakeup_threshold);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_commit(nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    return err;
  }

  nvs_close(nvs_handle);
  return 0;
}

int storage_write_data(char type, const char *data, int len) {
  struct tm timeinfo;
  util_get_current_time(&timeinfo);
  int total_len = 7 + len;
  size_t output_len;

  unsigned char b64output[total_len * 2];
  unsigned char buf[total_len + 1];
  buf[total_len] = (unsigned char)'\0';
  buf[0] = (unsigned char)timeinfo.tm_sec;
  buf[1] = (unsigned char)timeinfo.tm_min;
  buf[2] = (unsigned char)timeinfo.tm_hour;
  buf[3] = (unsigned char)timeinfo.tm_mday;
  buf[4] = (unsigned char)timeinfo.tm_mon;
  buf[5] = (unsigned char)timeinfo.tm_year;
  buf[6] = (unsigned char)timeinfo.tm_wday;
  memcpy(&buf[7], data, len);

  char out_buf[45];

  int cnt = 0;
  if(type == 't' || type == 'h' || type == 'p') {
    cnt = util_get_entry_batch_count();
  } else {
    ESP_LOGW("write_data", "bad type %c", type);
    return STORAGE_ERR_BAD_REQUEST;
  }

  sprintf(out_buf, "v_%c_%u", type, cnt);
  mbedtls_base64_encode(b64output, total_len * 2, &output_len, buf,
                        total_len);
  ESP_LOGI("write_data", "Time: %s, Value: %s, Type: %c",
            asctime(&timeinfo), &buf[7], type);

  nvs_handle_t nvs_handle;
  int err = nvs_open(STORAGE_NAME, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_str(nvs_handle, out_buf, (char*)b64output);
  if (err != ESP_OK) {
    ESP_LOGE("write_entry", "error(%d) writing entry", err);
    nvs_close(nvs_handle);
    return err;
  }
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK) {
    return err;
  }
  nvs_close(nvs_handle);

  util_increment_entry_batch_count();
  return ESP_OK;
}

int storage_write_data_batch(const char *data, int len) {
  struct tm timeinfo;
  util_get_current_time(&timeinfo);
  int total_len = 7 + len;
  size_t output_len;
  size_t b64len = total_len * 2;

  unsigned char b64output[b64len];
  unsigned char buf[total_len + 1];
  buf[total_len] = (unsigned char)'\0';
  buf[0] = (unsigned char)timeinfo.tm_sec;
  buf[1] = (unsigned char)timeinfo.tm_min;
  buf[2] = (unsigned char)timeinfo.tm_hour;
  buf[3] = (unsigned char)timeinfo.tm_mday;
  buf[4] = (unsigned char)timeinfo.tm_mon;
  buf[5] = (unsigned char)timeinfo.tm_year;
  buf[6] = (unsigned char)timeinfo.tm_wday;
  memcpy(&buf[7], data, len);

  char out_buf[64];

  sprintf(out_buf, "b_%u", util_get_entry_batch_count());
  mbedtls_base64_encode(b64output, b64len, &output_len, buf,
                        total_len);
  b64output[output_len] = 0;

  ESP_LOGI(TAG, "write_entry nr. %d", util_get_entry_batch_count());
  nvs_handle_t nvs_handle;
  //int err = nvs_open(STORAGE_DATA_NAME, NVS_READWRITE, &nvs_handle);
  int err = nvs_open_from_partition(STORAGE_DATA_NAME, STORAGE_DATA_NAME,
                                    NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    return err;
  }

  err = nvs_set_str(nvs_handle, out_buf, (char*)b64output);
  if (err != ESP_OK) {
    ESP_LOGE("write_entry", "error(%d) writing entry", err);
    nvs_close(nvs_handle);
    return err;
  }
  err = nvs_commit(nvs_handle);
  nvs_close(nvs_handle);
  if (err != ESP_OK) {
    return err;
  }

  util_increment_entry_batch_count();
  return ESP_OK;
}

uint32_t storage_compute_entry_batch_count() {
  nvs_handle_t nvs_handle;
  uint32_t count = 0;
  char query[15];

  //int err = nvs_open(STORAGE_DATA_NAME, NVS_READONLY, &nvs_handle);
  int err = nvs_open_from_partition(STORAGE_DATA_NAME, STORAGE_DATA_NAME, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "failed to compute entry batch, initialize with 0");
    nvs_close(nvs_handle);
    return count;
  }

  while(1) {
    sprintf(query, "b_%u", count);
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, query, NULL, &required_size);
    if (required_size > 0 || err == ESP_OK) {
      esp_task_wdt_reset();
      count++;
      continue;
    }

    nvs_close(nvs_handle);
    return count;
  }
}

void storage_print_entry_batch(uint32_t from, uint32_t to) {
  uint8_t dest[1024];
  int offset = 0;
  storage_read_entry_batch(from, to, dest);
  ESP_LOGW(TAG, "Batch Values: ");
  for (int i = 0; i < 5; i++) {
    int j = 0;
    char buf[128];
    while (dest[offset + j] != '\0') {
      buf[j] = dest[offset + j];
      j++;
    }
    buf[j + 1] = 0;

    ESP_LOGI(TAG, "entry nr %d: , %s", i, buf);
  }
}

uint32_t storage_read_entry_batch(uint32_t from, uint32_t to, uint8_t *dest) {
  nvs_handle_t nvs_handle;
  int err = nvs_open_from_partition(STORAGE_DATA_NAME, STORAGE_DATA_NAME, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    nvs_close(nvs_handle);
    ESP_LOGE(TAG, "error while reading entry batch, errno: %d", err);
    return 0;
  }

  uint32_t count = from;
  uint32_t offset = 0;

  while (1) {
    if (count > to) {
      return offset;
    }
    char query[15];
    sprintf(query, "b_%u", count);
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, query, NULL, &required_size);
    if (required_size > 0 && err == ESP_OK) {
      nvs_get_str(nvs_handle, query, (char *)&dest[offset], &required_size);
      offset += required_size;
    } else {
      break;
    }
    count++;
  }
  nvs_close(nvs_handle);
  return offset;
}
