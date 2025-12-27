#pragma once

#include <cstdint>

namespace teslasynth::core {

template <typename T> T clip(T t, const T &from, const T &to) {
  return std::max<T>(from, std::min<T>(t, to));
}
template <typename T> T lerp(T a, T b, float t) {
  return a * (1.0f - t) + b * t;
}

} // namespace teslasynth::core
