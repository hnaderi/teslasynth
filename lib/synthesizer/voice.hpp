#pragma once

#include "notes.hpp"
#include "voice_event.hpp"
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

template <std::uint8_t MAX_NOTES = CONFIG_MAX_NOTES, class ELEMENT = VoiceEvent>
class Voice final {
  uint8_t _size = MAX_NOTES;
  std::array<ELEMENT, MAX_NOTES> _notes;
  std::array<uint8_t, MAX_NOTES> _numbers;

  uint8_t find_free(uint8_t id) {
    uint8_t idx = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] != id)
        continue;
      idx = i;
      break;
    }
    return idx;
  }

public:
  Voice() {}
  Voice(const Voice &) = delete;
  Voice(Voice &&) = delete;
  Voice &operator=(const Voice &) = delete;
  Voice &operator=(Voice &&) = delete;
  Voice(uint8_t size) : _size(std::min(size, MAX_NOTES)) {}
  ELEMENT &start(const MidiNote &mnote, Duration time,
                 const Instrument &instrument, Hertz tuning) {
    const auto idx = find_free(mnote.number);
    _notes[idx].start(mnote, time, instrument, tuning);
    _numbers[idx] = mnote.number;
    return _notes[idx];
  }

  ELEMENT &start(const MidiNote &mnote, const Percussion &params,
                 Duration time) {
    const auto id = mnote.number + 128;
    const auto idx = find_free(id);
    _notes[idx].start(mnote.velocity, params, time);
    _numbers[idx] = id;
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

  ELEMENT &next() {
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
