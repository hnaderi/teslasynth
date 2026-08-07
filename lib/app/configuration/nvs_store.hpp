// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "esp_err.h"
#include <optional>
#include <string>

namespace teslasynth::app::configuration::nvs_store {

std::optional<std::string> load(const char *ns, const char *tag);
esp_err_t store(const char *ns, const char *tag, const char *json);

} // namespace teslasynth::app::configuration::nvs_store
