/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */
#include "nvram.h"

#include <endian.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif_ip_addr.h>
// #include "esp_system.h"

#include "esp_netif_types.h"
#include "params.h"
#include "utils.h"
#include "xxhash.h"

#include <freertos/FreeRTOS.h>
#include <lwip/ip4_addr.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <stdio.h>

#define STORAGE_NAMESPACE "nvram"
#define NVRAM_NET "net"
#define NVRAM_TOKEN "token"
#define NVRAM_MQTT "mqtt"
#define NVRAM_MISC "misc"
#define NVRAM_FOLDS "folds"
#define NVRAM_BEACON "beacon"

static const char* TAG = "nvram";

struct nvram {
  struct nvram_net net;
  struct nvram_token token;
  struct nvram_mqtt mqtt;
  struct nvram_misc misc;
  struct nvram_fold folds;
  struct nvram_beacon beacon;
  struct nvram_ids ids;
};

static struct nvram nvdata;

static esp_err_t nvram_read_ids(const struct nvram_ids* ids);
static esp_err_t nvram_read_net(struct nvram_net* net);
static esp_err_t nvram_read_token(struct nvram_token* token);
static esp_err_t nvram_read_mqtt(struct nvram_mqtt* token);
static esp_err_t nvram_read_misc(struct nvram_misc* misc);
static esp_err_t nvram_read_folds(struct nvram_fold* folds);
static esp_err_t nvram_read_beacon(struct nvram_beacon* beacon);
static esp_err_t nvram_update_net(const struct nvram_net* net);
static esp_err_t nvram_update_token(const struct nvram_token* token);
static esp_err_t nvram_update_mqtt(const struct nvram_mqtt* mqtt);
static esp_err_t nvram_update_misc(const struct nvram_misc* misc);
static esp_err_t nvram_update_folds(const struct nvram_fold* folds);
static esp_err_t nvram_update_beacon(const struct nvram_beacon* beacon);

static esp_err_t nvram_read_ids(const struct nvram_ids* ids) {
  esp_err_t err = esp_read_mac((uint8_t*) ids->device_id, ESP_MAC_BASE);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read mac base");
    return err;
  }
  err = esp_read_mac((uint8_t*) ids->eth_mac, ESP_MAC_ETH);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to rad eth mac address");
    return err;
  }

  bin2hex(ids->device_id, DEVICE_ID_LEN, (char*) ids->device_id_str,
          DEVICE_ID_STR_LEN);
  ESP_LOGI(TAG, "Device ID : %s", ids->device_id_str);

  XXH128_hash_t hash = XXH3_128bits(ids->device_id, DEVICE_ID_LEN);
  uint64_t big_high64 = bswap64(hash.high64);
  uint64_t big_low64 = bswap64(hash.low64);

  memcpy((void*) ids->serial, &big_high64, sizeof(uint64_t));
  memcpy((void*) (ids->serial + sizeof(uint64_t)), &big_low64,
         sizeof(uint64_t));
  snprintf((char*) ids->serial_str, SERIAL_STR_LEN, "%016llx%016llx",
           hash.high64, hash.low64);

  ESP_LOGI(TAG, "Serial : %s", ids->serial_str);

  return err;
}

static esp_err_t nvram_read_net(struct nvram_net* net) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_net);
  err = nvs_get_blob(handle, "net", (void*) net, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Net\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read network parameters");
  if (net->proto == STATIC) {
    ESP_LOGI(TAG, "IP : %s",
             ip4addr_ntoa((const ip4_addr_t*) &net->ip_info.ip));
    ESP_LOGI(TAG, "Netmask : %s",
             ip4addr_ntoa((const ip4_addr_t*) &net->ip_info.netmask));
    ESP_LOGI(TAG, "Gateway : %s",
             ip4addr_ntoa((const ip4_addr_t*) &net->ip_info.gw));
  } else {
    ESP_LOGI(TAG, "DHCP");
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_read_token(struct nvram_token* token) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_token);
  err = nvs_get_blob(handle, "token", (void*) token, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Token\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read token parameters");
  {
    ESP_LOGI(TAG, "Access token : %s", token->access);
    ESP_LOGI(TAG, "Refresh token : %s", token->refresh);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_read_mqtt(struct nvram_mqtt* mqtt) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_mqtt);
  err = nvs_get_blob(handle, "mqtt", (void*) mqtt, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"MQTT\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read MQTT parameters");
  if (mqtt->enable == true) {
    ESP_LOGI(TAG, "Host : %s", (char*) mqtt->host);
    ESP_LOGI(TAG, "Port : %d", mqtt->port);
    ESP_LOGI(TAG, "ID : %s", mqtt->id);
    ESP_LOGI(TAG, "PW : %s", mqtt->pw);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_read_misc(struct nvram_misc* misc) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_misc);
  err = nvs_get_blob(handle, "misc", (void*) misc, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Misc\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read misc parameters");
  {
    ESP_LOGI(TAG, "Prov : %s", misc->prov_done ? "True" : "False");
    ESP_LOGI(TAG, "TGS5141 : %d", misc->tgs5141);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_read_folds(struct nvram_fold* folds) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_fold);
  err = nvs_get_blob(handle, "folds", (void*) folds, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Folds\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read folds parameters");
  for (int i = 0; i < CONFIG_MAX_NUM_OF_SENSOR_TYPES; i++) {
    ESP_LOGI(TAG, "%s : %d", sensor_type[i], folds[i].fold);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_read_beacon(struct nvram_beacon* beacon) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  size_t size = sizeof(struct nvram_beacon);
  err = nvs_get_blob(handle, "beacon", (void*) beacon, &size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read \"Beacon\" parameters");
    nvs_close(handle);
    return err;
  }

  ESP_LOGI(TAG, "Read beacon parameters");
  ESP_LOGI(TAG, "Beacon : %s", beacon->enable ? "True" : "False");

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_net(const struct nvram_net* net) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, NVRAM_NET, (const void*) net,
                     sizeof(struct nvram_net));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"%s\" parameters", NVRAM_NET);
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit for %s", NVRAM_NET);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_token(const struct nvram_token* token) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, NVRAM_TOKEN, (const void*) token,
                     sizeof(struct nvram_token));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"%s\" parameters", NVRAM_TOKEN);
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit for %s", NVRAM_TOKEN);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_mqtt(const struct nvram_mqtt* mqtt) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, NVRAM_MQTT, (const void*) mqtt,
                     sizeof(struct nvram_mqtt));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"%s\" parameters", NVRAM_MQTT);
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit for %s", NVRAM_MQTT);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_misc(const struct nvram_misc* misc) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, NVRAM_MISC, (const void*) misc,
                     sizeof(struct nvram_misc));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"%s\" parameters", NVRAM_MISC);
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit for %s", NVRAM_MISC);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_folds(const struct nvram_fold* folds) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, "folds", (const void*) folds,
                     sizeof(struct nvram_fold));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"Folds\" parameters");
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit for %s", NVRAM_FOLDS);
  }

  nvs_close(handle);

  return err;
}

static esp_err_t nvram_update_beacon(const struct nvram_beacon* beacon) {
  nvs_handle_t handle;

  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open %s", STORAGE_NAMESPACE);
    return err;
  }

  err = nvs_set_blob(handle, "beacon", (void*) beacon,
                     sizeof(struct nvram_beacon));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update \"Beacon\" parameters");
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit..");
  }

  nvs_close(handle);

  return err;
}

esp_err_t nvram_set_net_params(const bool dhcp, const char* ip,
                               const char* netmask, const char* gateway,
                               const char* primary_dns,
                               const char* secondary_dns) {
  if (dhcp) {
    nvdata.net.proto = DHCP;
  } else {
    nvdata.net.proto = STATIC;
    nvdata.net.ip_info.ip.addr = ipaddr_addr(ip);
    nvdata.net.ip_info.netmask.addr = ipaddr_addr(netmask);
    nvdata.net.ip_info.gw.addr = ipaddr_addr(gateway);
    nvdata.net.primary_dns.addr = ipaddr_addr(primary_dns);
    if (secondary_dns != NULL) {
      nvdata.net.secondary_dns.addr = ipaddr_addr(secondary_dns);
    }
  }

  return nvram_update_net(&nvdata.net);
}

esp_err_t nvram_set_mqtt_params(const bool enable, const char* host,
                                const uint16_t port, const char* id,
                                const char* pw) {
  if (enable) {
    nvdata.mqtt.enable = enable;
    strcpy(nvdata.mqtt.host, host);
    nvdata.mqtt.port = port;
    if (id) {
      strcpy(nvdata.mqtt.id, id);
    }
    if (pw) {
      strcpy(nvdata.mqtt.pw, pw);
    }
  }

  return nvram_update_mqtt(&nvdata.mqtt);
}

esp_err_t nvram_set_token_params(const char* access, const char* refresh) {
  if (access == NULL || refresh == NULL) {
    return ESP_FAIL;
  }

  strcpy(nvdata.token.access, access);
  strcpy(nvdata.token.refresh, refresh);

  return nvram_update_token(&nvdata.token);
}

esp_err_t nvram_set_fold_params(const struct nvram_fold* folds) {
  if (folds == NULL) {
    return ESP_FAIL;
  }

  memcpy(&nvdata.folds, folds, sizeof(struct nvram_fold));

  return nvram_update_folds(&nvdata.folds);
}

esp_err_t nvram_set_fold_param(const int id, const uint8_t fold) {
  if (id < 0 || id >= CONFIG_MAX_NUM_OF_SENSOR_TYPES) {
    return ESP_FAIL;
  }

  nvdata.folds.fold[id] = fold;

  return nvram_update_folds(&nvdata.folds);
}

esp_err_t nvram_set_beacon_params(const bool enable) {
  nvdata.beacon.enable = enable;

  return nvram_update_beacon(&nvdata.beacon);
}

esp_err_t nvram_set_tgs5141_param(const uint32_t tgs5141) {
  nvdata.misc.tgs5141 = tgs5141;

  return nvram_update_misc(&nvdata.misc);
}

esp_err_t nvram_set_prov_done(void) {
  nvdata.misc.prov_done = true;

  return nvram_update_misc(&nvdata.misc);
}

esp_err_t nvram_get_net_params(bool* dhcp, esp_netif_ip_info_t* ip_info,
                               esp_ip4_addr_t* primary_dns,
                               esp_ip4_addr_t* secondary_dns) {
  if (dhcp == NULL) {
    return ESP_FAIL;
  }

  if (nvdata.net.proto == DHCP) {
    *dhcp = true;
  } else {
    if (ip_info == NULL || primary_dns == NULL) {
      return ESP_FAIL;
    }

    *dhcp = false;
    *ip_info = nvdata.net.ip_info;
    *primary_dns = nvdata.net.primary_dns;
    if (secondary_dns != NULL) {
      *secondary_dns = nvdata.net.secondary_dns;
    }
  }

  return ESP_OK;
}

void nvram_get_mqtt_params(bool* enable, char* host, uint16_t* port, char* id,
                           char* pw) {
  *enable = nvdata.mqtt.enable;
  if (nvdata.mqtt.enable) {
    strcpy(host, nvdata.mqtt.host);
    *port = nvdata.mqtt.port;
    if (strlen(id) != 0 && strlen(pw) != 0) {
      strcpy(id, nvdata.mqtt.id);
      strcpy(pw, nvdata.mqtt.pw);
    }
  }
}

void nvram_get_token_params(char* access, char* refresh) {
  strcpy(access, nvdata.token.access);
  strcpy(refresh, nvdata.token.refresh);
}

uint8_t nvram_get_fold_param(const int id) {
  if (id >= CONFIG_MAX_NUM_OF_SENSOR_TYPES) {
    return 0;
  } else {
    return nvdata.folds.fold[id];
  }
}

const struct nvram_fold* nvram_get_fold_params(void) {
  return &nvdata.folds;
}

bool nvram_get_beacon_params(void) {
  return nvdata.beacon.enable;
}

uint32_t nvram_get_tgs5141_param(void) {
  return nvdata.misc.tgs5141;
}

uint8_t* nvram_get_eth_mac(void) {
  return nvdata.ids.eth_mac;
}

uint8_t* nvram_get_serial(void) {
  return nvdata.ids.serial;
}

char* nvram_get_serial_str(void) {
  return nvdata.ids.serial_str;
}

uint8_t* nvram_get_device_id(void) {
  return nvdata.ids.device_id;
}

char* nvram_get_device_id_str(void) {
  return nvdata.ids.device_id_str;
}

bool nvram_get_prov_done(void) {
  return nvdata.misc.prov_done;
}

bool nvram_need_provisioning(void) {
  return nvdata.misc.prov_done ? false : true;
}

void nvram_init(void) {
  memset(&nvdata, 0, sizeof(nvdata));
  // Set default values..
  for (int i = 0; i < CONFIG_MAX_NUM_OF_SENSOR_TYPES; i++) {
    nvdata.folds.fold[i] = 10;
  }
  nvdata.beacon.enable = false;
  ///

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
  nvram_read_folds(&nvdata.folds);

  if (nvram_read_net(&nvdata.net) != ESP_OK ||
      nvram_read_token(&nvdata.token) != ESP_OK ||
      nvram_read_mqtt(&nvdata.mqtt) != ESP_OK ||
      nvram_read_misc(&nvdata.misc) != ESP_OK) {
    ESP_LOGE(TAG, "Provisioning Required..");
    nvdata.misc.prov_done = false;
  }
}
