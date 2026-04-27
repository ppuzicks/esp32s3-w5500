/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */
#include "nvram.h"
#include "params.h"

#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_log_buffer.h>
#include <freertos/idf_additions.h>
#include <freertos/projdefs.h>
#include <mbedtls/aes.h>
#include <mbedtls/cipher.h>
#include <mbedtls/esp_config.h>
#include <mbedtls/md.h>
#include <portmacro.h>

// #include <complex.h>

#define IV_HASH_LEN 16
#define KEY_HASH_LEN 32

#define MAX_URL_LEN 64
#define MAX_PROV_DATA_LEN 2048

#define STACK_SIZE 2048
#define PRIORITY 1

#define QUEUE_LEN 2
#define ITEM_SIZE sizeof(uint32_t)

#define PERIODIC_CONFIG_MINUTES 60    // 1hr
#define PERIODIC_FIRMWARE_MINUTES 360 // 6hr
#define ONE_MINUTES_TICKS pdMS_TO_TICKS(60000)

typedef enum {
  RUN_UPDATE_CONFIG = 1,
  RUN_FIRMWARE_UPDATE,
  PERIODIC_UPDATE_CONFIG,
  PERIODIC_FIRMWARE_UPDATE,
} prov_cmd_t;

struct provision {
  QueueHandle_t queue;
  TaskHandle_t worker;
  TimerHandle_t timer;

  uint32_t n_minutes;
  uint32_t n_tries;
  uint32_t n_periodic_config;
  uint32_t n_periodic_firmware;
};

static esp_err_t generate_hash(uint8_t* iv, uint8_t* key,
                               const uint8_t* device_id, size_t device_id_len,
                               const uint8_t* serial, size_t serial_len);
static esp_err_t decrypt_data(const uint8_t* enc_data,
                              const size_t enc_data_len, uint8_t* dec_data,
                              size_t* dec_data_len);
static void provision_timer_handler(TimerHandle_t xTimer);
static void provision_worker(void* arg);
static esp_err_t provision_config_update(void);
static esp_err_t firmware_update(void);

static const char* TAG = "prov";
static struct provision ctx;

static esp_err_t generate_hash(uint8_t* iv, uint8_t* key,
                               const uint8_t* device_id, size_t device_id_len,
                               const uint8_t* serial, size_t serial_len) {
  if (iv == NULL || key == NULL) {
    return ESP_FAIL;
  }

  const mbedtls_md_info_t* md_info;
  mbedtls_md_context_t md_ctx;
  unsigned char digest[MBEDTLS_MD_MAX_SIZE] = {
    0,
  };
  esp_err_t ret;

  mbedtls_md_init(&md_ctx);
  md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info == NULL) {
    ESP_LOGE(TAG, "Message Digest SHA256 not found");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_setup returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  // Make IV hash
  if (mbedtls_md_starts(&md_ctx) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_starts returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_md_update(&md_ctx, device_id, device_id_len) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_update returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_md_finish(&md_ctx, digest) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_finish returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  memcpy(iv, digest, IV_HASH_LEN);

  memset(digest, 0, MBEDTLS_MD_MAX_SIZE);
  // Make Key hash
  if (mbedtls_md_starts(&md_ctx) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_starts returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_md_update(&md_ctx, serial, serial_len) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_update returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_md_finish(&md_ctx, digest) != 0) {
    ESP_LOGE(TAG, "mbedtls_md_finish returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  memcpy(key, digest, KEY_HASH_LEN);

exit:
  mbedtls_md_free(&md_ctx);

  return ret;
}

static esp_err_t decrypt_data(const uint8_t* enc_data,
                              const size_t enc_data_len, uint8_t* dec_data,
                              size_t* dec_data_len) {
  uint8_t iv[IV_HASH_LEN] = {
    0,
  };
  uint8_t key[KEY_HASH_LEN] = {
    0,
  };

  const mbedtls_cipher_info_t* cipher_info;
  mbedtls_cipher_context_t cipher_ctx;
  mbedtls_cipher_mode_t cipher_mode;
  esp_err_t ret;

  if (generate_hash(iv, key, nvram_get_device_id(), DEVICE_ID_LEN,
                    nvram_get_serial(), SERIAL_LEN) != ESP_OK) {
    return ESP_FAIL;
  }

  mbedtls_cipher_init(&cipher_ctx);
  cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);
  if (cipher_info == NULL) {
    ESP_LOGE(TAG, "Cipher MBEDTLS_CIPHER_AES_256_CBC not found");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_cipher_setup(&cipher_ctx, cipher_info) != 0) {
    ESP_LOGE(TAG, "mbedtls_cipher_setup returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  // Decrypt
  if (mbedtls_cipher_setkey(&cipher_ctx, key,
                            mbedtls_cipher_info_get_key_bitlen(cipher_info),
                            MBEDTLS_DECRYPT) != 0) {
    ESP_LOGE(TAG, "mbedtls_cipher_setkey returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_cipher_set_iv(&cipher_ctx, iv, IV_HASH_LEN) != 0) {
    ESP_LOGE(TAG, "mbedtls_cipher_set_iv returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_cipher_reset(&cipher_ctx) != 0) {
    ESP_LOGE(TAG, "mbedtls_cipher_reset returned error");
    ret = ESP_FAIL;
    goto exit;
  }

  int block_size = mbedtls_cipher_get_block_size(&cipher_ctx);

  if ((enc_data_len % block_size) != 0) {
    // encrypted data는 항상 block size의 배수로 되어 있어야 함.
    ESP_LOGE(TAG, "encrypted data size is wrong");
    ret = ESP_FAIL;
    goto exit;
  }

  if (mbedtls_cipher_update(&cipher_ctx, enc_data, enc_data_len, dec_data,
                            dec_data_len) != 0) {
    ESP_LOGE(TAG, "mbedtls_cipher_update returned error");
    ret = ESP_FAIL;
  }

exit:
  return ret;
}

static esp_err_t provision_config_update(void) {
  uint8_t* enc_data =
      (uint8_t*) heap_caps_malloc(MAX_PROV_DATA_LEN, MALLOC_CAP_SPIRAM);
  uint8_t* dec_data =
      (uint8_t*) heap_caps_malloc(MAX_PROV_DATA_LEN, MALLOC_CAP_SPIRAM);
  char* prov_url = (char*) heap_caps_malloc(MAX_URL_LEN, MALLOC_CAP_SPIRAM);

  uint32_t dec_len;

  memset(enc_data, 0, MAX_PROV_DATA_LEN);
  memset(dec_data, 0, MAX_PROV_DATA_LEN);
  memset(prov_url, 0, MAX_URL_LEN);

  strcat(prov_url, PROVISIONER_PATH);
  strcat(prov_url, "/");
  strcat(prov_url, nvram_get_device_id_str());

  ESP_LOGI(TAG, "prov_url %s", prov_url);

  return ESP_OK;
}

static void provision_timer_handler(TimerHandle_t xTimer) {
  // ESP_LOGI(TAG, "%s", __FUNCTION__);

  ctx.n_periodic_config++;
  ctx.n_periodic_firmware++;

  if (ctx.n_periodic_config >= PERIODIC_CONFIG_MINUTES) {
    prov_cmd_t cmd = PERIODIC_UPDATE_CONFIG;

    if (xQueueSend(ctx.queue, (void*) &cmd, 0) != pdPASS) {
      ESP_LOGE(TAG, "Failed to send PERIODIC_UPDATE_CONFIG");
    }

    ctx.n_periodic_config = 0;
  }

  if (ctx.n_periodic_firmware >= PERIODIC_FIRMWARE_MINUTES) {
    prov_cmd_t cmd = PERIODIC_FIRMWARE_UPDATE;

    if (xQueueSend(ctx.queue, (void*) &cmd, 0) != pdPASS) {
      ESP_LOGE(TAG, "FAiled to send PERIODIC_FIRMWARE_UPDATE");
    }

    ctx.n_periodic_firmware = 0;
  }
}

static void provision_worker(void* arg) {
  struct provision* p_ctx = (struct provision*) arg;

  while (true) {
    prov_cmd_t cmd;
    if (xQueueReceive(p_ctx->queue, (void*) &cmd, portMAX_DELAY) == pdPASS) {

      // Provision Here
      switch (cmd) {
      case RUN_UPDATE_CONFIG:
        ESP_LOGI(TAG, "RUN_UPDATE_CONFIG");
        provision_config_update();
        break;
      case RUN_FIRMWARE_UPDATE:
        break;
      case PERIODIC_UPDATE_CONFIG:
        break;
      case PERIODIC_FIRMWARE_UPDATE:
        break;
      default:
        break;
      }
    }
  }
}

void provision_init(void) {
  ctx.n_periodic_config = ctx.n_periodic_firmware = 0;
  ctx.queue = xQueueCreate(QUEUE_LEN, ITEM_SIZE);
  if (ctx.queue == NULL) {
    ESP_LOGE(TAG, "Failed to create provisioning queue");
    return;
  }
  xTaskCreate(provision_worker, "provision_worker", STACK_SIZE, (void*) &ctx,
              PRIORITY, &ctx.worker);
  if (ctx.worker == NULL) {
    ESP_LOGE(TAG, "Failed to create task for provision worker");
    return;
  }

  ESP_LOGI(TAG, "");
}

esp_err_t provisioning_start(void) {
  ctx.timer = xTimerCreate("provison_timer", ONE_MINUTES_TICKS, pdTRUE, NULL,
                           provision_timer_handler);
  if (ctx.timer == NULL) {
    ESP_LOGE(TAG, "Failed to create provisioning timer");
    return ESP_FAIL;
  }

  return (xTimerStart(ctx.timer, 0) == pdFAIL) ? ESP_FAIL : ESP_OK;
}

esp_err_t update_config(void) {
  prov_cmd_t cmd = RUN_UPDATE_CONFIG;

  if (xQueueSend(ctx.queue, (void*) &cmd, 0) != pdPASS) {
    ESP_LOGE(TAG, "Failed to send RUN_UPDATE_CONFIG");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t update_firmware(void) {
  prov_cmd_t cmd = RUN_FIRMWARE_UPDATE;

  if (xQueueSend(ctx.queue, (void*) &cmd, 0) != pdPASS) {
    ESP_LOGE(TAG, "Failed to send RUN_FIRMWARE_UPDATE");
    return ESP_FAIL;
  }

  return ESP_OK;
}
