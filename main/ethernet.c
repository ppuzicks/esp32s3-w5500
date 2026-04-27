/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */
#include "ethernet.h"
#include "event_handler.h"
#include "nvram.h"

#include <esp_err.h>
#include <esp_eth_com.h>
#include <esp_eth_driver.h>
#include <esp_eth_netif_glue.h>
#include <esp_event.h>
#include <esp_event_base.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_defaults.h>
#include <esp_netif_types.h>
#include <ethernet_init.h>

static void net_ip_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

static const char* TAG = "ethernet";

static void net_ip_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data) {
  ESP_LOGI(TAG, "net_ip_event_handler");

  switch (event_id) {
  case IP_EVENT_ETH_GOT_IP:
    ESP_LOGI(TAG, "IP_EVENT_ETH_GOT_IP");
    {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
      const esp_netif_ip_info_t* ip_info = &event->ip_info;

      ESP_LOGI(TAG, "IP : " IPSTR, IP2STR(&ip_info->ip));
      ESP_LOGI(TAG, "Netmask : " IPSTR, IP2STR(&ip_info->netmask));
      ESP_LOGI(TAG, "Gateway : " IPSTR, IP2STR(&ip_info->gw));
    }
    event_send(EVT_RECV_IP, NULL, 0);
    break;

  case IP_EVENT_ETH_LOST_IP:
    ESP_LOGI(TAG, "IP_EVENT_ETH_LOST_IP");
    break;

  default:
    break;
  }
}

static void net_iface_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
  ESP_LOGI(TAG, "net_iface_event_handler");

  switch (event_id) {
  case ETHERNET_EVENT_CONNECTED:
    ESP_LOGI(TAG, "TODO: Config static ip when enable static ip flag");
    // net_iface_set_static_ip((esp_netif_t*)arg);
    break;
  default:
    break;
  }
}

static esp_err_t net_iface_set_static_ip(esp_netif_t* netif) {
  if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to stop dhcp client");
    return ESP_FAIL;
  }

  esp_netif_ip_info_t ipv4 = {
    0,
  };

  // ipv4.ip.addr = ipaddr_addr(nvram_get)

  return ESP_OK;
}

void net_iface_init(void) {
  ESP_LOGD(TAG, "net_iface_init");
  uint8_t eth_cnt = 0;
  esp_eth_handle_t* eth_handles;

  // Initialize TCP/IP network interface aka the esp-netif (should be called
  // only once in application)
  ESP_ERROR_CHECK(esp_netif_init());
  // Create default event loop that running in background
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize Ethernet driver
  ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_cnt));

  if (eth_cnt > 1) {
    ESP_LOGW(TAG, "Multiple Ethernet devices detected.. something wrong.. "
                  "first initialized is to be used!");
  }

  esp_err_t err = esp_eth_ioctl(eth_handles[0], ETH_CMD_S_MAC_ADDR,
                                (void*) nvram_get_eth_mac());
  // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and
  // you don't need to modify default esp-netif configuration parameters.
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t* eth_netif = esp_netif_new(&cfg);
  // Attach Ethernet driver to TCP/IP stack
  ESP_ERROR_CHECK(
      esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));

  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                             &net_ip_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &net_iface_event_handler, NULL));

  // Start Ethenet driver state machine
  ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));

  eth_dev_info_t info = ethernet_init_get_dev_info(eth_handles[0]);
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
