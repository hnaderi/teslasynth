#pragma once

#include "core.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

enum CurveType { Lin, Exp, Const };
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
  bool _target_reached = false;

public:
  Curve(EnvelopeLevel start, EnvelopeLevel target, Duration32 total,
        CurveType type);
  Curve(EnvelopeLevel constant);
  EnvelopeLevel update(Duration32 delta);
  bool is_target_reached() const { return _target_reached; }
  std::optional<Duration32> will_reach_target(const Duration32 &dt) const;
};
} // namespace teslasynth::synth
