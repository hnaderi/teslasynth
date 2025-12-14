#pragma once

#include "../helpers/json.hpp"
#include "application.hpp"

namespace teslasynth::app::configuration::codec {
namespace keys {
constexpr const char *max_on_time = "max-on-time";
constexpr const char *min_deadtime = "min-deadtime";
constexpr const char *max_duty = "max-duty";
constexpr const char *duty_window = "duty-window";
constexpr const char *tuning = "tuning";
constexpr const char *notes = "notes";
constexpr const char *channels = "channels";
constexpr const char *instrument = "instrument";
}; // namespace keys

bool parse(helpers::JSONParser &parser, AppConfig &config);
helpers::JSONEncoder encode(const AppConfig &config);
} // namespace teslasynth::app::configuration::codec
