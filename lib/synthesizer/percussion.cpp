#include "percussion.hpp"
#include "core/duration.hpp"
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

constexpr Hertz min_prf = 20_hz, max_prf = 4_khz;
template <typename T> T clip(T t, const T &from, const T &to) {
  return std::max<T>(from, std::min<T>(t, to));
}
template <typename T> T lerp(T a, T b, float t) {
  return a * (1.0f - t) + b * t;
}
} // namespace

float Hit::random() { return frand(rng_state); }

bool Hit::next() {
  if (envelope_.is_off())
    now = end;
  const bool active = is_active();
  if (active) {
    float jitter = (2.0f * random() - 1.0f) * float(noise_) * volume_;

    Duration32 period =
        prf.is_zero() ? Duration32::micros(50 + random() * 2000)
                      : clip(prf * (1 + jitter), min_prf, max_prf).period();

    current_.start = now;
    current_.period = period;
    auto level = envelope_.update(period, true);
    if (skip_ > 0 && random() < skip_)
      current_.volume = EnvelopeLevel(0);
    else
      current_.volume = volume_ * level * EnvelopeLevel(0.75 + 0.25 * random());

    now += period;
  }
  return active;
}

void Hit::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                const Percussion &params) {
  // Xorshift relies on rng_state not be zero
  // Reset it if zero, otherwise use whatever value in memory (possibly some
  // garbage from other colocated data)
  if (rng_state == 0)
    rng_state = 0x12345678;

  now = time;
  auto scaled_burst = params.burst * (0.5f + 0.5f * amplitude);
  end = time + scaled_burst;
  envelope_ = params.envelope;
  volume_ = amplitude;
  prf = params.prf;
  noise_ = Probability(lerp(float(params.noise), 1.0f, amplitude * 0.3f));
  skip_ = params.skip;

  next();
}
} // namespace teslasynth::synth
