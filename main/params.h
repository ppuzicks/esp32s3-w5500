/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include "esp_netif.h"

#define HOST_NAME_LEN 64
#define ID_LEN 64
#define PW_LEN ID_LEN
#define TOKEN_LEN 128

#define DEVICE_ID_LEN 6
#define DEVICE_ID_STR_LEN (DEVICE_ID_LEN * 2 + 1)
#define SERIAL_LEN 16
#define SERIAL_STR_LEN (SERIAL_LEN * 2 + 1)

enum proto_t {
  DHCP = 0,
  STATIC,
};

struct nvram_net {
  enum proto_t proto;
  esp_netif_ip_info_t ip_info;
};

struct nvram_mqtt {
  bool enable;
  uint8_t host[HOST_NAME_LEN];
  uint16_t port;
  char id[ID_LEN];
  char pw[PW_LEN];
};

struct nvram_token {
  uint8_t access[TOKEN_LEN];
  uint8_t refresh[TOKEN_LEN];
};

struct nvram_fold {
  // uint8_t id;   // sensor_type
  uint8_t fold; // sensor_fold, minutes
};

struct nvram_misc {
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
