// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "hardware.hpp"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include "wifi.hpp"
#include <cstring>

#define GPIO_CONFIG(sym) static_cast<gpio_num_t>(CONFIG_TESLASYNTH_##sym)
namespace teslasynth::app::configuration::hardware {

constexpr std::array<OutputChannelConfig, OutputConfig::size> default_channels{{
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 1
    {GPIO_CONFIG(OUTPUT_GPIO_PIN1)},
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 2
    {GPIO_CONFIG(OUTPUT_GPIO_PIN2)},
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 3
    {GPIO_CONFIG(OUTPUT_GPIO_PIN3)},
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 4
    {GPIO_CONFIG(OUTPUT_GPIO_PIN4)},
#endif
}};

constexpr auto default_led_logic =
#ifdef CONFIG_TESLASYNTH_OUTPUT_GPIO_LED_ACTIVE_LOW
    LogicType::active_low;
#else
    LogicType::active_high;
#endif

LEDConfig::LEDConfig() : pin(GPIO_CONFIG(OUTPUT_GPIO_LED)), logic(default_led_logic) {}

HardwareConfig::HardwareConfig() : output(default_channels) {}

} // namespace teslasynth::app::configuration::hardware

namespace teslasynth::app::configuration::wifi {

static_assert(sizeof(CONFIG_TESLASYNTH_DEVICE_NAME) <= WifiConfig::ssid_size,
              "Default SSID does not fit in WifiConfig::ssid");
static_assert(sizeof(CONFIG_TESLASYNTH_WIFI_PASSWORD) <= WifiConfig::password_size,
              "Default password does not fit in WifiConfig::password");

WifiConfig::WifiConfig() : channel(CONFIG_TESLASYNTH_WIFI_CHANNEL) {
  // strncpy zero-fills the unused tail; persist() writes the whole object.
  strncpy(ssid, CONFIG_TESLASYNTH_DEVICE_NAME, sizeof(ssid));
  strncpy(password, CONFIG_TESLASYNTH_WIFI_PASSWORD, sizeof(password));
}

} // namespace teslasynth::app::configuration::wifi
