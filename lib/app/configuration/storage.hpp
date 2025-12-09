#pragma once

#include "application.hpp"
#include "hardware.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::Config;

namespace synth {
AppConfig read();
void persist(UIHandle &handle);

bool read(AppConfig &config);
bool persist(const AppConfig &config);
} // namespace synth

namespace hardware {
bool read(HardwareConfig &config);
bool persist(const HardwareConfig &config);

bool encode(const HardwareConfig &config, uint8_t *buf, size_t bufsize,
            size_t &out_len);
bool decode(HardwareConfig &config, const uint8_t *buf, size_t len);
} // namespace hardware
} // namespace teslasynth::app::configuration
