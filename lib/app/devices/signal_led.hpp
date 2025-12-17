#pragma once
#include "../configuration/hardware.hpp"

namespace teslasynth::app::devices::signal_led {
void init(const configuration::hardware::LEDConfig &config);
}
