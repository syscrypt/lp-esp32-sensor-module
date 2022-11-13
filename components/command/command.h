#pragma once

#include <stdint.h>

#define COMMAND_ERR_BAD_REQUEST 1

int process_command_batch(const uint8_t *args, const uint16_t length);
