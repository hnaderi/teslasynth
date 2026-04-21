// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "esp_event_base.h"

ESP_EVENT_DECLARE_BASE(EVENT_SYNTHESIZER_BASE);
enum {
  SYNTHESIZER_PLAYING,
  SYNTHESIZER_STOPPED,
  SYNTHESIZER_CONFIG_UPDATED,
};
