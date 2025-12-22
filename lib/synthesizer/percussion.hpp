#pragma once

#include <core.hpp>
#include <core/duration.hpp>
#include <core/probability.hpp>
#include <cstdint>
#include <notes.hpp>
#include <stddef.h>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct Percussion {
  Duration32 burst = 0_us;
  Hertz prf = 0_hz;
  Probability noise = Probability();
  Probability skip = Probability();

  constexpr bool operator==(const Percussion &b) const {
    return burst == b.burst && prf == b.prf && noise == b.noise &&
           skip == b.skip;
  }
  constexpr bool operator!=(const Percussion &b) const {
    return burst != b.burst || prf != b.prf || noise != b.noise ||
           skip != b.skip;
  }

  inline operator std::string() const {
    std::string stream = "Burst " + std::string(burst) + " PRF " +
                         std::string(prf) + " Noise " + std::string(noise) +
                         " Skip " + std::string(skip);
    return stream;
  }
};

class Hit {
  uint32_t rng_state;
  Duration end, now;
  Hertz prf = 0_hz;
  Probability noise_ = Probability(), skip_ = Probability();
  EnvelopeLevel volume_;
  NotePulse current_;
  inline float random();

public:
  void start(const MidiNote& mnote, Duration time, const Percussion &params);
  bool next();
  const NotePulse &current() const { return current_; }
  bool is_active() const { return now < end; }
};
} // namespace teslasynth::synth
