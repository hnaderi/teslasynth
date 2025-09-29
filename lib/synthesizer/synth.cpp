#include "synth.hpp"
#include "instruments.hpp"

SynthChannel::SynthChannel() {}

void SynthChannel::on_note_on(uint8_t number, uint8_t velocity, uint32_t time) {
  if (velocity == 0)
    on_note_off(number, velocity, time);
  else {
    for (uint8_t i = 0; i < MAX_NOTES; i++) {
      if (notes[i].is_active())
        continue;
      notes[i].start(number, velocity, instrument, time);
      return;
    }

    // Steal a voice
    for (uint8_t i = 0; i < MAX_NOTES; i++) {
      if (notes[i].is_active())
        continue;
      notes[i] = Note();
      notes[i].start(number, velocity, instrument, time);
      return;
    }
  }
}
void SynthChannel::on_note_off(uint8_t number, uint8_t velocity,
                               uint32_t time) {
  for (uint8_t i = 0; i < MAX_NOTES; i++) {
    if (notes[i].is_active())
      continue;
    notes[i].start(number, velocity, instrument, time);
    return;
  }
}
void SynthChannel::on_program_change(uint8_t value) {}
void SynthChannel::on_control_change(uint8_t value) {}

inline uint8_t next_note(Note *notes) {
  auto out = 0;
  auto min = notes[0].time();
  for (int i = 1; i < MAX_NOTES; i++) {
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
  auto next = next_note(notes);
  notes[next].tick(&pulse);
  return 0;
}
