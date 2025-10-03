#include "envelope.hpp"
#include "core.hpp"
#include <cmath>
#include <cstdint>
#include <stdio.h>

constexpr float epsilon = 0.001;
// -log_e(0.001)
constexpr float logfactor = 6.907755278982137;

Curve::Curve(EnvelopeLevel start, EnvelopeLevel target, Duration total,
             CurveType type)
    : _target(target), _type(type), _total(total), _current(start) {
  const auto t = total.value();
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

EnvelopeLevel Curve::update(Duration delta) {
  if (_target_reached)
    return _target;

  const uint32_t dt = delta.value();
  switch (_type) {
  case Exp:
    _current += (_target - _current) * (1 - std::expf(-(float)dt / _state.tau));
    break;
  case Lin:
    _current += _state.slope * dt;
    break;
  }

  _elapsed += delta;
  _target_reached = _current == _target || _elapsed >= _total;
  return _current;
}
