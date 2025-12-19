#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace teslasynth::core {

class Probability {
  float _value;

public:
  constexpr explicit Probability() : _value(0) {}
  constexpr explicit Probability(float level)
      : _value(level > 1   ? 1.f
               : level < 0 ? 0.f
                           : level) {}

  constexpr static Probability zero() { return Probability(); }
  constexpr static Probability max() { return Probability(1); }

  constexpr bool is_zero() const { return _value == 0; }
  constexpr Probability operator+(const Probability &b) const {
    return Probability(_value + b._value);
  }
  constexpr Probability operator+(float b) const {
    return Probability(_value + b);
  }
  Probability &operator+=(const Probability &b) {
    if (1.f - _value < b._value)
      _value = 1.f;
    else
      _value += b._value;
    return *this;
  }
  Probability &operator+=(float b) {
    _value += b;
    if (_value > 1.f)
      _value = 1.f;
    else if (_value < 0)
      _value = 0.f;
    return *this;
  }
  float operator-(const Probability &b) const { return _value - b._value; }

  constexpr Probability operator*(const Probability &b) const {
    return Probability(b._value * _value);
  }
  constexpr bool operator<(const Probability &b) const {
    return _value < b._value;
  }
  constexpr bool operator>(const Probability &b) const {
    return _value > b._value;
  }
  constexpr bool operator==(const Probability &b) const {
    return std::fabs(_value - b._value) < 1e-3f;
  }
  constexpr bool operator!=(const Probability &b) const {
    return _value != b._value;
  }
  constexpr bool operator<=(const Probability &b) const {
    return _value <= b._value;
  }
  constexpr bool operator>=(const Probability &b) const {
    return _value >= b._value;
  }

  constexpr operator float() const { return _value; }
  inline operator std::string() const {
    return std::to_string(_value * 100) + "%";
  }
};
} // namespace teslasynth::core
