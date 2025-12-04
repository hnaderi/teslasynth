#pragma once

#include "midi_synth.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::Config;

const Config &load_config();
const Config &get_config();
void update_config(const Config &config);
void reset_config();
void save_config();

} // namespace teslasynth::app::configuration
