#include "core.hpp"
#include "core/envelope_level.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "notes.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace teslasynth::synth {

void Note::start(Hertz prf, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, const Vibrato &vibrato) {
  if (_active && amplitude.is_zero())
    return release(time);
  _freq = prf;
  _envelope = env;
  _vibrato = vibrato;
  _active = true;
  _released = false;
  _level = _envelope.update(0_us, true);
  _volume = amplitude;
  _now = time;
  next();
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, const Vibrato &vibrato, Hertz tuning) {
  start(frequency_for(number, tuning), amplitude, time, env, vibrato);
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Instrument &instrument, Hertz tuning) {
  start(number, amplitude, time, instrument.envelope, instrument.vibrato,
        tuning);
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, Hertz tuning) {
  start(number, amplitude, time, env, Vibrato::none(), tuning);
}

void Note::release(Duration time) {
  _released = true;
  _release = time;
}

void Note::off() { _active = false; }

bool Note::next() {
  if (_envelope.is_off())
    _active = false;
  if (_active) {
    Duration32 period = (_freq + _vibrato.offset(now())).period();
    _pulse.start = _now;
    _pulse.volume = _level * _volume;
    _pulse.period = period;

    Duration next_tick = _now + period;
    if (!_released || next_tick < _release)
      _level = _envelope.update(period, true);
    else {
      if (_now <= _release) {
        uint32_t remained = _release.micros() - _now.micros();
        _envelope.update(Duration32::micros(remained), true);
        remained = (*(next_tick - _release)).micros();
        _level = _envelope.update(Duration32::micros(remained), false);
      } else
        _level = _envelope.update(period, false);
    }
    _now = next_tick;
  }
  return _active;
}

} // namespace teslasynth::synth
