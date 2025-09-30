#include "instruments.hpp"
#include "synth.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

Note::Note() {}
Note::Note(uint8_t number, uint8_t velocity, uint8_t instrument,
           uint32_t time) {
  _number = number;
  _velocity = velocity;
  _on = _now = time;
  _active = _started = true;
}
void Note::release(uint32_t time) {
  _released = true;
  _release = time;
}
bool Note::tick(const Config &config, NotePulse &out) {
  auto active = _active;
  if (_active) {
    float freq = config.a440 * std::expf((_number - 69) / 12.0f);
    uint32_t period = config.ticks_per_sec / freq;
    uint32_t max_ticks = config.max_on_time * config.ticks_per_micro;
    uint32_t level = _velocity / 127.f * max_ticks;
    uint32_t duty = std::min(max_ticks, level);

    out.start = _now;
    out.off = _now + duty;
    out.end = _now + period;

    _now += period;
  }
  return active;
}
