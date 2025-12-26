#pragma once

#include "core/duration.hpp"
#include "core/envelope_level.hpp"
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct NotePulse {
  Duration start;
  Duration32 period;
  EnvelopeLevel volume;

  constexpr bool is_zero() const { return volume.is_zero(); }
  inline operator std::string() const {
    return std::string("Note[start:") + std::string(start) +
           ", vol:" + std::string(volume) + ", period:" + std::string(period) +
           "]";
  }
};

} // namespace teslasynth::synth
