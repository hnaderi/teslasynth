// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "esp_err.h"
#include "hardware.hpp"
#include "synth.hpp"
#include "wifi.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::ChannelConfig;

class Guard {
public:
  Guard();
  ~Guard();

  Guard(const Guard &) = delete;
  Guard &operator=(const Guard &) = delete;
};

// On failure `config` is left holding factory defaults and `reason` explains
// what was wrong, for the maintenance-mode report.
struct ReadOutcome {
  bool ok = true;
  const char *reason = nullptr;

  explicit operator bool() const { return ok; }
};

namespace synth {
ReadOutcome read(AppConfig &config);
esp_err_t persist(const AppConfig &config);
} // namespace synth

namespace hardware {
ReadOutcome read(HardwareConfig &config);
esp_err_t persist(const HardwareConfig &config);
} // namespace hardware

namespace wifi {
ReadOutcome read(WifiConfig &config);
esp_err_t persist(const WifiConfig &config);
} // namespace wifi
} // namespace teslasynth::app::configuration
