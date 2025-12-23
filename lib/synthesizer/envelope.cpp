#include "envelope.hpp"
#include "core.hpp"
#include <cmath>
#include <optional>

using namespace teslasynth::core;

namespace teslasynth::synth {

Envelope::Envelope(ADSR configs)
    : _configs(configs),
      _current(configs.type == Const ? Curve(configs.sustain)
                                     : Curve(EnvelopeLevel(0), EnvelopeLevel(1),
                                             configs.attack, configs.type)),
      _stage(Attack) {}

Envelope::Envelope(EnvelopeLevel level) : Envelope(ADSR::constant(level)) {}

Duration32 Envelope::progress(Duration32 delta, bool on) {
  Duration32 remained = delta;
  auto dt = _current.will_reach_target(remained);
  while (dt && (!remained.is_zero() || !on)) {
    switch (_stage) {
    case Attack:
      _current = Curve(EnvelopeLevel(1), _configs.sustain, _configs.decay,
                       _configs.type);
      _stage = Decay;
      break;
    case Decay:
      _current = Curve(_configs.sustain);
      _stage = Sustain;
      break;
    case Sustain:
      if (!on) {
        _current = Curve(_configs.sustain, EnvelopeLevel(0), _configs.release,
                         _configs.type);
        _stage = Release;
      } else {
        dt = 0_us;
      }
      break;
    case Release:
      _current = Curve(EnvelopeLevel(0));
      _stage = Off;
      break;
    case Off:
      return 0_us;
    }

    remained = *dt;
    dt = _current.will_reach_target(remained);
  }
  return remained;
}

EnvelopeLevel Envelope::update(Duration32 delta, bool on) {
  if (is_off())
    return EnvelopeLevel(0);
  auto remained = progress(delta, on);
  return _current.update(remained);
}

}; // namespace teslasynth::synth
