/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include "esp_err.h"
#include "esp_netif_types.h"
#include "params.h"

#include <stdbool.h>

esp_err_t nvram_set_net_params(const bool dhcp, const char* ip,
                               const char* netmask, const char* gateway,
                               const char* primary_dns,
                               const char* secondary_dns);
esp_err_t nvram_set_mqtt_params(const bool enable, const char* host,
                                const uint16_t port, const char* id,
                                const char* pw);
esp_err_t nvram_set_token_params(const char* access, const char* refresh);
esp_err_t nvram_set_fold_params(const struct nvram_fold* folds);
esp_err_t nvram_set_fold_param(const int id, const uint8_t fold);
esp_err_t nvram_set_beacon_params(const bool enable);
esp_err_t nvram_set_tgs5141_param(const uint32_t tgs5141);
esp_err_t nvram_set_prov_done(void);

esp_err_t nvram_get_net_params(bool* dhcp, esp_netif_ip_info_t* ip_info,
                               esp_ip4_addr_t* primary_dns,
                               esp_ip4_addr_t* secondary_dns);

void nvram_get_mqtt_params(bool* enable, char* host, uint16_t* port, char* id,
                           char* pw);

void nvram_get_token_params(char* access, char* refresh);
uint8_t nvram_get_fold_param(const int id);
const struct nvram_fold* nvram_get_fold_params(void);
bool nvram_get_beacon_params(void);
uint32_t nvram_get_tgs5141_param(void);
uint8_t* nvram_get_eth_mac(void);
uint8_t* nvram_get_serial(void);
char* nvram_get_serial_str(void);
uint8_t* nvram_get_device_id(void);
char* nvram_get_device_id_str(void);

bool nvram_get_prov_done(void);
bool nvram_need_provisioning(void);

void nvram_init(void);
