/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "params.h"

static const char *TAG = "nvram";

struct nvram {
  struct nvram_net net;
  struct nvram_token token;
  struct nvram_mqtt mqtt;
  struct nvram_misc misc;
  struct nvram_fold folds[CONFIG_MAX_NUM_OF_SENSOR_TYPES];
  struct nvram_beacon beacon;

  bool prov_done;
  uint8_t device_id[DEVICE_ID_LEN];
  char device_id_str[DEVICE_ID_STR_LEN];
  uint8_t serial[SERIAL_LEN];
  char serial_str[SERIAL_STR_LEN];
  int32_t co_sensitivity; // for tgs5141 co sensor
};

static struct nvram nvdata;

void nvram_init(void) {
  memset(&nvdata, 0, sizeof(nvdata));

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGI(TAG, "nvs flash init failed");

    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}
