#pragma once

#include "../midi/midi_core.hpp"
#include "../synthesizer/notes.hpp"
#include "core.hpp"
#include "instruments.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>

#ifndef CONFIG_DEFAULT_MAX_DUTY
#define CONFIG_DEFAULT_MAX_DUTY 100
#endif

namespace teslasynth::midisynth {
using namespace teslasynth::synth;
using namespace teslasynth::midi;

/**
 * A numeric value representing duty cycle.
 * Max value: 100%
 * Resolution: 0.5%
 */
class DutyCycle final {
  constexpr static auto max_duty = 100, max_value = 200;
  uint8_t value_;

  template <typename T> constexpr static uint8_t validate(T v) {
    if (v >= max_duty)
      return max_value;
    if (v <= 0)
      return 0;
    return static_cast<float>(v) / max_duty * max_value;
  }

public:
  constexpr DutyCycle() : value_(0) {}

  template <typename T>
  explicit constexpr DutyCycle(T value) : value_(validate(value)) {}

  constexpr bool is_max() const { return value_ == max_value; }
  constexpr bool is_zero() const { return value_ == 0; }
  constexpr static DutyCycle max() { return DutyCycle(max_duty); }
  constexpr static DutyCycle min() { return DutyCycle(0); }
  constexpr uint8_t value() const { return value_; }
  constexpr uint8_t inverse() const { return max_value - value_; }
  constexpr operator float() const {
    return value_ / static_cast<float>(max_value);
  }
  constexpr float percent() const { return 100 * static_cast<float>(*this); }
  inline operator std::string() const {
    return std::to_string(static_cast<float>(value_) / max_value * max_duty) +
           "%";
  }
};

struct ChannelConfig {
  static constexpr uint8_t max_notes = CONFIG_MAX_NOTES;
  static constexpr float default_max_duty = CONFIG_DEFAULT_MAX_DUTY;

  Duration16 max_on_time = 100_us, min_deadtime = 100_us, duty_window = 10_ms;
  uint8_t notes = max_notes;
  DutyCycle max_duty = DutyCycle(CONFIG_DEFAULT_MAX_DUTY);
  std::optional<uint8_t> instrument = {};

  constexpr bool operator==(const ChannelConfig &other) const {
    return max_on_time == other.max_on_time &&
           min_deadtime == other.min_deadtime &&
           duty_window == other.duty_window && notes == other.notes &&
           max_duty == other.max_duty && instrument == other.instrument;
  }

  inline operator std::string() const {
    return std::string("Concurrent notes: ") + std::to_string(notes) +
           "\nMax on time: " + std::string(max_on_time) +
           "\nMin deadtime: " + std::string(min_deadtime) +
           "\nMax duty: " + std::string(max_duty) +
           "\nDuty window: " + std::string(duty_window) +
           "\nInstrument: " + (instrument ? std::to_string(*instrument) : "-");
  }
};

struct SynthConfig {
  Hertz a440 = 440_hz;
  std::optional<uint8_t> instrument = {};

  constexpr bool operator==(const SynthConfig &other) const {
    return a440 == other.a440 && instrument == other.instrument;
  }

  inline operator std::string() const {
    return std::string("Tuning: ") + std::string(a440) +
           "\nInstrument: " + (instrument ? std::to_string(*instrument) : "-");
  }
};

template <std::uint8_t OUTPUTS = 1> class Configuration {
  SynthConfig synth_;
  std::array<ChannelConfig, OUTPUTS> channels_{};

public:
  constexpr Configuration() {}
  constexpr Configuration(
      const SynthConfig &synth_config,
      const std::array<ChannelConfig, OUTPUTS> &channel_configs)
      : synth_(synth_config), channels_(channel_configs) {}
  constexpr Configuration(const SynthConfig &synth_config)
      : synth_(synth_config) {}
  constexpr Configuration(
      const std::array<ChannelConfig, OUTPUTS> &channel_configs)
      : channels_(channel_configs) {}

  constexpr Configuration<OUTPUTS> with(const SynthConfig &synth_config) {
    return Configuration<OUTPUTS>(synth_config, channels_);
  }
  constexpr Configuration<OUTPUTS>
  with(const std::array<ChannelConfig, OUTPUTS> &channel_configs) {
    return Configuration<OUTPUTS>(synth_, channel_configs);
  }

  SynthConfig &synth() { return synth_; }
  const SynthConfig &synth() const { return synth_; }
  ChannelConfig &channel(uint8_t ch) { return channels_[ch]; }
  const ChannelConfig &channel(uint8_t ch) const { return channels_[ch]; }
  std::array<ChannelConfig, OUTPUTS> &channels() { return channels_; }
  const std::array<ChannelConfig, OUTPUTS> &channels() const {
    return channels_;
  }
  constexpr uint8_t channels_size() const { return OUTPUTS; }

  constexpr bool operator==(const Configuration<OUTPUTS> &other) const {
    return synth_ == other.synth_ && channels_ == other.channels_;
  }

  inline operator std::string() const {
    std::string res =
        std::string("Synth: ") + std::string(synth_) + "\nChannels: ";
    for (const auto &ch : channels()) {
      res += "\n" + std::string(ch);
    }
    return res;
  }
};
} // namespace teslasynth::midisynth
