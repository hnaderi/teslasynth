#pragma once

#include "../midi/midi_core.hpp"
#include "channel_state.hpp"
#include "core/envelope_level.hpp"
#include <cstddef>
#include <cstdint>

namespace teslasynth::midisynth {

template <std::uint8_t OUTPUTS = 1> class OutputNumber final {
  static_assert(OUTPUTS >= 1, "Minimum number of outputs is 1");
  uint8_t value_;

  constexpr OutputNumber(int8_t value) : value_(value) {}

public:
  OutputNumber() = delete;
  constexpr static bool is_valid(int8_t v) { return v >= 0 && v < OUTPUTS; }
  constexpr static std::optional<OutputNumber> from(uint8_t v) {
    if (is_valid(v))
      return OutputNumber(v);
    else
      return {};
  }
  constexpr operator uint8_t() const { return value_; }
};

template <std::uint8_t OUTPUTS = 1> class OutputNumberOpt final {
  static_assert(OUTPUTS >= 1, "Minimum number of outputs is 1");
  int8_t value_;

public:
  constexpr OutputNumberOpt() : value_(-1) {}
  constexpr OutputNumberOpt(int8_t value) : value_(value) {}

  constexpr bool has_value() const {
    return OutputNumber<OUTPUTS>::is_valid(value_);
  }
  constexpr operator bool() const { return has_value(); }
  constexpr uint8_t max() const { return OUTPUTS; }
  constexpr std::optional<OutputNumber<OUTPUTS>> value() const {
    return OutputNumber<OUTPUTS>::from(value_);
  }
  inline operator std::string() const {
    if (has_value())
      return std::to_string(value_);
    else
      return "x";
  }
};

template <std::uint8_t OUTPUTS = 1> class ChannelMapping final {
  typedef std::array<OutputNumberOpt<OUTPUTS>, 16> Mapping;
  Mapping data_{};

public:
  ChannelMapping() {
    for (auto i = 0; i < OUTPUTS; i++) {
      data_[i] = OutputNumberOpt<OUTPUTS>(i);
    }
  }
  ChannelMapping(const Mapping &mapping) : data_(mapping) {}
  constexpr const OutputNumberOpt<OUTPUTS> &
  operator[](midi::MidiChannelNumber ch) const {
    return data_[ch.value];
  }
  OutputNumberOpt<OUTPUTS> &operator[](midi::MidiChannelNumber ch) {
    return data_[ch.value];
  }
  constexpr const Mapping &data() const { return data_; }
  constexpr auto size() const { return data_.size(); }
  constexpr auto begin() const { return data_.begin(); }
  constexpr auto end() const { return data_.end(); }
};

class InstrumentMapping final {
  typedef std::array<uint8_t, 16> Mapping;
  Mapping data_{};

public:
  constexpr const uint8_t &operator[](midi::MidiChannelNumber ch) const {
    return data_[ch.value];
  }
  uint8_t &operator[](midi::MidiChannelNumber ch) { return data_[ch.value]; }
  constexpr const Mapping &data() const { return data_; }
};

class MidiChannels final {
  typedef std::array<synth::ChannelState, 16> Mapping;
  Mapping data_{};

public:
  constexpr const synth::ChannelState &
  operator[](midi::MidiChannelNumber ch) const {
    return data_[ch.value];
  }
  synth::ChannelState &operator[](midi::MidiChannelNumber ch) {
    return data_[ch.value];
  }
  constexpr auto size() const { return data_.size(); }
  constexpr auto begin() const { return data_.begin(); }
  constexpr auto end() const { return data_.end(); }
};

} // namespace teslasynth::midisynth
