#pragma once

#include "core.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

enum CurveType { Lin, Exp };
union CurveState {
  float tau;   // Exp
  float slope; // Lin
};

class Curve {
  EnvelopeLevel _target;
  CurveType _type;
  Duration32 _total;

  Duration32 _elapsed;
  EnvelopeLevel _current;
  CurveState _state;
  bool _target_reached = false, _const;

public:
  Curve(EnvelopeLevel start, EnvelopeLevel target, Duration32 total,
        CurveType type);
  Curve(EnvelopeLevel constant);
  EnvelopeLevel update(Duration32 delta);
  bool is_target_reached() const { return _target_reached; }
  std::optional<Duration32> how_much_remains_after(const Duration32 &dt) const;
  constexpr CurveType type() const { return _type; }
  constexpr bool hold() const { return _const; }
};
} // namespace teslasynth::synth
