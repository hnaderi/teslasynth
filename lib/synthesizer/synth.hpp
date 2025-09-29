#pragma once

#include "core.hpp"
#include <stdint.h>
#include "note.hpp"

#define MAX_NOTES 4

class SynthChannel {
  uint8_t instrument = 0;
  Note notes[MAX_NOTES];
  uint8_t playing_notes = 0;
  uint32_t time = 0;

public:
  SynthChannel();
  void on_note_on(uint8_t number, uint8_t velocity, uint32_t time);
  void on_note_off(uint8_t number, uint8_t velocity, uint32_t time);
  void on_program_change(uint8_t value);
  void on_control_change(uint8_t value);
  uint16_t render(Pulse *buffer, uint16_t max_size, uint16_t delta);
};
