// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "hardware.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <vector>

namespace teslasynth::app::configuration::hardware {

namespace {
constexpr char TAG[] = "hw_config";
constexpr char KEY[] = "config";

esp_err_t init(nvs_handle_t &handle) {
  esp_err_t err = nvs_open("hwconf", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  return err;
}
} // namespace

namespace {
bool valid_pin(gpio_num_t pin) {
  return pin >= gpio_num_t::GPIO_NUM_NC && pin < gpio_num_t::GPIO_NUM_MAX;
}
} // namespace

bool HardwareConfig::is_valid() const {
  for (const auto &channel : output.channels)
    if (!valid_pin(channel.pin))
      return false;

  if (!valid_pin(input.maintenance) || !valid_pin(led.pin))
    return false;

  return led.logic == LogicType::active_high || led.logic == LogicType::active_low;
}

bool read(HardwareConfig &config) {
  bool success = true;
  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));
  size_t len = sizeof(config);
  esp_err_t err = nvs_get_blob(handle, KEY, &config, &len);
  if (err != ESP_OK || len != sizeof(config) || config.version != HardwareConfig::current_version ||
      !config.is_valid()) {
    ESP_LOGW(TAG, "Outdated or corrupted configuration; resetting to defaults");
    success = false;
    config = HardwareConfig();
  }
  nvs_close(handle);
  return success;
}

esp_err_t persist(const HardwareConfig &config) {
  static_assert(std::is_trivially_copyable<HardwareConfig>::value,
                "HardwareConfig must be trivially copyable");

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

} // namespace teslasynth::app::configuration::hardware
