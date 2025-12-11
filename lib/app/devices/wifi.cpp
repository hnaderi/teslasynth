#include "wifi.hpp"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "sdkconfig.h"

namespace teslasynth::app::devices::wifi {
namespace {
constexpr char const *TAG = "WIFI";

void initialise_mdns(void) {
  mdns_init();
  mdns_hostname_set(CONFIG_TESLASYNTH_DEVICE_NAME);
  mdns_instance_name_set("Teslasynth web server");

  mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};

  ESP_ERROR_CHECK(
      mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                       sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
             MAC2STR(event->mac), event->aid, event->reason);
  }
}

void wifi_init_softap(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = CONFIG_TESLASYNTH_DEVICE_NAME,
              .password = CONFIG_TESLASYNTH_WIFI_PASSWORD,
              .channel = CONFIG_TESLASYNTH_WIFI_CHANNEL,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
              .authmode = WIFI_AUTH_WPA3_PSK,
              .max_connection = 1,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
              .authmode = WIFI_AUTH_WPA2_PSK,
#endif
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
              .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
          },
  };
  if (strlen(CONFIG_TESLASYNTH_WIFI_PASSWORD) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
           CONFIG_TESLASYNTH_DEVICE_NAME, CONFIG_TESLASYNTH_WIFI_PASSWORD,
           CONFIG_TESLASYNTH_WIFI_CHANNEL);
}

} // namespace

void init() {
  wifi_init_softap();
  initialise_mdns();
  netbiosns_init();
  netbiosns_set_name(CONFIG_TESLASYNTH_DEVICE_NAME);
}
} // namespace teslasynth::app::devices::wifi
