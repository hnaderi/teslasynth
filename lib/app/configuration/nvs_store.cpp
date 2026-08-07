// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "nvs_store.hpp"
#include "esp_log.h"
#include "nvs.h"

namespace teslasynth::app::configuration::nvs_store {

namespace {
constexpr char KEY[] = "config";

esp_err_t open(const char *ns, const char *tag, nvs_open_mode_t mode, nvs_handle_t &handle) {
  esp_err_t err = nvs_open(ns, mode, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(tag, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  return err;
}
} // namespace

std::optional<std::string> load(const char *ns, const char *tag) {
  nvs_handle_t handle;
  if (open(ns, tag, NVS_READONLY, handle) != ESP_OK)
    return {};

  size_t len = 0;
  esp_err_t err = nvs_get_str(handle, KEY, nullptr, &len);
  if (err != ESP_OK || len == 0) {
    nvs_close(handle);
    return {};
  }

  std::string json(len, '\0');
  err = nvs_get_str(handle, KEY, json.data(), &len);
  nvs_close(handle);
  if (err != ESP_OK)
    return {};

  json.resize(len > 0 ? len - 1 : 0);
  return json;
}

esp_err_t store(const char *ns, const char *tag, const char *json) {
  nvs_handle_t handle;
  esp_err_t err = open(ns, tag, NVS_READWRITE, handle);
  if (err != ESP_OK)
    return err;

  err = nvs_set_str(handle, KEY, json);
  if (err != ESP_OK) {
    ESP_LOGE(tag, "Couldn't persist configuration: %s", esp_err_to_name(err));
  } else {
    err = nvs_commit(handle);
  }
  nvs_close(handle);
  return err;
}

} // namespace teslasynth::app::configuration::nvs_store
