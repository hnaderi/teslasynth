#pragma once

#include "duration.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdint.h>
#include <string>

namespace teslasynth::core {

class Hertz {
  float _value;

  static constexpr uint32_t _coef_kilo = 1000;
  static constexpr uint32_t _coef_mega = 1000 * _coef_kilo;

public:
  explicit constexpr Hertz(float v) : _value(v) {}
  static constexpr Hertz kilohertz(uint32_t v) { return Hertz(v * 1000); }
  static constexpr Hertz megahertz(uint32_t v) { return Hertz(v * 1'000'000); }

  constexpr Hertz operator+(const Hertz b) const {
    return Hertz(_value + b._value);
  }
  constexpr Hertz operator-(const Hertz b) const {
    return Hertz(_value - b._value);
  }
  constexpr Hertz operator-() const { return Hertz(-_value); }
  constexpr Hertz operator*(const int b) const { return Hertz(_value * b); }
  constexpr Hertz operator*(const float b) const { return Hertz(_value * b); }
  constexpr operator float() const { return _value; }

  constexpr bool operator<(const Hertz &b) const { return _value < b._value; }
  constexpr bool operator>(const Hertz &b) const { return _value > b._value; }
  constexpr bool operator==(const Hertz &b) const {
    return fabsf(_value - b._value) < 0.001;
  }
  constexpr bool operator!=(const Hertz &b) const {
    return fabsf(_value - b._value) > 0.001;
  }
  constexpr bool operator<=(const Hertz &b) const { return _value <= b._value; }
  constexpr bool operator>=(const Hertz &b) const { return _value >= b._value; }
  constexpr bool is_zero() const { return _value == 0; }
  constexpr Duration32 period() const {
    return Duration32::micros(1e6 / _value);
  }

  inline operator std::string() const {
    if (_value > _coef_mega) {
      return std::to_string(_value / static_cast<float>(_coef_mega)) + "MHz";
    } else if (_value > _coef_kilo) {
      return std::to_string(_value / static_cast<float>(_coef_kilo)) + "KHz";
    } else {
      return std::to_string(_value) + "Hz";
    }
  }
};

inline constexpr Hertz operator""_hz(unsigned long long n) {
  return Hertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_khz(unsigned long long n) {
  return Hertz::kilohertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_mhz(unsigned long long n) {
  return Hertz::megahertz(static_cast<float>(n));
}

inline constexpr Hertz operator""_hz(long double n) {
  return Hertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_khz(long double n) {
  return Hertz::kilohertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_mhz(long double n) {
  return Hertz::megahertz(static_cast<float>(n));
}
} // namespace teslasynth::core
