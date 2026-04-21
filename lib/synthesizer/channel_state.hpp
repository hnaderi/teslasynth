// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "core/envelope_level.hpp"
#include "pitchbend.hpp"

namespace teslasynth::synth {
using namespace teslasynth::core;

struct ChannelState {
  PitchBend pitch_bend;
  EnvelopeLevel amplitude = core::EnvelopeLevel::max();
  float smoothing = 0.1;
};

} // namespace teslasynth::synth
