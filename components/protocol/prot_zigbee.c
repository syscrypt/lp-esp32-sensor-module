#include "prot_zigbee.h"
#include "command.h"
#include "util.h"
#include "device.h"
#include "event_handler.h"
#include "transfer_protocol.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal/gpio_types.h"

#include <malloc.h>
#include <string.h>

#define EMPTY_FRAME_SIZE 18
#define TRANSMIT_FRAME_PARAMS "\x10\x01\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\x00\x00"
#define TXD_PIN (GPIO_NUM_12)
#define RXD_PIN (GPIO_NUM_14)
#define UART_BUFFER_SIZE 2048

static const char* TAG = "zigbee module";

void setup_uart() {
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
  };

  uart_driver_install(UART_NUM_1, UART_BUFFER_SIZE, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
}

int zigbee_init(void *self, void *args) {
  setup_uart();
  return PROTOCOL_OP_SUCCEEDED;
}

int zigbee_send(void *self, const char *data, int len) {
  if (len == 0) {
    return PROTOCOL_ERR_SEND_DATA_LENGTH_ZERO;
  }
  uint16_t local_len = (uint16_t)len;

  const int total_len = EMPTY_FRAME_SIZE + local_len;

  uint16_t acc_len = 0x0e + local_len;
  uint8_t len_low = (acc_len << 8 >> 8) & acc_len;
  uint8_t len_high = acc_len >> 8;

  uint8_t *frame = (uint8_t *)malloc(total_len);
  memcpy(&frame[0], "\x7e", 1);
  frame[1] = len_high;
  frame[2] = len_low;
  memcpy(&frame[3], TRANSMIT_FRAME_PARAMS, 14);
  memcpy(&frame[17], data, local_len);

  uint16_t checksum = 0;
  for (int i = 3; i < total_len - 1; i++) {
    checksum += frame[i];
  }
  checksum = checksum & 0x00ff;
  checksum = 0xff00 - checksum;

  frame[total_len - 1] = checksum - 1;
  uart_write_bytes(UART_NUM_1, frame, total_len);

  free(frame);
  return PROTOCOL_OP_SUCCEEDED;
}

int zigbee_recv(void *self, char* dest, int* len) {
  uint8_t recv_buf[UART_BUFFER_SIZE];
  uint8_t copy_buf[512];
  int length = 0;

  while(1) {
    length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t)&length));
    length = uart_read_bytes(UART_NUM_1, &recv_buf[1], UART_BUFFER_SIZE - 1, 100);
    if (length > 0) {
      uint8_t type = recv_buf[4];
      ESP_LOGI(TAG, "received %d bytes from zigbee protocol", length);
      switch(type) {
      case (0x95): {
      } break;
      case (0x90): {
        const uint16_t msg_length = (uint16_t)recv_buf[3] + ((uint16_t)recv_buf[2] << 8) - 12;
        copy_buf[0] = 0x03;
        copy_buf[1] = recv_buf[2];
        copy_buf[2] = recv_buf[3];
        memcpy(&copy_buf[3], &recv_buf[16], msg_length);
        esp_event_post_to(event_loop_receive_data_handle,
                          EVENT_BASE_RECEIVE_DATA, EVENT_RECEIVE_DATA_ID,
                          (void *)copy_buf, msg_length + 3, 1);
        break;
      }
      default:
        recv_buf[0] = 0xff;
        esp_event_post_to(event_loop_receive_data_handle,
                          EVENT_BASE_RECEIVE_DATA, EVENT_RECEIVE_DATA_ID,
                          (void *)recv_buf, length + 2, 1);
        break;
      }
    }
  }
}

int zigbee_destroy(void *self) {return PROTOCOL_OP_SUCCEEDED;}
