#include "prot_config.h"
#include "device.h"
#include "event_handler.h"
#include "storage.h"
#include "config.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// Pass these information via init paramers
#define DEVELOPMENT_MAXIMUM_RETRY 128
#define PORT 3333
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

const char *TAG = "prot config";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static char addr_str[128];
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < DEVELOPMENT_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

static void do_retransmit(int sock) {
  int len;
  unsigned char rx_buffer[1024];

  do {
    len = recv(sock, &rx_buffer[3], sizeof(rx_buffer) - 3, 0);
    if (len < 0) {
      //ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
    } else if (len == 0) {
      ESP_LOGW(TAG, "Connection closed");
    } else {
      int response_len = 0;
      unsigned char resp_buf[2048];
      switch (rx_buffer[3]) {
      case 0x0b: {
        resp_buf[0] = 0x00;
        resp_buf[1] = 0x00;
        response_len = 2;
      } break;
      case 0x0c: {
        // Get config
        ESP_LOGI(TAG, "send config response");
        storage_read_config();
        memcpy(resp_buf, &get_config()->deep_sleep_ms,
               sizeof(get_config()->deep_sleep_ms));
        memcpy(&resp_buf[4], &get_config()->wakeup_counter_threshold,
               sizeof(get_config()->wakeup_counter_threshold));
        resp_buf[8] = get_config()->activated_protocol;
        resp_buf[9] = get_config()->send_last_x_values;
        resp_buf[10] = get_config()->toggle_config_mode;
        resp_buf[11] = get_config()->toggle_deep_sleep;
        resp_buf[12] = get_config()->toggle_wakeup_threshold;
        response_len = 13;
      } break;
      case 0x0d: {
        ESP_LOGI(TAG, "process command batch");
        // Command event
        rx_buffer[1] = 0x02;
        rx_buffer[2] = ((uint32_t)len & (uint32_t)0x0000FF00) >> 8;
        rx_buffer[3] = (uint32_t)len & (uint32_t)0x000000FF;
        esp_event_post_to(event_loop_receive_data_handle,
                          EVENT_BASE_RECEIVE_DATA, EVENT_RECEIVE_DATA_ID,
                          (void *)&rx_buffer[1], len + 3, 1);
        response_len = 1;
        resp_buf[0] = 0x00;
      } break;
      case 0x0e: {
        ESP_LOGI(TAG, "dump sensor data");
        if(len < 9) {
          ESP_LOGE(TAG, "unknown request");
          break;
        }
        uint32_t from = 0;
        uint32_t to = 0;
        from = ((rx_buffer[4] | from) << 24) | ((rx_buffer[5] | from) << 16) | ((rx_buffer[6] | from) << 8) | (rx_buffer[7] | from);
        to = ((rx_buffer[8] | to) << 24) | ((rx_buffer[9] | to) << 16) | ((rx_buffer[10] | to) << 8) | (rx_buffer[11] | to);
        response_len = (int)storage_read_entry_batch(from, to, resp_buf);
      } break;
      case 0x0f: {
        ESP_LOGI(TAG, "request is maintenance mode");
        response_len = 1;
        resp_buf[0] = 0x00;
      } break;
      default: {
        ESP_LOGE(TAG, "unknown config command");
      } break;
      }

      if (response_len > 0) {
        ESP_LOGI(TAG, "write response");
        int written = send(sock, resp_buf, response_len, 0);
        if (written < 0) {
          ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        }
      }
    }
  } while (len > 0);
}

int config_recv(void *self, char* dest, int* len) {
  int listen_sock = 0;
  int addr_family = AF_INET;
  int ip_protocol = 0;
  struct sockaddr_in6 dest_addr;

  while (1) {
    if (addr_family == AF_INET) {
      struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
      dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
      dest_addr_ip4->sin_family = AF_INET;
      dest_addr_ip4->sin_port = htons(PORT);
      ip_protocol = IPPROTO_IP;
    } else if (addr_family == AF_INET6) {
      bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
      dest_addr.sin6_family = AF_INET6;
      dest_addr.sin6_port = htons(PORT);
      ip_protocol = IPPROTO_IPV6;
    }

    listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket created");

    int err =
        bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
      ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
      ESP_LOGE(TAG, "Error occured during listen: %d", errno);
      goto CLEAN_UP;
    }

    while (1) {
      ESP_LOGI(TAG, "Socket listening");

      struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
      uint addr_len = sizeof(source_addr);
      int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
      if (sock < 0) {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        break;
      }

      // Convert ip address to string
      if (source_addr.sin6_family == PF_INET) {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr,
                    addr_str, sizeof(addr_str) - 1);
      } else if (source_addr.sin6_family == PF_INET6) {
        inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
      }
      ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

      do_retransmit(sock);
      shutdown(sock, 0);
      close(sock);
    }
  }

CLEAN_UP:
  close(listen_sock);
  vTaskDelete(NULL);
  return PROTOCOL_OP_SUCCEEDED;
}

int config_init(void *self, void *args) {
  ESP_ERROR_CHECK(esp_netif_init());

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  int err = esp_wifi_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE("wifi", "initalization failed with error code %d", err);
  }

  s_wifi_event_group = xEventGroupCreate();

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = DEVELOPMENT_SSID,
              .password = DEVELOPMENT_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,

              .pmf_cfg = {.capable = true, .required = false},
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", DEVELOPMENT_SSID,
             DEVELOPMENT_PASSWORD);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", DEVELOPMENT_SSID,
             DEVELOPMENT_PASSWORD);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  /* The event will not be processed after unregister */
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);

  return ESP_OK;
}

int config_send(void *self, const char *data, int len) {
  /*
  if (sock != -1) {
    int err = sendto(sock, data, len, 0, (struct sockaddr *)&dest_addr,
                     sizeof(dest_addr));
    if (err < 0) {
      ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
      return PROTOCOL_OP_FAILED;
    }
    ESP_LOGI(TAG, "Message sent");
    ESP_LOGI(TAG, "Shutting down socket");
    shutdown(sock, 0);
    close(sock);
  }
  */
  return PROTOCOL_OP_SUCCEEDED;
}

int config_destroy(void *self) {
  ESP_LOGI(TAG, "shutting down wifi...");
  esp_wifi_stop();
  esp_wifi_deinit();
  return PROTOCOL_OP_SUCCEEDED;
}
