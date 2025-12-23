#pragma once

#include "core.hpp"
#include "curve.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct ADSR {
  Duration32 attack;
  Duration32 decay;
  EnvelopeLevel sustain = EnvelopeLevel(1);
  Duration32 release;
  enum CurveType type = CurveType::Const;

  constexpr static ADSR constant(EnvelopeLevel level) {
    return {0_us, 0_us, level, 0_us, Const};
  }
  constexpr static ADSR linear(Duration32 attack, Duration32 decay,
                               EnvelopeLevel sustain, Duration32 release) {
    return {attack, decay, sustain, release, Lin};
  }
  constexpr static ADSR exponential(Duration32 attack, Duration32 decay,
                                    EnvelopeLevel sustain, Duration32 release) {
    return {attack, decay, sustain, release, Exp};
  }

  constexpr bool operator==(ADSR b) const {
    return attack == b.attack && decay == b.decay && sustain == b.sustain &&
           release == b.release && type == b.type;
  }

  constexpr bool operator!=(ADSR b) const {
    return attack != b.attack || decay != b.decay || sustain != b.sustain ||
           release != b.release || type != b.type;
  }

  inline operator std::string() const {
    std::string stream;
    switch (type) {
    case Lin:
      stream = "lin";
      break;
    case Exp:
      stream = "exp";
      break;
    case Const:
      stream = "const";
      break;
    }

    stream += " A: " + std::string(attack) + " D: " + std::string(decay) +
              " S: " + std::string(sustain) + " R: " + std::string(release);

    return stream;
  }
};

class Envelope {
  ADSR _configs;
  Curve _current;

  Duration32 progress(Duration32 delta, bool on);

public:
  enum Stage { Attack, Decay, Sustain, Release, Off };

  Envelope(ADSR configs);
  Envelope(EnvelopeLevel level);
  EnvelopeLevel update(Duration32 delta, bool on);
  Stage stage() const { return _stage; }
  bool is_off() const { return _stage == Off; }

private:
  Stage _stage;
};

} // namespace teslasynth::synth
