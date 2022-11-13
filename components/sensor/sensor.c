#include "sensor.h"
#include "bmp280.h"

#include <stdio.h>
#include <string.h>

#define SDA_GPIO 32
#define SCL_GPIO 33

void init_sensor(void* sensor) {
  bmp280_params_t params;
  bmp280_init_default_params(&params);
  memset((bmp280_t*)sensor, 0, sizeof(bmp280_t));
  ESP_ERROR_CHECK(bmp280_init_desc((bmp280_t*)sensor, BMP280_I2C_ADDRESS_0, 0, SDA_GPIO, SCL_GPIO));
  ESP_ERROR_CHECK(bmp280_init((bmp280_t*)sensor, &params));
  bool bme280p = ((bmp280_t*)sensor)->id == BME280_CHIP_ID;
}

void read_all_values(void* sensor, float* temperature, float* humidity, float* pressure) {
  bmp280_read_float((bmp280_t*)sensor, temperature, pressure, humidity);
}
