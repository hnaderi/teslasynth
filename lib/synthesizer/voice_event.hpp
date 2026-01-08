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
  using VoiceState = std::variant<std::monostate, Note, Hit>;
  VoiceState state{std::monostate{}};

public:
  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const SoundPreset &preset, const ChannelState *channel = nullptr);

  void release(Duration time);
  void off();

  bool next();
  const NotePulse &current() const;
  bool is_active() const;
  Type type() const;
};
} // namespace teslasynth::synth
