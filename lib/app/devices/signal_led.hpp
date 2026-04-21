// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include "../configuration/hardware.hpp"

namespace teslasynth::app::devices::signal_led {
void init(const configuration::hardware::LEDConfig &config);
}
