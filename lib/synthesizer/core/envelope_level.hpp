#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdint.h>
#include <string>

namespace teslasynth::core {

class EnvelopeLevel {
  float _value;

public:
  constexpr explicit EnvelopeLevel() : _value(0) {}
  constexpr explicit EnvelopeLevel(float level)
      : _value(level > 1   ? 1.f
               : level < 0 ? 0.f
                           : level) {}

  constexpr static EnvelopeLevel zero() { return EnvelopeLevel(); }
  constexpr static EnvelopeLevel max() { return EnvelopeLevel(1); }
  constexpr static EnvelopeLevel logscale(uint8_t value) {
    return EnvelopeLevel(log2f(1.f + value) / 8.f);
  }

  constexpr bool is_zero() const { return _value == 0; }
  constexpr EnvelopeLevel operator+(const EnvelopeLevel &b) const {
    return EnvelopeLevel(_value + b._value);
  }
  constexpr EnvelopeLevel operator+(float b) const {
    return EnvelopeLevel(_value + b);
  }
  EnvelopeLevel &operator+=(const EnvelopeLevel &b) {
    if (1.f - _value < b._value)
      _value = 1.f;
    else
      _value += b._value;
    return *this;
  }
  EnvelopeLevel &operator+=(float b) {
    _value += b;
    if (_value > 1.f)
      _value = 1.f;
    else if (_value < 0)
      _value = 0.f;
    return *this;
  }
  float operator-(const EnvelopeLevel &b) const { return _value - b._value; }

  template <typename T>
  constexpr SimpleDuration<T> operator*(const SimpleDuration<T> &b) const {
    return b * _value;
  }
  constexpr EnvelopeLevel operator*(const EnvelopeLevel &b) const {
    return EnvelopeLevel(b._value * _value);
  }
  constexpr bool operator<(const EnvelopeLevel &b) const {
    return _value < b._value;
  }
  constexpr bool operator>(const EnvelopeLevel &b) const {
    return _value > b._value;
  }
  constexpr bool operator==(const EnvelopeLevel &b) const {
    return std::fabs(_value - b._value) < 1e-3f;
  }
  constexpr bool operator!=(const EnvelopeLevel &b) const {
    return _value != b._value;
  }
  constexpr bool operator<=(const EnvelopeLevel &b) const {
    return _value <= b._value;
  }
  constexpr bool operator>=(const EnvelopeLevel &b) const {
    return _value >= b._value;
  }

  constexpr operator float() const { return _value; }
  inline operator std::string() const { return std::to_string(_value); }
};
} // namespace teslasynth::core
