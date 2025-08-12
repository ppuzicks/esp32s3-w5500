/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "ethernet.h"

#include "esp_err.h"
#include "esp_eth_com.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "esp_netif_types.h"
#include "ethernet_init.h"

static const char *TAG = "ethernet";

static void net_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "net_ip_event_handler");

  switch (event_id) {
  default:
    break;
  }
}

static void net_iface_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "net_iface_event_handler");

  switch (event_id) {
  default:
    break;
  }
}

void net_iface_init(void) {
  ESP_LOGD(TAG, "net_iface_init");
  uint8_t eth_cnt = 0;
  esp_eth_handle_t *eth_handles;

  // Initialize TCP/IP network interface aka the esp-netif (should be called
  // only once in application)
  ESP_ERROR_CHECK(esp_netif_init());
  // Create default event loop that running in background
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize Ethernet driver
  ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_cnt));

  if (eth_cnt == 1) {
    // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and
    // you don't need to modify default esp-netif configuration parameters.
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(
        esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));
  }

  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                             &net_ip_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &net_iface_event_handler, NULL));

  // Start Ethenet driver state machine
  for (int i = 0; i < eth_cnt; i++) {
    ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
  }

  for (int i = 0; i < eth_cnt; i++) {
    eth_dev_info_t info = ethernet_init_get_dev_info(eth_handles[i]);
    if (info.type == ETH_DEV_TYPE_INTERNAL_ETH) {
      ESP_LOGI(TAG, "Device Name: %s", info.name);
      ESP_LOGI(TAG, "Device type: ETH_DEV_TYPE_INTERNAL_ETH(%d)", info.type);
      ESP_LOGI(TAG, "Pins: mdc: %d, mdio: %d", info.pin.eth_internal_mdc,
               info.pin.eth_internal_mdio);
    } else if (info.type == ETH_DEV_TYPE_SPI) {
      ESP_LOGI(TAG, "Device Name: %s", info.name);
      ESP_LOGI(TAG, "Device type: ETH_DEV_TYPE_SPI(%d)", info.type);
      ESP_LOGI(TAG, "Pins: cs: %d, intr: %d", info.pin.eth_spi_cs,
               info.pin.eth_spi_int);
    }
  }
}
