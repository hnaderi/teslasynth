// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "json.hpp"
#include "hardware.hpp"
#include "synth.hpp"
#include "wifi.hpp"
#include "result.hpp"

namespace teslasynth::app::configuration::codec {
namespace keys {
constexpr char max_on_time[] = "max-on-time";
constexpr char min_deadtime[] = "min-deadtime";
constexpr char max_duty[] = "max-duty";
constexpr char duty_window[] = "duty-window";
constexpr char pulse_resolution[] = "pulse-resolution";
constexpr char tuning[] = "tuning";
constexpr char notes[] = "notes";
constexpr char channels[] = "channels";
constexpr char instrument[] = "instrument";
constexpr char routing[] = "routing";
constexpr char percussion[] = "percussion";
constexpr char mapping[] = "mapping";
constexpr char version[] = "version";
}; // namespace keys

template <typename T> using Decoder = teslasynth::helpers::Result<T, const char *>;

Decoder<AppConfig> parse_appconfig(helpers::JSONParser &parser);
helpers::JSONEncoder encode(const AppConfig &config);

Decoder<hardware::HardwareConfig> parse_hwconfig(helpers::JSONParser &parser);
helpers::JSONEncoder encode(const hardware::HardwareConfig &config);

Decoder<wifi::WifiConfig> parse_wificonfig(helpers::JSONParser &parser,
                                           const wifi::WifiConfig &current);
helpers::JSONEncoder encode(const wifi::WifiConfig &config);
helpers::JSONEncoder encode_stored(const wifi::WifiConfig &config);

bool has_version(const helpers::JSONParser &parser, uint32_t expected);
} // namespace teslasynth::app::configuration::codec
