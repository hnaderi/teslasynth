#pragma once

#include "../channel_state.hpp"
#include "core.hpp"
#include "core/duration.hpp"
#include "core/hertz.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "pulse.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

class Note final {
  Hertz _freq = Hertz(0), _current_freq = Hertz(0);
  Envelope _envelope;
  Vibrato _vibrato;
  NotePulse _pulse;
  EnvelopeLevel _level, _volume;
  Duration _release, _now;
  bool _active = false;
  bool _released = false;

  ChannelState const *_channel;

public:
  void start(Hertz prf, EnvelopeLevel amplitude, Duration time,
             const Envelope &env, const Vibrato &vibrato,
             const ChannelState *channel = nullptr);

  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const Envelope &env, const Vibrato &vibrato, Hertz tuning,
             const ChannelState *channel = nullptr);

  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const Instrument &instrument, Hertz tuning,
             const ChannelState *channel = nullptr);

  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const Envelope &env, Hertz tuning,
             const ChannelState *channel = nullptr);
  void release(Duration time);

  void off();

  bool next();
  const NotePulse &current() const { return _pulse; }

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  const Duration &now() const { return _now; }
  const Hertz &frequency() const { return _freq; }
  const EnvelopeLevel &max_volume() const { return _volume; }

  static constexpr Hertz frequency_for(uint8_t number, Hertz tuning = 440_hz) {
    return tuning * exp2f((number - 69) / 12.0f);
  }
};

} // namespace teslasynth::synth
