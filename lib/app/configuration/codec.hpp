#pragma once

#include "../helpers/json.hpp"
#include "application.hpp"

namespace teslasynth::app::configuration::codec {
bool parse(helpers::JSONParser& parser, AppConfig &config);
} // namespace teslasynth::app::configuration::codec
