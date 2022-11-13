#pragma once

#include "esp_attr.h"

#include <sys/time.h>
#include <time.h>

void util_setup_nv_storage();

void util_get_current_time(struct tm* timeinfo);

void util_set_current_time(struct tm *timeinfo);

uint32_t util_get_entry_batch_count();

uint32_t util_increment_entry_batch_count();

void util_set_entry_batch_count(uint32_t count);

uint32_t util_get_wake_up_counter();

uint32_t util_increment_wake_up_counter();

void util_set_wake_up_counter(uint32_t count);
