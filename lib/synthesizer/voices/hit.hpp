#pragma once

#include "../percussion.hpp"
#include "../pulse.hpp"
#include "channel_state.hpp"
#include "envelope.hpp"
#include <core.hpp>
#include <core/duration.hpp>
#include <core/probability.hpp>
#include <cstdint>
#include <stddef.h>

namespace teslasynth::synth {
using namespace teslasynth::core;

class Hit {
  uint32_t rng_state;
  Duration end, now;
  Hertz prf = 0_hz;
  Probability noise_ = Probability(), skip_ = Probability();
  EnvelopeLevel volume_;
  Envelope envelope_;
  NotePulse current_;

  ChannelState const *_channel;
  inline float random();

public:
  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const Percussion &params, const ChannelState *channel = nullptr);
  bool next();
  const NotePulse &current() const { return current_; }
  bool is_active() const { return now < end; }
};

} // namespace teslasynth::synth
