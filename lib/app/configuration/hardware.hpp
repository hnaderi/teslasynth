// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <array>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace teslasynth::app::configuration::hardware {
struct OutputChannelConfig {
  gpio_num_t pin = gpio_num_t::GPIO_NUM_NC;
};

enum class LogicType : uint8_t {
  active_high = 1,
  active_low = 0,
};

struct LEDConfig {
  gpio_num_t pin;
  LogicType logic;
  uint8_t reserved[3] = {};

  LEDConfig();
};

struct OutputConfig {
  constexpr static uint8_t size = CONFIG_TESLASYNTH_OUTPUT_COUNT;
  std::array<OutputChannelConfig, size> channels;
};

struct InputConfig {
  gpio_num_t maintenance = gpio_num_t::GPIO_NUM_0;
};

struct HardwareConfig {
  constexpr static uint32_t current_version = 0;
  uint32_t version = current_version;
  OutputConfig output{};
  InputConfig input;
  LEDConfig led;

  HardwareConfig();

  bool is_valid() const;
};

static_assert(std::has_unique_object_representations_v<HardwareConfig>,
              "HardwareConfig is persisted as a blob and must have no padding");

} // namespace teslasynth::app::configuration::hardware
