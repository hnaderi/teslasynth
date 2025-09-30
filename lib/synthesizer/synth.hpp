#pragma once

#include "core.hpp"
#include <cstdint>

struct Config {
  static constexpr uint32_t ticks_per_sec = 2'000'000;
  static constexpr auto ticks_per_micro = ticks_per_sec / 1'000'000;
  static constexpr uint8_t max_notes = 4;

  uint16_t min_on_time = 0, max_on_time = 100, min_deadtime = 100;
  float a440 = 440.0f;
};

struct NotePulse {
  uint32_t start, off, end;
};

class Note {
  uint8_t _number, _velocity;
  uint32_t _on, _release, _now;
  uint8_t instrument;
  bool _started = false;
  bool _active = false;
  bool _released = false;
  bool _high = false;

public:
  Note();
  Note(uint8_t number, uint8_t velocity, uint8_t instrument, uint32_t time);
  void release(uint32_t time);
  bool tick(const Config &config, NotePulse &out);

  bool is_active() const { return _active; }
  uint32_t time() const { return _now; }
  uint8_t number() const { return _number; }
};

class SynthChannel {
  uint8_t instrument = 0;
  Note notes[Config::max_notes];
  uint8_t playing_notes = 0;
  uint32_t time = 0;
  const Config &_config;

public:
  SynthChannel(const Config &config);
  void on_note_on(uint8_t number, uint8_t velocity, uint32_t time);
  void on_note_off(uint8_t number, uint8_t velocity, uint32_t time);
  void on_program_change(uint8_t value);
  void on_control_change(uint8_t value);
  uint16_t render(Pulse *buffer, uint16_t max_size, uint16_t delta);
};
