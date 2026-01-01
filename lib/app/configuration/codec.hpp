#pragma once

#include "../helpers/json.hpp"
#include "application.hpp"

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

bool parse(helpers::JSONParser &parser, AppConfig &config);
helpers::JSONEncoder encode(const AppConfig &config);
} // namespace teslasynth::app::configuration::codec
