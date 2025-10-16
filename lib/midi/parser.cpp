#include "midi_core.hpp"
#include "midi_parser.hpp"
#include <cstddef>

MidiParser::MidiParser(ChannelMessageCallback on_channel_message)
    : _on_channel_message(on_channel_message) {}

void MidiParser::feed(const uint8_t *input, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (MidiStatus::is_status(input[i])) {
      auto status = MidiStatus(input[i]);
      if (status.is_channel()) {
        _current_status = status;
        _has_status = true;
        _waiting_for_data = true;
        _has_data = false;
      }
    } else {
      if (_has_status) {
        if (_waiting_for_data) {
          _data0 = MidiData(input[i]);
          _waiting_for_data = false;
          _has_data = true;
        } else {
          MidiData data0 = _has_data ? _data0 : MidiData(input[i]),
                   data1 = _has_data ? MidiData(input[i]) : MidiData();
          _on_channel_message({
              .type = _current_status.channel_status_type(),
              .channel = _current_status.channel(),
              .data0 = data0,
              .data1 = data1,
          });
        }
      } else {
      }
    }
  }
}
