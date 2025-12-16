#pragma once

#include "esp_err.h"
#include "hardware.hpp"
#include "synth.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::ChannelConfig;

namespace synth {
AppConfig read();
bool read(AppConfig &config);
esp_err_t persist(const AppConfig &config);
} // namespace synth

namespace hardware {
bool read(HardwareConfig &config);
bool persist(const HardwareConfig &config);

bool encode(const HardwareConfig &config, uint8_t *buf, size_t bufsize,
            size_t &out_len);
bool decode(HardwareConfig &config, const uint8_t *buf, size_t len);
} // namespace hardware
} // namespace teslasynth::app::configuration
