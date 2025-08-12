/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ethernet.h"
#include "nvram.h"

static const char *TAG = "esp32s3-w5500";

void app_main(void) {
  ESP_LOGI(TAG, "esp32s3-w5500");

  nvram_init();

  net_iface_init();
}
