/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "http_client.h"

#include <esp_crt_bundle.h>
#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_log_buffer.h>

#define HTTP_RECV_BUFF_SIZE 4096

struct http_data {
  int recv_size;
  size_t buff_size;
  uint8_t* ptr_buff;
};

static const char* TAG = "http_client";

static esp_err_t http_resp_cb(esp_http_client_event_t* evt);

static esp_err_t http_resp_cb(esp_http_client_event_t* evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
  case HTTP_EVENT_ON_CONNECTED:
  case HTTP_EVENT_HEADER_SENT:
  case HTTP_EVENT_ON_HEADER:
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA %d", evt->data_len);
    break;

  default:
    break;
  }
  return ESP_OK;
}

esp_err_t https_get(const char* host_name, const char* path, const char* token,
                    uint8_t* resp_buff, size_t resp_buff_size,
                    uint32_t* recv_size) {
  int status_code = 0;

  if (host_name == NULL || path != NULL || resp_buff == NULL ||
      resp_buff_size == 0) {
    return ESP_FAIL;
  }

  struct http_data user_data = { .ptr_buff = resp_buff,
                                 .buff_size = resp_buff_size,
                                 .recv_size = 0 };

  esp_http_client_config_t config = {
    .host = host_name,
    .path = (path != NULL) ? path : "/",
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .user_data = (void*) &user_data,
    .buffer_size = HTTP_RECV_BUFF_SIZE,
    .event_handler = http_resp_cb,
    .crt_bundle_attach = esp_crt_bundle_attach,
  };

  ESP_LOGI(TAG, "HTTP Get from %s%s", host_name, (path != NULL) ? path : "/");

  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (token != NULL) {
    char bearer[256] = {
      0,
    };
    strcat(bearer, "Bearer ");
    strcat(bearer, token);
    esp_http_client_set_header(client, "Authorization", (const char*) bearer);
  }

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    status_code = esp_http_client_get_status_code(client);
    if (status_code == 404) {
      err = ESP_FAIL;
    }
    *recv_size = (uint32_t) esp_http_client_get_content_length(client);

    ESP_LOGI(TAG, "HTTP Get Status Code %d, Contents Length %d", status_code,
             *recv_size);
  } else {
    ESP_LOGE(TAG, "HTTP Get request failed: %s", esp_err_to_name(err));
  }

  return err;
}
