#pragma once

#include "rtc_wake_stub.h"

#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_log.h"

static const char* TAG = "wake up routine";

int protocol_setup_finished = 0;
int components_initialized = 0;

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
  esp_default_wake_deep_sleep();

  if(!components_initialized) {
    protocol_setup_finished++;
    ESP_LOGI(TAG, "counter: %d", protocol_setup_finished);
    //components_initialized = 1;
  }
}
