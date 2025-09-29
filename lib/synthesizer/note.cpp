#include "note.hpp"
#include "instruments.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>


Note::Note() {}
void Note::start(uint8_t number, uint8_t velocity, uint8_t instrument,
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
bool Note::tick(NotePulse *out) {
  auto active = _active;
  if (_active) {
    float freq = 440.0f * std::expf((_number - 69) / 12.0f);
    uint32_t period = ticks_per_sec / freq;
    uint32_t duty = std::min(1, 2);

    _now += period;

    out->start = 0; // TODO now?
    out->off = 10;
    out->end = 100;
  }
  return active;
}
