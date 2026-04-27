
/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "event_handler.h"
#include "nvram.h"
#include "prov.h"

#include <esp_err.h>
#include <esp_event.h>
#include <esp_event_base.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/idf_additions.h>

// #include "freertos/projdefs.h"
// #include "portmacro.h"

#define STACK_SIZE 4096
#define QUEUE_SIZE 10
#define PRIORITY 5

ESP_EVENT_DEFINE_BASE(APP_EVENT);

static const char* TAG = "event_handler";
static esp_event_loop_handle_t handle;

static void event_loop(void* handler_arg, esp_event_base_t base, int32_t id,
                       void* event_data) {
  // Event Handler Logic
  switch (id) {
  case EVT_TEST:
    ESP_LOGI(TAG, "EVT_TEST");
    break;
  case EVT_RECV_IP:
    ESP_LOGI(TAG, "EVT_RECV_IP");

    if (nvram_need_provisioning() == true) {
      // Start Provisioning
      event_send(EVT_PROVISIONING, NULL, 0);
    } else {
      ESP_LOGI(TAG, "TODO: Need to start services..");
      ESP_LOGI(TAG, "websocket, mqtt, beacon, dfu..");
    }
    break;

  case EVT_PROVISIONING:
    ESP_LOGI(TAG, "EVT_PROVISIONING");
    // Start Firmware Update and Config Download
    update_config();

    break;
  default:
    break;
  }
}

esp_err_t event_send(const EventID event_id, const void* event_msg,
                     const size_t event_msg_size) {
  return esp_event_post_to(handle, APP_EVENT, event_id, event_msg,
                           event_msg_size, 0);
}

esp_err_t event_handler_init(void) {
  esp_event_loop_args_t args = { .queue_size = QUEUE_SIZE,
                                 .task_name = "event_loop",
                                 .task_priority = PRIORITY,
                                 .task_stack_size = STACK_SIZE,
                                 .task_core_id = tskNO_AFFINITY };

  esp_event_loop_create(&args, &handle);
  return esp_event_handler_instance_register_with(
      handle, APP_EVENT, ESP_EVENT_ANY_ID, event_loop, NULL, NULL);
}
