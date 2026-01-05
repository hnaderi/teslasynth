#include "lfo.hpp"
#include <cmath>
#include <math.h>

namespace teslasynth::synth {
using namespace teslasynth::core;

constexpr float _2pi = 6.2831853071795864769;

Hertz Vibrato::offset(const Duration &now) {
  float t = (now.micros() / 1e6f);
  float phase = fmod(freq * _2pi * t, _2pi);
  return depth * sinf(phase);
}

} // namespace teslasynth::synth
