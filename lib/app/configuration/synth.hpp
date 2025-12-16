#pragma once

#include "midi_synth.hpp"
#include "sdkconfig.h"

typedef teslasynth::midisynth::Configuration<CONFIG_TESLASYNTH_OUTPUT_COUNT>
    AppConfig;
