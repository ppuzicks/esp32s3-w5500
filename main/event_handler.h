/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include "esp_err.h"
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(APP_EVENT);

typedef enum {
  EVT_TEST,
  EVT_RECV_IP,
  EVT_PROVISIONING,
} EventID;

esp_err_t event_handler_init(void);
esp_err_t event_send(const EventID event_id, const void* event_msg,
                     const size_t event_msg_size);
