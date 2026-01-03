#pragma once

#include "../helpers/json.hpp"
#include "application.hpp"
#include "configuration/hardware.hpp"
#include "configuration/synth.hpp"
#include "result.hpp"

namespace teslasynth::app::configuration::codec {
namespace keys {
constexpr char max_on_time[] = "max-on-time";
constexpr char min_deadtime[] = "min-deadtime";
constexpr char max_duty[] = "max-duty";
constexpr char duty_window[] = "duty-window";
constexpr char tuning[] = "tuning";
constexpr char notes[] = "notes";
constexpr char channels[] = "channels";
constexpr char instrument[] = "instrument";
constexpr char routing[] = "routing";
constexpr char percussion[] = "percussion";
constexpr char mapping[] = "mapping";
}; // namespace keys

template <typename T>
using Decoder = teslasynth::helpers::Result<T, const char *>;

Decoder<AppConfig> parse_appconfig(helpers::JSONParser &parser);
helpers::JSONEncoder encode(const AppConfig &config);

Decoder<hardware::HardwareConfig> parse_hwconfig(helpers::JSONParser &parser);
helpers::JSONEncoder encode(const hardware::HardwareConfig &config);
} // namespace teslasynth::app::configuration::codec
