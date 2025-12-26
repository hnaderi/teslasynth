#pragma once

#include "envelope.hpp"
#include <core.hpp>
#include <core/duration.hpp>
#include <core/probability.hpp>
#include <cstdint>
#include <stddef.h>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct Percussion {
  Duration32 burst = 0_us;
  Hertz prf = 0_hz;
  Probability noise = Probability();
  Probability skip = Probability();
  envelopes::EnvelopeConfig envelope = EnvelopeLevel(1);

  constexpr bool operator==(const Percussion &b) const {
    return burst == b.burst && prf == b.prf && noise == b.noise &&
           skip == b.skip && envelope == b.envelope;
  }
  constexpr bool operator!=(const Percussion &b) const {
    return burst != b.burst || prf != b.prf || noise != b.noise ||
           skip != b.skip || envelope != b.envelope;
  }

  inline operator std::string() const {
    std::string stream =
        "Burst " + std::string(burst) + " PRF " + std::string(prf) + " Noise " +
        std::string(noise) + " Skip " + std::string(skip) + " Envelope " +
        std::visit([](auto const &e) { return std::string(e); }, envelope);
    return stream;
  }
};
} // namespace teslasynth::synth
