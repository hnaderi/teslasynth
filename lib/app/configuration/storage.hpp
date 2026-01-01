#pragma once

#include "esp_err.h"
#include "hardware.hpp"
#include "synth.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::ChannelConfig;

namespace synth {
bool read(AppConfig &config);
esp_err_t persist(const AppConfig &config);
} // namespace synth

namespace hardware {
bool read(HardwareConfig &config);
esp_err_t persist(const HardwareConfig &config);
} // namespace hardware
} // namespace teslasynth::app::configuration
