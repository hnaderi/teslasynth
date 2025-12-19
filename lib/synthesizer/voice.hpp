#pragma once

#include "notes.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#ifndef CONFIG_MAX_NOTES
#define CONFIG_MAX_NOTES 4
#endif

namespace teslasynth::synth {
using namespace teslasynth::core;

template <std::uint8_t MAX_NOTES = CONFIG_MAX_NOTES> class Voice final {
  uint8_t _size = MAX_NOTES;
  std::array<Note, MAX_NOTES> _notes;
  std::array<uint8_t, MAX_NOTES> _numbers;

public:
  Voice() {}
  Voice(const Voice &) = delete;
  Voice(Voice &&) = delete;
  Voice &operator=(const Voice &) = delete;
  Voice &operator=(Voice &&) = delete;
  Voice(uint8_t size) : _size(std::min(size, MAX_NOTES)) {}
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, Hertz tuning) {
    uint8_t idx = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] != mnote.number)
        continue;
      idx = i;
      break;
    }
    _notes[idx].start(mnote, time, instrument, tuning);
    _numbers[idx] = mnote.number;
    return _notes[idx];
  }
  void release(uint8_t number, Duration time) {
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] == number) {
        _notes[i].release(time);
        return;
      }
    }
  }
  inline void release(const MidiNote &mnote, Duration time) {
    release(mnote.number, time);
  }
  void off() {
    for (uint8_t i = 0; i < _size; i++)
      _notes[i].off();
  }

  Note &next() {
    uint8_t out = 0;
    Duration min = Duration::max();
    for (uint8_t i = 0; i < _size; i++) {
      if (!_notes[i].is_active())
        continue;
      Duration time = _notes[i].current().start;
      if (time < min) {
        out = i;
        min = time;
      }
    }
    return _notes[out];
  }

  void adjust_size(uint8_t size) {
    if (size <= MAX_NOTES && size > 0 && size != _size) {
      off();
      _size = size;
    }
  }
  uint8_t active() const {
    uint8_t active = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active())
        active++;
    }
    return active;
  }
  uint8_t size() const { return _size; }
  constexpr uint8_t max_size() const { return MAX_NOTES; }
};
} // namespace teslasynth::synth
