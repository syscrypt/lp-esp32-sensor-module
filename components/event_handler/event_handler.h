#pragma once

#include "esp_event.h"

#define EVENT_BASE_RECEIVE_DATA "event_base_receive_data"
#define EVENT_RECEIVE_DATA_ID 1337
#define EVENT_RECEIVE_DATA_QUEUE_SIZE 32
#define EVENT_RECEIVE_DATA_TASK_NAME "receive_data_task"
#define EVENT_LOOP_TASK_STACK_SIZE 4096

esp_event_loop_handle_t event_loop_receive_data_handle;

void run_on_receive_data_event(void *handler_arg, esp_event_base_t base,
                               int32_t id, void *event_data);
