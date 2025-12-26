#include "core.hpp"
#include "pitchbend.hpp"

namespace teslasynth::synth {
using namespace teslasynth::core;

struct ChannelState {
  PitchBend pitch_bend;
  EnvelopeLevel amplitude = core::EnvelopeLevel::max();
};

} // namespace teslasynth::synth
