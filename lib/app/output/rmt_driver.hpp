#pragma once

#include "midi_synth.hpp"
#include <cstddef>

namespace teslasynth::app::devices::rmt {
void pulse_write(const midisynth::Pulse *pulse, size_t len, uint8_t ch = 0);
void enable(void);
void disable(void);
} // namespace teslasynth::app::devices::rmt
