// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "../configuration/hardware.hpp"
#include "esp_event_base.h"

namespace teslasynth::app::helpers::maintenance {
ESP_EVENT_DECLARE_BASE(MAINT_EVENT_BASE);
typedef enum {
  MAINT_EVT_HOLD_STARTED,
  MAINT_EVT_RELEASE_PHASE,
  MAINT_EVT_ABORTED,
  MAINT_EVT_TRIGGERED,
} maint_event_id_t;

bool check();
void reboot() __attribute__((__noreturn__));
void init(const configuration::hardware::InputConfig &config);
} // namespace teslasynth::app::helpers::maintenance
