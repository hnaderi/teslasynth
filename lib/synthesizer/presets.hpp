#pragma once

#include "array"
#include "envelope.hpp"
#include "lfo.hpp"
#include <core/hertz.hpp>
#include <instruments.hpp>
#include <percussion.hpp>
#include <stddef.h>
#include <variant>

namespace teslasynth::synth {
struct PitchPreset {
  const Instrument *instrument;
  Hertz tuning;

  constexpr bool operator==(const PitchPreset &b) const {
    return tuning == b.tuning && instrument == b.instrument;
  }
  constexpr bool operator!=(const PitchPreset &b) const {
    return tuning != b.tuning || instrument != b.instrument;
  }
};
struct PercussivePreset {
  const Percussion *percussion;

  constexpr bool operator==(const PercussivePreset &b) const {
    return percussion == b.percussion;
  }
  constexpr bool operator!=(const PercussivePreset &b) const {
    return percussion != b.percussion;
  }
};

typedef std::variant<PitchPreset, PercussivePreset> SoundPreset;
} // namespace teslasynth::synth
