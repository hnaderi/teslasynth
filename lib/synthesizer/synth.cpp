#include "synth.hpp"
#include "instruments.hpp"

SynthChannel::SynthChannel(const Config &config) : _config(config) {}

void SynthChannel::on_note_on(uint8_t number, uint8_t velocity, uint32_t time) {
  if (velocity == 0)
    on_note_off(number, velocity, time);
  else {
    for (uint8_t i = 0; i < _config.max_notes; i++) {
      if (notes[i].is_active())
        continue;
      notes[i] = Note(number, velocity, instrument, time);
      return;
    }

    // Steal a voice
    for (uint8_t i = 0; i < _config.max_notes; i++) {
      if (notes[i].is_active())
        continue;
      notes[i] = Note(number, velocity, instrument, time);
      return;
    }
  }
}
void SynthChannel::on_note_off(uint8_t number, uint8_t velocity,
                               uint32_t time) {
  for (uint8_t i = 0; i < _config.max_notes; i++) {
    if (notes[i].is_active())
      continue;
    notes[i].release(time);
    return;
  }
}
void SynthChannel::on_program_change(uint8_t value) {}
void SynthChannel::on_control_change(uint8_t value) {}

inline uint8_t next_note(Note *notes, const Config &config) {
  auto out = 0;
  auto min = notes[0].time();
  for (int i = 1; i < config.max_notes; i++) {
    auto time = notes[i].time();
    if (time < min) {
      out = i;
      min = time;
    }
  }
  return out;
}

uint16_t SynthChannel::render(Pulse *buffer, uint16_t max_size,
                              uint16_t delta) {
  NotePulse pulse;
  auto next = next_note(notes, _config);
  notes[next].tick(_config, pulse);
  return 0;
}
