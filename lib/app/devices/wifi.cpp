// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "wifi.hpp"
#include "configuration/wifi.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "sdkconfig.h"
#include <algorithm>
#include <cstring>

namespace teslasynth::app::devices::wifi {
using configuration::wifi::WifiConfig;

namespace {
constexpr char TAG[] = "WIFI";

void initialise_mdns(void) {
  mdns_init();
  mdns_hostname_set(CONFIG_TESLASYNTH_DEVICE_NAME);
  mdns_instance_name_set("Teslasynth web server");

  mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};

  ESP_ERROR_CHECK(mdns_service_add("Teslasynth", "_http", "_tcp", 80, serviceTxtData,
                                   sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                        void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid,
             event->reason);
  }
}

void wifi_init_softap(const WifiConfig &config) {
  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
              .authmode = WIFI_AUTH_WPA3_PSK,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
              .authmode = WIFI_AUTH_WPA2_PSK,
#endif
              .max_connection = 1,
              .pmf_cfg =
                  {
                      .required = true,
                  },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
              .bss_max_idle_cfg =
                  {
                      .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                      .protected_keep_alive = 1,
                  },
#endif
              .sae_pwe_h2e = WPA3_SAE_PWE_HASH_TO_ELEMENT,
          },
  };

  const size_t ssid_len =
      std::min(strnlen(config.ssid, WifiConfig::ssid_size), sizeof(wifi_config.ap.ssid));
  memcpy(wifi_config.ap.ssid, config.ssid, ssid_len);
  wifi_config.ap.ssid_len = ssid_len;

  // wifi_config is zero initialised, so stopping one short keeps the password
  // NUL-terminated.
  const size_t password_len = std::min(strnlen(config.password, WifiConfig::password_size),
                                       sizeof(wifi_config.ap.password) - 1);
  memcpy(wifi_config.ap.password, config.password, password_len);
  wifi_config.ap.channel = config.channel;

  if (config.is_open()) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    // esp_wifi_set_config rejects PMF-required on an open AP.
    wifi_config.ap.pmf_cfg.required = false;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi started. SSID:%s channel:%d security:%s", config.ssid, config.channel,
           config.is_open() ? "open" : "protected");
}

} // namespace

void init(const WifiConfig &config) {
  if (config.is_valid()) {
    wifi_init_softap(config);
  } else {
    ESP_LOGE(TAG, "Stored WiFi configuration is invalid; falling back to factory defaults.");
    wifi_init_softap(WifiConfig());
  }

  initialise_mdns();
  netbiosns_init();
  netbiosns_set_name(CONFIG_TESLASYNTH_DEVICE_NAME);
}
} // namespace teslasynth::app::devices::wifi
