#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct NotePulse {
  Duration start;
  Duration32 period;
  EnvelopeLevel volume;

  constexpr bool is_zero() const { return volume.is_zero(); }
  inline operator std::string() const {
    return std::string("Note[start:") + std::string(start) +
           ", vol:" + std::string(volume) + ", period:" + std::string(period) +
           "]";
  }
};

struct MidiNote {
  uint8_t number;
  uint8_t velocity;

  constexpr Hertz frequency(Hertz tuning = 440_hz) const {
    return tuning * exp2f((number - 69) / 12.0f);
  }

  constexpr EnvelopeLevel volume() const {
    return EnvelopeLevel(velocity / 127.f);
  }
  constexpr bool operator==(MidiNote b) const {
    return number == b.number && velocity == b.velocity;
  }
  constexpr bool operator!=(MidiNote b) const {
    return number != b.number || velocity != b.velocity;
  }
};

class Note final {
  Hertz _freq = Hertz(0);
  Envelope _envelope =
      Envelope(ADSR{0_us, 0_us, EnvelopeLevel(0), 0_us, CurveType::Lin});
  Vibrato _vibrato;
  NotePulse _pulse;
  EnvelopeLevel _level, _volume;
  Duration _release, _now;
  bool _active = false;
  bool _released = false;

public:
  void start(const MidiNote &mnote, Duration time, Envelope env,
             Vibrato vibrato, Hertz tuning);
  void start(const MidiNote &mnote, Duration time, const Instrument &instrument,
             Hertz tuning);
  void start(const MidiNote &mnote, Duration time, Envelope env, Hertz tuning);
  void release(Duration time);

  void off();

  bool next();
  const NotePulse &current() const { return _pulse; }

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  const Duration &now() const { return _now; }
  const Hertz &frequency() const { return _freq; }
  const EnvelopeLevel &max_volume() const { return _volume; }
};

} // namespace teslasynth::synth
