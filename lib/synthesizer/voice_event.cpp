#include "voice_event.hpp"
#include <core/envelope_level.hpp>
#include <notes.hpp>

namespace teslasynth::synth {
bool VoiceEvent::is_active() const {
  switch (type_) {
  case Type::Tone:
    return state.note.is_active();
  case Type::Hit:
    return state.hit.is_active();
  default:
    return false;
  }
}

constexpr NotePulse empty;

const NotePulse &VoiceEvent::current() const {
  switch (type_) {
  case Type::Tone:
    return state.note.current();
  case Type::Hit:
    return state.hit.current();
  default:
    return empty;
  }
}
bool VoiceEvent::next() {
  bool res = false;
  switch (type_) {
  case Type::Tone:
    res = state.note.next();
    break;
  case Type::Hit:
    res = state.hit.next();
    break;
  default:
    res = false;
  }
  if (!res)
    off();
  return res;
}

void VoiceEvent::start(const MidiNote &mnote, Duration time,
                       const Instrument &instrument, Hertz tuning) {
  type_ = Type::Tone;
  state.note.start(mnote, time, instrument, tuning);
}
void VoiceEvent::start(uint8_t velocity, const Percussion &params,
                       Duration time) {
  type_ = Type::Hit;
  state.hit.start(velocity, params, time);
}

void VoiceEvent::release(Duration time) {
  switch (type_) {
  case Type::Tone:
    state.note.release(time);
  default:
    return;
  }
}
void VoiceEvent::off() { type_ = Type::None; }
} // namespace teslasynth::synth
