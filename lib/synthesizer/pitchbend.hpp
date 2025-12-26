#pragma once

#include "core.hpp"
#include "core/hertz.hpp"
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

class PitchBend {
  float normalized = 0.0f;

  // Note: Can become a dynamic parameter later
  constexpr static float range = 2.0f;

public:
  constexpr PitchBend() = default;
  constexpr PitchBend(float value)
      : normalized(value > 1    ? 1
                   : value < -1 ? -1
                                : value) {}

  constexpr static PitchBend midi(uint16_t v) {
    return PitchBend((float(v) - 8192.0f) / 8192.0f);
  }

  constexpr bool operator==(const PitchBend &b) const {
    return normalized == b.normalized && range == b.range;
  }

  constexpr bool operator!=(const PitchBend &b) const {
    return normalized != b.normalized || range != b.range;
  }

  constexpr float multiplier() const {
    float semitones = normalized * range;
    return exp2f(semitones / 12.0f);
  }

  constexpr Hertz operator*(const Hertz &f) const { return f * multiplier(); }

  inline operator std::string() const {
    return std::string("Bend: ") + std::to_string(multiplier());
  }
};

}; // namespace teslasynth::synth
