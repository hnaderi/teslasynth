#pragma once

#include "config_data.hpp"
#include "sdkconfig.h"

using AppConfig =
    teslasynth::midisynth::Configuration<CONFIG_TESLASYNTH_OUTPUT_COUNT>;
using AppMidiRoutingConfig =
    teslasynth::midisynth::MidiRoutingConfig<CONFIG_TESLASYNTH_OUTPUT_COUNT>;
