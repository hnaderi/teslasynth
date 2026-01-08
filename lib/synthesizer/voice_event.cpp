#include "voice_event.hpp"
#include "presets.hpp"
#include "voices/note.hpp"
#include <core/envelope_level.hpp>

namespace teslasynth::synth {

template <class... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

struct ActiveVisitor {
  bool operator()(const std::monostate &) const { return false; }
  bool operator()(const Note &n) const { return n.is_active(); }
  bool operator()(const Hit &h) const { return h.is_active(); }
};

bool VoiceEvent::is_active() const {
  return std::visit(ActiveVisitor{}, state);
}

constexpr static NotePulse empty{
    .start = 0_s,
    .period = 0_s,
    .volume = EnvelopeLevel::zero(),
};

struct CurrentVisitor {
  const NotePulse &operator()(const std::monostate &) const { return empty; }
  const NotePulse &operator()(const Note &n) const { return n.current(); }
  const NotePulse &operator()(const Hit &h) const { return h.current(); }
};

const NotePulse &VoiceEvent::current() const {
  return std::visit(CurrentVisitor{}, state);
}
bool VoiceEvent::next() {
  bool res = false;

  std::visit(Overload{
                 [&](std::monostate &) { res = false; },
                 [&](Note &n) { res = n.next(); },
                 [&](Hit &h) { res = h.next(); },
             },
             state);
  if (!res)
    off();
  return res;
}

void VoiceEvent::start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                       const SoundPreset &preset, const ChannelState *channel) {
  std::visit(Overload{
                 [&](const PitchPreset &arg) {
                   state = Note{};
                   std::get<Note>(state).start(number, amplitude, time,
                                               *arg.instrument, arg.tuning,
                                               channel);
                 },
                 [&](const PercussivePreset &arg) {
                   state = Hit{};
                   std::get<Hit>(state).start(number, amplitude, time,
                                              *arg.percussion, channel);
                 },
             },
             preset);
}

void VoiceEvent::release(Duration time) {
  if (std::holds_alternative<Note>(state)) {
    std::get<Note>(state).release(time);
  }
}
void VoiceEvent::off() { state = std::monostate{}; }

struct TypeVisitor {
  constexpr VoiceEvent::Type operator()(const std::monostate &) const {
    return VoiceEvent::Type::None;
  }
  constexpr VoiceEvent::Type operator()(const Note &n) const {
    return VoiceEvent::Type::Tone;
  }
  constexpr VoiceEvent::Type operator()(const Hit &h) const {
    return VoiceEvent::Type::Hit;
  }
};

VoiceEvent::Type VoiceEvent::type() const {
  return std::visit(TypeVisitor{}, state);
}
} // namespace teslasynth::synth
