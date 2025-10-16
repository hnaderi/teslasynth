#pragma once

#include "midi_core.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>

using ChannelMessageCallback = std::function<void(const MidiChannelMessage &)>;

class MidiParser {
  MidiStatus _current_status = 0;
  MidiData _data0;
  bool _has_status = false, _waiting_for_data = false, _has_data = false;
  ChannelMessageCallback _on_channel_message;

public:
  MidiParser(ChannelMessageCallback on_channel_message);
  void feed(const uint8_t *input, size_t len);
  MidiStatus status() const { return _current_status; }
  bool has_status() const { return _has_status; }
};
