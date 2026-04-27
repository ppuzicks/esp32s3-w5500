/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

// #include "esp_netif.h"
#include <esp_netif_ip_addr.h>
#include <esp_netif_types.h>
#include <sdkconfig.h>

#define HOST_NAME_LEN 64
#define ID_LEN 64
#define PW_LEN ID_LEN
#define TOKEN_LEN 128

#define MAC_ADDR_LEN 6
#define DEVICE_ID_LEN 6
#define DEVICE_ID_STR_LEN (DEVICE_ID_LEN * 2 + 1)
#define SERIAL_LEN 16
#define SERIAL_STR_LEN (SERIAL_LEN * 2 + 1)

#define UPDATE_SERVER_DOMAIN "cym-rnd.duckdns.org"
#define PROVISIONER_PATH "/provisioning"
#define OTA_PATH "/ota"
#define OTA_VERSION_PATH "/ota/version"

#define SNTP_DEFAULT_SERVER "pool.ntp.org"

enum sensor_type_id {
  TYPE_T = 0,  // 온도 (대기온도 등 통칭)
  TYPE_H,      // 습도 (일반적인 상대습도)
  TYPE_CO2,    // 이산화탄소 농도
  TYPE_VOC,    // VOC (유기화합물) - SPG41
  TYPE_NOX,    // NOx (질소산화물)
  TYPE_PM_1_0, // 미세먼지(PM1.0) 농도
  TYPE_PM_2_5, // 미세먼지(PM2.5) 농도
  TYPE_PM_10,  // 미세먼지(PM10) 농도
  TYPE_CO,     // 일산화탄소 농도
  TYPE_MAX = CONFIG_MAX_NUM_OF_SENSOR_TYPES
};

enum proto_t {
  DHCP = 0,
  STATIC,
};

struct nvram_net {
  enum proto_t proto;

  esp_netif_ip_info_t ip_info;
  esp_ip4_addr_t primary_dns;
  esp_ip4_addr_t secondary_dns;
};

struct nvram_mqtt {
  bool enable;
  char host[HOST_NAME_LEN];
  uint16_t port;
  char id[ID_LEN];
  char pw[PW_LEN];
};

struct nvram_token {
  char access[TOKEN_LEN];
  char refresh[TOKEN_LEN];
};

struct nvram_fold {
  // uint8_t id;   // sensor_type
  uint8_t fold[CONFIG_MAX_NUM_OF_SENSOR_TYPES]; // sensor_fold, minutes
};

struct nvram_misc {
  bool prov_done;
  uint32_t tgs5141; // tgs5141 sensitivity
};

struct nvram_beacon {
  bool enable;
};

struct nvram_version {
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
  uint8_t tweak;
};

struct nvram_ids {
  uint8_t eth_mac[MAC_ADDR_LEN];
  uint8_t device_id[DEVICE_ID_LEN];
  char device_id_str[DEVICE_ID_STR_LEN];
  uint8_t serial[SERIAL_LEN];
  char serial_str[SERIAL_STR_LEN];
};

static const char* sensor_type[CONFIG_MAX_NUM_OF_SENSOR_TYPES] = {
  "AT", "RH", "CD", "VOC", "NOX", "PM_1_0", "PM_2_5", "PM_10", "CM"
};
