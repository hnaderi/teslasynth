// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "wifi.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include <cstring>
#include <type_traits>

namespace teslasynth::app::configuration::wifi {

namespace {
constexpr char TAG[] = "wifi_config";
constexpr char KEY[] = "config";

esp_err_t init(nvs_handle_t &handle) {
  esp_err_t err = nvs_open("wificonf", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  return err;
}
} // namespace

bool WifiConfig::is_valid() const {
  const size_t ssid_len = strnlen(ssid, ssid_size);
  if (ssid_len == 0 || ssid_len >= ssid_size)
    return false;

  const size_t password_len = strnlen(password, password_size);
  if (password_len >= password_size)
    return false;
  if (password_len != 0 && password_len < min_password_len)
    return false;

  return channel >= min_channel && channel <= max_channel;
}

bool read(WifiConfig &config) {
  bool success = true;
  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));

  size_t len = sizeof(config);
  esp_err_t err = nvs_get_blob(handle, KEY, &config, &len);
  if (err != ESP_OK || len != sizeof(config) || config.version != WifiConfig::current_version ||
      !config.is_valid()) {
    success = false;
    config = WifiConfig();
  }

  nvs_close(handle);
  return success;
}

esp_err_t persist(const WifiConfig &config) {
  static_assert(std::is_trivially_copyable<WifiConfig>::value,
                "WifiConfig must be trivially copyable");

  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));

  auto err = nvs_set_blob(handle, KEY, &config, sizeof(config));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Couldn't persist configuration!");
  } else {
    nvs_commit(handle);
  }
  nvs_close(handle);
  return err;
}

} // namespace teslasynth::app::configuration::wifi
