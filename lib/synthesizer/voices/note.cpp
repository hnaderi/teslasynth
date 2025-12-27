#include "note.hpp"
#include "core.hpp"
#include "core/envelope_level.hpp"
#include "core/functions.hpp"
#include "core/hertz.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace teslasynth::synth {

void Note::start(Hertz prf, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, const Vibrato &vibrato,
                 const ChannelState *channel) {
  if (_active && amplitude.is_zero())
    return release(time);
  _current_freq = _freq = prf;
  _envelope = env;
  _vibrato = vibrato;
  _active = true;
  _released = false;
  _level = _envelope.update(0_us, true);
  _volume = amplitude;
  _now = time;
  _channel = channel;
  next();
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, const Vibrato &vibrato, Hertz tuning,
                 const ChannelState *channel) {
  start(frequency_for(number, tuning), amplitude, time, env, vibrato, channel);
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Instrument &instrument, Hertz tuning,
                 const ChannelState *channel) {
  start(number, amplitude, time, instrument.envelope, instrument.vibrato,
        tuning, channel);
}

void Note::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const Envelope &env, Hertz tuning,
                 const ChannelState *channel) {
  start(number, amplitude, time, env, Vibrato::none(), tuning, channel);
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
    Duration32 period = (_current_freq + _vibrato.offset(now())).period();
    _pulse.start = _now;
    _pulse.volume =
        _level * _volume *
        (_channel != nullptr ? _channel->amplitude : EnvelopeLevel::max());
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
    if (_channel != nullptr) {
      if (!_channel->pitch_bend.is_zero()) {
        _current_freq = lerp(_current_freq, _channel->pitch_bend * _freq,
                             _channel->smoothing);
      }
    }
  }
  return _active;
}

} // namespace teslasynth::synth
