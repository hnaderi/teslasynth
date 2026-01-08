#pragma once

#include "core/envelope_level.hpp"
#include "presets.hpp"
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
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] != id)
        continue;
      return i;
    }

    EnvelopeLevel quietest = EnvelopeLevel::max();
    uint8_t quietest_idx = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].current().volume < quietest)
        quietest_idx = i;
    }
    return quietest_idx;
  }

public:
  Voice() {}
  Voice(const Voice &) = delete;
  Voice(Voice &&) = delete;
  Voice &operator=(const Voice &) = delete;
  Voice &operator=(Voice &&) = delete;
  Voice(uint8_t size) : _size(std::min(size, MAX_NOTES)) {}

  ELEMENT &start(uint8_t number, EnvelopeLevel amplitude, Duration time,
                 const SoundPreset &preset,
                 const ChannelState *channel = nullptr) {
    const auto idx = find_free(number);
    _notes[idx].start(number, amplitude, time, preset, channel);
    _numbers[idx] = number;
    return _notes[idx];
  }

  void release(uint8_t number, Duration time) {
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] == number) {
        _notes[i].release(time);
      }
    }
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
