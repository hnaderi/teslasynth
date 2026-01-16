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

enum class LogicType : bool {
  active_high = true,
  active_low = false,
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
  constexpr static uint32_t current_version = 0;
  uint32_t version = current_version;
  OutputConfig output{};
  InputConfig input;
  LEDConfig led;

  HardwareConfig();
};

} // namespace teslasynth::app::configuration::hardware
