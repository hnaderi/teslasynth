// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <array>
#include <cstdint>
#include <optional>

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
  constexpr static uint32_t current_version = 1;
  OutputConfig output{};
  InputConfig input;
  LEDConfig led;

  HardwareConfig();

  bool is_valid() const {
    constexpr auto valid_pin = [](gpio_num_t pin) {
      return pin >= gpio_num_t::GPIO_NUM_NC && pin < gpio_num_t::GPIO_NUM_MAX;
    };

    for (const auto &channel : output.channels)
      if (!valid_pin(channel.pin))
        return false;

    if (!valid_pin(input.maintenance) || !valid_pin(led.pin))
      return false;

    return led.logic == LogicType::active_high || led.logic == LogicType::active_low;
  }
};

} // namespace teslasynth::app::configuration::hardware
