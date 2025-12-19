#include "percussion.hpp"
#include <core/envelope_level.hpp>
#include <core/hertz.hpp>

namespace teslasynth::synth {
namespace {
// fast PRNG using Xorshift32 algorithm
// Read this: https://en.wikipedia.org/wiki/Xorshift
float frand(uint32_t &x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return (x >> 8) * (1.0f / 16777216.0f);
}

constexpr Hertz min_prf = 200_hz, max_prf = 4_khz;
template <typename T> T clip(T t, const T &from, const T &to) {
  return std::max<T>(from, std::min<T>(t, to));
}
} // namespace

float Hit::random() { return frand(rng_state); }

bool Hit::next() {
  const bool active = is_active();
  if (active) {
    auto oscillation =
        clip(prf * (1 + noise_ * (2 * random() - 1)), min_prf, max_prf);
    if (oscillation.is_zero()) {
      now = end;
      return false;
    }

    current_.start = now;
    current_.period = oscillation.period();
    if (random() < skip_)
      current_.volume = EnvelopeLevel(0);
    else
      current_.volume = volume_ * EnvelopeLevel(random());

    now += current_.period;
  }
  return active;
}

void Hit::start(uint8_t velocity, const Percussion &params, Duration time) {
  // Xorshift relies on rng_state not be zero
  // Reset it if zero, otherwise use whatever value in memory (possibly some
  // garbage from other colocated data)
  if (rng_state == 0)
    rng_state = 0x12345678;

  now = time;
  end = time + params.burst;
  volume_ = EnvelopeLevel::logscale(velocity * 2 + 1);
  prf = params.prf;
  noise_ = params.noise;
  skip_ = params.skip;

  next();
}
} // namespace teslasynth::synth
