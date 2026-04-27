/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */
#include "esp_log.h"
#include "ethernet.h"
#include "event_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvram.h"
#include "prov.h"
#include "sdkconfig.h"

#include <stdio.h>

static const char* TAG = "main";

void app_main(void) {
  ESP_LOGI(TAG, "esp32s3-w5500");

  nvram_init();
  event_handler_init();
  net_iface_init();
  provision_init();
}
