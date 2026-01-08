#include "core.hpp"
#include "envelope.hpp"
#include <cmath>
#include <optional>

namespace teslasynth::synth {
using namespace teslasynth::core;

// -log_e(0.001)
constexpr float logfactor = 6.907755278982137;

Curve::Curve(EnvelopeLevel start, EnvelopeLevel target, Duration32 total,
             CurveType type)
    : _target(target), _type(type), _total(total), _current(start),
      _const(false) {
  const auto t = total.micros();
  if (t <= 0) {
    _target_reached = true;
    _current = target;
  } else
    switch (type) {
    case Exp:
      _state.tau = (float)t / logfactor;
      break;
    case Lin:
      _state.slope = (target - start) / t;
      break;
    }
}
Curve::Curve(EnvelopeLevel constant)
    : _target(constant), _type(Lin), _current(constant), _target_reached(false),
      _const(true) {}

std::optional<Duration32>
Curve::how_much_remains_after(const Duration32 &dt) const {
  if (!_const) {
    return (dt + _elapsed) - _total;
  } else {
    return std::nullopt;
  }
}

EnvelopeLevel Curve::update(Duration32 delta) {
  if (_target_reached || _const) {
    // noop
  } else if (_elapsed + delta >= _total) {
    _target_reached = true;
    _elapsed = _total;
    _current = _target;
  } else {
    const auto dt = delta.micros();
    if (_type == Exp)
      _current += (_target - _current) * (1 - expf(-(float)dt / _state.tau));
    else if (_type == Lin)
      _current += _state.slope * dt;

    _elapsed += delta;
    _target_reached = _elapsed >= _total;
  }
  return _current;
}

}; // namespace teslasynth::synth
