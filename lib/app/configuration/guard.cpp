// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "storage.hpp"

namespace teslasynth::app::configuration {

namespace {
constexpr char TAG[] = "config_guard";

SemaphoreHandle_t mutex() {
  static SemaphoreHandle_t handle = xSemaphoreCreateRecursiveMutex();
  return handle;
}
} // namespace

Guard::Guard() {
  auto handle = mutex();
  if (handle == nullptr) {
    ESP_LOGE(TAG, "Configuration mutex unavailable!");
    return;
  }
  xSemaphoreTakeRecursive(handle, portMAX_DELAY);
}

Guard::~Guard() {
  auto handle = mutex();
  if (handle != nullptr)
    xSemaphoreGiveRecursive(handle);
}

} // namespace teslasynth::app::configuration
