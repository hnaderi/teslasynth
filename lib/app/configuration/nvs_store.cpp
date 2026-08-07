// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "esp_log.h"
#include "nvs.h"
#include "nvs_store.hpp"
#include "persistence.hpp"
#include <cstring>

namespace teslasynth::app::configuration::nvs_store {

namespace {
constexpr char TAG[] = "config_store";
constexpr char KEY[] = "config";

// NVS namespaces are capped at 15 characters and predate the scope names, so
// they are mapped rather than used directly.
const char *nvs_namespace(const char *scope) {
  if (strcmp(scope, "hardware") == 0)
    return "hwconf";
  if (strcmp(scope, "wifi") == 0)
    return "wificonf";
  return "synth";
}

esp_err_t open(const char *scope, nvs_open_mode_t mode, nvs_handle_t &handle) {
  esp_err_t err = nvs_open(nvs_namespace(scope), mode, &handle);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle for %s!", esp_err_to_name(err), scope);
  }
  return err;
}
} // namespace

namespace {
std::optional<std::string> load(const char *scope) {
  nvs_handle_t handle;
  if (open(scope, NVS_READONLY, handle) != ESP_OK)
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

bool save(const char *scope, const char *json) {
  nvs_handle_t handle;
  if (open(scope, NVS_READWRITE, handle) != ESP_OK)
    return false;

  esp_err_t err = nvs_set_str(handle, KEY, json);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Couldn't persist %s configuration: %s", scope, esp_err_to_name(err));
  } else {
    err = nvs_commit(handle);
  }
  nvs_close(handle);
  return err == ESP_OK;
}
} // namespace

void install() {
  store::install(&load, &save);
}

} // namespace teslasynth::app::configuration::nvs_store
