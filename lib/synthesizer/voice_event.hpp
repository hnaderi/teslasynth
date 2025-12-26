#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "lfo.hpp"
#include "percussion.hpp"
#include "pulse.hpp"
#include "voices/hit.hpp"
#include "voices/note.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <presets.hpp>
#include <string>

namespace teslasynth::synth {
class VoiceEvent {
public:
  enum class Type : uint8_t {
    None = 0,
    Tone = 1,
    Hit = 2,
  };

private:
  union State {
    Note note;
    Hit hit;
  } state = {};
  Type type_ = Type::None;

public:
  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const SoundPreset &preset);

  void release(Duration time);
  void off();

  bool next();
  const NotePulse &current() const;
  bool is_active() const;
  const Type &type() const { return type_; }
};
} // namespace teslasynth::synth
