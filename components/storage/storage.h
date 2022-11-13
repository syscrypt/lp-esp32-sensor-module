#pragma once

#include "config.h"

#define STORAGE_ERR_BAD_REQUEST 1

int storage_read_config();

int storage_write_config();

void storage_print_entry_batch(uint32_t from, uint32_t to);

int storage_write_data(char type, const char *data, int len);

int storage_write_data_batch(const char *data, int len);

uint32_t storage_compute_entry_batch_count();

/*
  * Fetches a block of persisted sensor data and returns the length of that
  * block. The individual values are zero terminated.
  */
uint32_t storage_read_entry_batch(uint32_t from, uint32_t to, uint8_t * dest);
