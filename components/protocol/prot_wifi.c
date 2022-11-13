#include <stdio.h>
#include <string.h>

#include "prot_wifi.h"

#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// Pass these information via init parameters
#define DEVELOPMENT_MAXIMUM_RETRY 128

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define PORT 1337

static const char *TAG = "wifi station";

static int s_retry_num = 0;
static int sock;

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
      esp_restart();
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

int create_socket(struct sockaddr_in *dest_addr_ip4) {
  int addr_family = AF_INET;

  dest_addr_ip4->sin_addr.s_addr = inet_addr("255.255.255.255");
  dest_addr_ip4->sin_family = AF_INET;
  dest_addr_ip4->sin_port = htons(PORT);

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    return sock;
  }
  ESP_LOGI(TAG, "Socket created successfully");
  return sock;
}

int wifi_init(void *self, void *args) {
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

int wifi_send(void *self, const char *data, int len) {
  struct sockaddr_in dest_addr;
  create_socket(&dest_addr);

  if (sock != -1) {
    int err = sendto(sock, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
      ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
      return PROTOCOL_OP_FAILED;
    }
    ESP_LOGI(TAG, "Message sent");
    ESP_LOGI(TAG, "Shutting down socket");
    shutdown(sock, 0);
    close(sock);
  }
  return PROTOCOL_OP_FAILED;
}

int wifi_recv(void* self, char* dest, int* len) {
  return PROTOCOL_OP_SUCCEEDED;
}

int wifi_destroy(void* self) {
  if (sock != -1) {
    ESP_LOGI(TAG, "Shutting down socket and restarting...");
    shutdown(sock, 0);
    close(sock);
  }
  ESP_LOGI(TAG, "shutting down wifi...");
  esp_wifi_stop();
  esp_wifi_deinit();
  return PROTOCOL_OP_SUCCEEDED;
}
