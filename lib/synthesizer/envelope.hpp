#pragma once

#include "core.hpp"
#include <algorithm>
#include <cstdint>
#include <stdint.h>

enum CurveType { Lin, Exp };

class EnvelopeLevel {
  uint8_t value;

public:
  explicit EnvelopeLevel(int level) {
    if (level > 100)
      value = 100;
    else if (level < 0)
      value = 0;
    else
      value = level;
  }

  EnvelopeLevel operator+(const EnvelopeLevel &b) const {
    return EnvelopeLevel(value + b.value);
  }
  EnvelopeLevel &operator+=(const EnvelopeLevel &b) {
    if (100 - value < b.value)
      value = 100;
    else
      value += b.value;
    return *this;
  }

  Duration operator*(const Duration &b) const { return b * (value / 100.f); }
  constexpr bool operator<(const EnvelopeLevel &b) const {
    return value < b.value;
  }
  constexpr bool operator>(const EnvelopeLevel &b) const {
    return value > b.value;
  }
  constexpr bool operator==(const EnvelopeLevel &b) const {
    return value == b.value;
  }
  constexpr bool operator!=(const EnvelopeLevel &b) const {
    return value != b.value;
  }
  constexpr bool operator<=(const EnvelopeLevel &b) const {
    return value <= b.value;
  }
  constexpr bool operator>=(const EnvelopeLevel &b) const {
    return value >= b.value;
  }
};

struct ADSR {
  Duration attack, decay;
  EnvelopeLevel sustain;
  Duration release;
  enum CurveType type;
};

class Curve {
  const EnvelopeLevel _target;
  EnvelopeLevel _current;
};

class Envelope {
  const ADSR &_configs;

public:
  Envelope(const ADSR &configs) : _configs(configs) {}
  EnvelopeLevel update(Duration delta, bool on);
};
