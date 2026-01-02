#pragma once

#include "core/duration.hpp"
#include "core/envelope_level.hpp"
#include "core/hertz.hpp"
#include "core/probability.hpp"

#include <ostream>

template <typename T = uint64_t>
std::ostream &operator<<(std::ostream &out,
                         const teslasynth::core::SimpleDuration<T> &d) {
  return out << std::string(d);
}
