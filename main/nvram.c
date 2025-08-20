/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "lwip/ip4_addr.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "params.h"
#include "sdkconfig.h"
#include "utils.h"
#include "xxhash.h"

#include <stdio.h>

#define STORAGE_NAMESPACE "nvram"

static const char* TAG = "nvram";

struct nvram {
  struct nvram_net net;
  struct nvram_token token;
  struct nvram_mqtt mqtt;
  struct nvram_misc misc;
  struct nvram_fold folds[CONFIG_MAX_NUM_OF_SENSOR_TYPES];
  struct nvram_beacon beacon;
  struct nvram_ids ids;
};

static struct nvram nvdata;

static bool nvram_read_ids(const struct nvram_ids* ids);
static bool nvram_read_net(struct nvram_net* net);
static bool nvram_read_token(struct nvram_token* token);
static bool nvram_read_mqtt(struct nvram_mqtt* token);
static bool nvram_read_misc(struct nvram_misc* misc);
static bool nvram_read_fold(struct nvram_fold* folds);
static bool nvram_read_beacon(struct nvram_beacon* beacon);

static bool nvram_read_ids(const struct nvram_ids* ids) {
  esp_read_mac((uint8_t*) ids->device_id, ESP_MAC_BASE);
  bin2hex(ids->device_id, DEVICE_ID_LEN, (char*) ids->device_id_str,
          DEVICE_ID_STR_LEN);
  ESP_LOGI(TAG, "Device ID : %s", ids->device_id_str);

  XXH128_hash_t hash = XXH3_128bits(ids->device_id, DEVICE_ID_LEN);
  memcpy((void*) ids->serial, &hash, sizeof(hash));
  bin2hex((uint8_t*) &hash, sizeof(hash), (char*) ids->serial_str,
          SERIAL_STR_LEN);
  ESP_LOGI(TAG, "Serial : %s", ids->serial_str);

  return true;
}

static bool nvram_read_net(struct nvram_net* net) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_net);
  err = nvs_get_blob(handle, "net", (void*) net, &size);
  nvs_close(handle);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Net\" parameters");
    return false;
  }

  ESP_LOGI(TAG, "Read network parameters");
  if (net->proto == STATIC) {
    ESP_LOGI(TAG, "IP : %s", ip4addr_ntoa((const ip4_addr_t*) &net->ipaddr));
    ESP_LOGI(TAG, "Netmask : %s", ip4addr_ntoa((const ip4_addr_t*) &net->netmask));
    ESP_LOGI(TAG, "Gateway : %s", ip4addr_ntoa((const ip4_addr_t*) &net->gateway));
  }
  else {
    ESP_LOGI(TAG, "DHCP");
  }
  
  return true;
}

static bool nvram_read_token(struct nvram_token* token) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_token);
  err = nvs_get_blob(handle, "token", (void*)token, &size);
  nvs_close(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Token\" parameters");
    return false;
  }
  
  ESP_LOGI(TAG, "Read token parameters");
  {
    ESP_LOGI(TAG, "Access token : %s", token->access);
    ESP_LOGI(TAG, "Refresh token : %s", token->refresh);
  }

  return true;
}

static bool nvram_read_mqtt(struct nvram_mqtt* mqtt) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_mqtt);
  err = nvs_get_blob(handle, "mqtt", (void*)mqtt, &size);
  nvs_close(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"MQTT\" parameters");
    return false;
  }

  ESP_LOGI(TAG, "Read MQTT parameters");
  if (mqtt->enable == true) {
    ESP_LOGI(TAG, "Host : %s", (char*)mqtt->host);
    ESP_LOGI(TAG, "Port : %d", mqtt->port);
    ESP_LOGI(TAG, "ID : %s", mqtt->id);
    ESP_LOGI(TAG, "PW : %s", mqtt->pw);
  }

  return true;
}

static bool nvram_read_misc(struct nvram_misc* misc) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_misc);
  err = nvs_get_blob(handle, "misc", (void*)misc, &size);
  nvs_close(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Misc\" parameters");
    return false;
  }

  ESP_LOGI(TAG, "Read misc parameters");
  {
    ESP_LOGI(TAG, "Prov : %s", misc->prov_done ? "True" : "False");
    ESP_LOGI(TAG, "TGS5141 : %d", misc->tgs5141);
  }

  return true;
}

static bool nvram_read_fold(struct nvram_fold* folds) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_fold);
  err = nvs_get_blob(handle, "folds", (void*)folds, &size);
  nvs_close(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Folds\" parameters");

    // Set Default fold..
    for (int i=0; i<CONFIG_MAX_NUM_OF_SENSOR_TYPES; i++) {
      folds->fold[i] = 10;
    }
    return false;
  }
  
  ESP_LOGI(TAG, "Read folds parameters");
  for (int i=0; i<CONFIG_MAX_NUM_OF_SENSOR_TYPES; i++) {
     ESP_LOGI(TAG, "%s : %d", sensor_type[i], folds[i].fold);
  }

  return true;
}

static bool nvram_read_beacon(struct nvram_beacon* beacon) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return false;
  }

  size_t size = sizeof(struct nvram_beacon);
  err = nvs_get_blob(handle, "beacon", (void*)beacon, &size);
  nvs_close(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Beacon\" parameters");
    beacon->enable = false;
  }

  ESP_LOGI(TAG, "Read beacon parameters");
  ESP_LOGI(TAG, "Beacon : %s", beacon->enable ? "True" : "False");
  
  return true;
}

bool nvram_get_prov_done(void) {
  return nvdata.misc.prov_done;
}



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

  nvram_read_ids(&nvdata.ids);
  nvram_read_beacon(&nvdata.beacon);

  if (nvram_read_net(&nvdata.net) == false ||
      nvram_read_token(&nvdata.token) == false ||
      nvram_read_mqtt(&nvdata.mqtt) == false ||
      nvram_read_misc(&nvdata.misc) == false) {
    ESP_LOGE(TAG, "Provisioning Required..");
  }
   
}
