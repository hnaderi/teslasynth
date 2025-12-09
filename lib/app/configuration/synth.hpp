#pragma once

#include "application.hpp"
#include "hardware.hpp"
#include "midi_synth.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::Config;

AppConfig read();
void persist(UIHandle &handle);

namespace hardware {
bool read(AppHardwareConfig& config);
bool persist(const hardware::AppHardwareConfig &config);
} // namespace hardware
} // namespace teslasynth::app::configuration
