// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "channel_state.hpp"
#include "core/envelope_level.hpp"
#include "bank/instruments.hpp"
#include "lfo.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include "pitchbend.hpp"
#include "presets.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "unity_internals.h"
#include <cstdint>
#include <unity.h>
#include <vector>

using namespace teslasynth::midisynth;

constexpr uint8_t mnotef(int i) {
  return static_cast<uint8_t>(69 + i);
}
constexpr Instrument instrument(int i) {
  return {.envelope = EnvelopeLevel(i * 0.1), .vibrato = Vibrato::none()};
}

class FakeNotes {
  Note note;

public:
  struct Started {
    uint8_t number;
    EnvelopeLevel amplitude;
    Duration time;
    const SoundPreset preset;
    const ChannelState *state;

    bool is_pitch() const { return std::holds_alternative<PitchPreset>(preset); }
    bool is_perc() const { return std::holds_alternative<PercussivePreset>(preset); }
    const PitchPreset &as_pitch() const { return std::get<PitchPreset>(preset); }

    const PercussivePreset &as_perc() const { return std::get<PercussivePreset>(preset); }

    void assert_instrument(const Instrument &expectedInst //, Hertz expectedTuning,
    ) const {
      TEST_ASSERT_TRUE_MESSAGE(is_pitch(), "Expected PitchPreset but got PercussivePreset");
      const auto &p = as_pitch();
      assert_instrument_equal(expectedInst, *p.instrument);
    }
    void assert_percussion(const Percussion &expectedPerc) const {
      TEST_ASSERT_TRUE_MESSAGE(is_perc(), "Expected PercussivePreset but got PitchPreset");
      const auto &p = as_perc();
      TEST_ASSERT_TRUE_MESSAGE(expectedPerc == *p.percussion, "Percussion pointer mismatch");
    }
  };

  struct Released {
    uint8_t number;
    Duration time;
  };

  struct Off {};

private:
  std::vector<Started> started_;
  std::vector<Released> released_;
  std::vector<Off> offs_;
  std::vector<uint8_t> adjusts_;

public:
  Note &start(uint8_t number, EnvelopeLevel amplitude, Duration time, const SoundPreset &preset,
              const ChannelState *state = nullptr) {
    started_.push_back({number, amplitude, time, preset, state});
    return note;
  }

  void release(uint8_t number, Duration time) { released_.push_back({number, time}); }
  void off() { offs_.push_back({}); }

  void adjust_size(uint8_t size) { adjusts_.push_back(size); }

  const std::vector<Started> started() const { return started_; }
  const std::vector<Released> released() const { return released_; }
  const std::vector<Off> turned_off() const { return offs_; }
  const std::vector<uint8_t> adjusted() const { return adjusts_; }
};

void test_note_pulse_empty(void) {
  Teslasynth<> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  TEST_ASSERT_FALSE(tsynth.track().is_playing());
}

void test_boundaries(void) {
  Teslasynth<> tsynth;
  tsynth.note_on(10, 70, 127, Duration::zero());
  tsynth.note_off(10, 70, Duration::zero());
  tsynth.change_instrument(10, 100);
}

void test_should_handle_note_on(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &notes = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const Duration now = 10_ms * i;
    const auto velocity = 10 * (i + 1);
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, velocity), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_duration_equal(notes.started().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.started().back().number);
    assert_level_equal(notes.started().back().amplitude, EnvelopeLevel::logscale(2 * velocity + 1));
    notes.started().back().assert_instrument(default_instrument());
  }
}

void test_should_handle_note_off(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    const auto velocity = 10 * (i + 1);
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, velocity), now);
    tsynth.handle(MidiChannelMessage::note_off(0, 69 + i, velocity), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.released().size());
    assert_duration_equal(voice.released().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, voice.released().back().number);
  }
}

void test_should_handle_note_on_velocity_zero(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    const auto velocity = 10 * (i + 1);
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, velocity), now);
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 0), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.released().size());
    assert_duration_equal(voice.released().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, voice.released().back().number);
  }
}

void test_should_ignore_note_off_when_not_playing(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    tsynth.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_FALSE(track.is_playing());
    TEST_ASSERT_EQUAL(0, voice.released().size());
  }
}

void test_should_handle_instrument_change(void) {
  constexpr auto N = 10;
  std::array<Instrument, N> instruments;
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
  }
  Teslasynth<1, FakeNotes> tsynth;
  tsynth.use_instruments(instruments);
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();

  tsynth.handle(MidiChannelMessage::program_change(0, 0), 10_ms);
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  TEST_ASSERT_FALSE(track.is_playing());

  for (auto i = 0; i < N; i++) {
    tsynth.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(i, tsynth.instrument_number(0));
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * (i + 1)), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.started().size());
    voice.started().back().assert_instrument(instrument(i));
  }
}

void test_should_ignore_instrument_change_when_config_has_instrument(void) {
  constexpr auto N = 10;
  std::array<Instrument, N> instruments;
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
  }

  Configuration<> config(SynthConfig{.instrument = 2});
  Teslasynth<1, FakeNotes> tsynth(config);
  tsynth.use_instruments(instruments);
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();

  for (auto i = 0; i < N; i++) {
    const auto velocity = 10 * (i + 1);
    tsynth.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(2, tsynth.instrument_number(0));
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, velocity), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.started().size());
    voice.started().back().assert_instrument(instrument(2));
  }
}

void test_config_instrument_overrides_runtime_instrument(void) {
  Teslasynth<1, FakeNotes> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));

  tsynth.configuration().synth().instrument = 2;
  TEST_ASSERT_EQUAL(2, tsynth.instrument_number(0));

  tsynth.configuration().channels()[0].instrument = 3;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(0));

  tsynth.configuration().synth().instrument = 4;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(0));
}

void test_instrument_override_follows_the_routed_output(void) {
  Teslasynth<2, FakeNotes> tsynth;
  auto &config = tsynth.configuration();
  config.channels()[1].instrument = 3;

  config.routing().mapping[11] = 1;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(11));

  config.routing().mapping[7] = 1;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(7));

  config.routing().mapping[9] = 0;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(9));
}

void test_instrument_override_ignored_when_channel_is_unrouted(void) {
  Teslasynth<2, FakeNotes> tsynth;
  auto &config = tsynth.configuration();
  config.channels()[1].instrument = 3;
  config.synth().instrument = 5;

  config.routing().mapping[11] = -1;
  TEST_ASSERT_EQUAL(5, tsynth.instrument_number(11));
}

void test_default_config_is_valid(void) {
  TEST_ASSERT_TRUE(Configuration<1>().is_valid());
  TEST_ASSERT_TRUE(Configuration<4>().is_valid());
  TEST_ASSERT_TRUE(Configuration<8>().is_valid());
}

void test_config_validation_rejects_corrupt_values(void) {
  {
    Configuration<2> config;
    config.channels()[1].notes = ChannelConfig::max_notes + 1;
    TEST_ASSERT_FALSE(config.is_valid());
  }
  {
    Configuration<2> config;
    config.channels()[1].notes = 0;
    TEST_ASSERT_FALSE(config.is_valid());
  }
  {
    Configuration<2> config;
    config.synth().tuning = Hertz(0);
    TEST_ASSERT_FALSE(config.is_valid());
  }
  {
    Configuration<2> config;
    config.synth().tuning = Hertz(-440);
    TEST_ASSERT_FALSE(config.is_valid());
  }
}

void test_config_validation_accepts_every_settable_value(void) {
  Configuration<2> config;
  for (uint8_t n = 1; n <= ChannelConfig::max_notes; n++) {
    config.channels()[0].notes = n;
    TEST_ASSERT_TRUE(config.is_valid());
  }
  for (float duty = 0.5; duty <= 100; duty += 0.5) {
    config.channels()[0].max_duty = DutyCycle(duty);
    TEST_ASSERT_TRUE(config.is_valid());
  }
}

void test_non_existing_instrument_number_falls_back_to_default(void) {
  Teslasynth<1, FakeNotes> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  tsynth.configuration().synth().instrument = 200;
  assert_instrument_equal(tsynth.instrument(0), default_instrument());
}

void test_should_turnoff_when_needed(void) {
  const std::vector<ControlChange> cc_event_types{
      ControlChange::ALL_SOUND_OFF,
      ControlChange::ALL_NOTES_OFF,
      ControlChange::RESET_ALL_CONTROLLERS,
  };
  for (auto cc : cc_event_types) {
    for (auto ch = 0; ch < 16; ch++) {
      Teslasynth<1, FakeNotes> tsynth;
      auto &track = tsynth.track();
      auto &voice = tsynth.voice();
      TEST_ASSERT_EQUAL(0, voice.turned_off().size());
      tsynth.handle(MidiChannelMessage::control_change(ch, cc, 0), 10_ms);
      TEST_ASSERT_EQUAL(1, voice.turned_off().size());
      TEST_ASSERT_FALSE(track.is_playing());
    }
  }
}

void test_should_start_playing_the_first_note_on_message(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  TEST_ASSERT_FALSE(track.is_playing());
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 100_s);

  TEST_ASSERT_TRUE(track.is_playing());
  TEST_ASSERT_EQUAL(1, voice.started().size());
  assert_duration_equal(voice.started().back().time, Duration::zero());
  TEST_ASSERT_EQUAL(69, voice.started().back().number);
  assert_level_equal(voice.started().back().amplitude, EnvelopeLevel::max());
  voice.started().back().assert_instrument(default_instrument());
}

void test_should_ignore_off_messages_when_not_playing(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  TEST_ASSERT_FALSE(track.is_playing());
  tsynth.handle(MidiChannelMessage::note_off(0, 69, 127), 100_s);
  TEST_ASSERT_FALSE(track.is_playing());
  TEST_ASSERT_EQUAL(0, voice.started().size());
}

void test_should_adjust_note_sizes(void) {
  std::array<ChannelConfig, 1> configs = {ChannelConfig{.notes = 2}};
  Configuration<> conf({}, configs);
  Teslasynth<1, FakeNotes> tsynth(conf);
  auto &voice = tsynth.voice();

  TEST_ASSERT_EQUAL(1, voice.adjusted().size());
  TEST_ASSERT_EQUAL(2, voice.adjusted().back());
}

void test_reload_config_should_adjust_note_sizes(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().channels()[0].notes = 2;
  tsynth.reload_config();

  TEST_ASSERT_EQUAL(2, voice.adjusted().size());
  TEST_ASSERT_EQUAL(2, voice.adjusted().back());
}

void test_should_handle_channel_volume(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();

  for (auto ch = 0; ch < 16; ch++) {
    // Route channel to the only output we have here
    tsynth.configuration().routing().mapping[ch] = 0;

    auto msg = MidiChannelMessage::control_change(ch, ControlChange::CHANNEL_VOLUME_MSB, 8 * ch);
    tsynth.handle(msg, 0_s);
    tsynth.handle(MidiChannelMessage::note_on(ch, 69, 127), 0_ms);

    TEST_ASSERT_EQUAL(ch + 1, voice.started().size());
    auto channel_state = voice.started().back().state;
    TEST_ASSERT_NOT_NULL(channel_state);
    assert_level_equal(channel_state->amplitude, EnvelopeLevel(8 * ch / 127.f));
  }
}

void test_should_handle_pitch_bend(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();

  for (auto ch = 0; ch < 16; ch++) {
    // Route channel to the only output we have here
    tsynth.configuration().routing().mapping[ch] = 0;

    auto msg = MidiChannelMessage::pitchbend(ch, 1000 * ch);
    tsynth.handle(msg, 0_s);
    tsynth.handle(MidiChannelMessage::note_on(ch, 69, 127), 0_ms);

    TEST_ASSERT_EQUAL(ch + 1, voice.started().size());
    auto channel_state = voice.started().back().state;
    TEST_ASSERT_NOT_NULL(channel_state);
    TEST_ASSERT_TRUE(channel_state->pitch_bend == PitchBend::midi(ch * 1000));
  }
}

void test_reset_all_controllers_clears_pitch_bend(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().routing().mapping[0] = 0;

  tsynth.handle(MidiChannelMessage::pitchbend(0, 1000), 0_s);
  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::RESET_ALL_CONTROLLERS, 0),
                0_ms);
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 1_ms);

  TEST_ASSERT_EQUAL(1, voice.started().size());
  auto *state = voice.started().back().state;
  TEST_ASSERT_NOT_NULL(state);
  TEST_ASSERT_TRUE(state->pitch_bend == PitchBend());
}

void test_reset_all_controllers_clears_channel_volume(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().routing().mapping[0] = 0;

  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::CHANNEL_VOLUME_MSB, 64), 0_ms);
  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::RESET_ALL_CONTROLLERS, 0),
                0_ms);
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 1_ms);

  TEST_ASSERT_EQUAL(1, voice.started().size());
  auto *state = voice.started().back().state;
  TEST_ASSERT_NOT_NULL(state);
  assert_level_equal(state->amplitude, EnvelopeLevel::max());
}

void test_reset_all_controllers_only_affects_targeted_channel(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().routing().mapping[0] = 0;
  tsynth.configuration().routing().mapping[1] = 0;

  tsynth.handle(MidiChannelMessage::pitchbend(0, 1000), 0_s);
  tsynth.handle(MidiChannelMessage::pitchbend(1, 2000), 0_s);
  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::RESET_ALL_CONTROLLERS, 0),
                0_ms);
  tsynth.handle(MidiChannelMessage::note_on(1, 69, 127), 1_ms);

  TEST_ASSERT_EQUAL(1, voice.started().size());
  auto *state = voice.started().back().state;
  TEST_ASSERT_NOT_NULL(state);
  TEST_ASSERT_TRUE(state->pitch_bend == PitchBend::midi(2000));
}

void test_all_sound_off_preserves_channel_state(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().routing().mapping[0] = 0;

  tsynth.handle(MidiChannelMessage::pitchbend(0, 1000), 0_s);
  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::ALL_SOUND_OFF, 0), 0_ms);
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 1_ms);

  TEST_ASSERT_EQUAL(1, voice.started().size());
  auto *state = voice.started().back().state;
  TEST_ASSERT_NOT_NULL(state);
  TEST_ASSERT_TRUE(state->pitch_bend == PitchBend::midi(1000));
}

void test_all_notes_off_preserves_channel_state(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().routing().mapping[0] = 0;

  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::CHANNEL_VOLUME_MSB, 64), 0_ms);
  tsynth.handle(MidiChannelMessage::control_change(0, ControlChange::ALL_NOTES_OFF, 0), 0_ms);
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 1_ms);

  TEST_ASSERT_EQUAL(1, voice.started().size());
  auto *state = voice.started().back().state;
  TEST_ASSERT_NOT_NULL(state);
  assert_level_equal(state->amplitude, EnvelopeLevel(64 / 127.f));
}

void test_sample_all_should_write_each_output_to_its_own_buffer_slot(void) {
  // Regression: `start = ch * BUFSIZE` was a uint8_t in sample_all, so for
  // OUTPUTS=8, BUFSIZE=200 (the Python-binding configuration) ch * BUFSIZE
  // overflowed for ch >= 2.
  // Writes for those channels landed in the wrong slice of the buffer,
  // while at(ch, i) (size_t arithmetic) read the never-written region,
  // yielding default {on=0, off=0} pulses.
  Teslasynth<8> tsynth;
  PulseBuffer<8, 200> buf;

  // Default routing maps MIDI ch N -> output N for N=0..7. Trigger every output.
  for (uint8_t ch = 0; ch < 8; ch++) {
    tsynth.handle(MidiChannelMessage::note_on(ch, 69, 127), Duration::zero());
  }

  tsynth.sample_all(10_ms, buf);

  for (uint8_t ch = 0; ch < 8; ch++) {
    TEST_ASSERT_GREATER_THAN_UINT8(0, buf.written[ch]);
    TEST_ASSERT_FALSE_MESSAGE(buf.at(ch, 0).is_zero(),
                              "first pulse should be a synthesised pulse, not "
                              "default-initialised");
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_note_pulse_empty);
  RUN_TEST(test_boundaries);
  RUN_TEST(test_should_handle_note_on);
  RUN_TEST(test_should_handle_note_off);
  RUN_TEST(test_should_handle_note_on_velocity_zero);
  RUN_TEST(test_should_ignore_note_off_when_not_playing);
  RUN_TEST(test_should_handle_instrument_change);
  RUN_TEST(test_should_ignore_instrument_change_when_config_has_instrument);
  RUN_TEST(test_config_instrument_overrides_runtime_instrument);
  RUN_TEST(test_instrument_override_follows_the_routed_output);
  RUN_TEST(test_instrument_override_ignored_when_channel_is_unrouted);
  RUN_TEST(test_default_config_is_valid);
  RUN_TEST(test_config_validation_rejects_corrupt_values);
  RUN_TEST(test_config_validation_accepts_every_settable_value);
  RUN_TEST(test_non_existing_instrument_number_falls_back_to_default);
  RUN_TEST(test_should_turnoff_when_needed);
  RUN_TEST(test_should_start_playing_the_first_note_on_message);
  RUN_TEST(test_should_ignore_off_messages_when_not_playing);
  RUN_TEST(test_should_adjust_note_sizes);
  RUN_TEST(test_reload_config_should_adjust_note_sizes);
  RUN_TEST(test_should_handle_channel_volume);
  RUN_TEST(test_should_handle_pitch_bend);
  RUN_TEST(test_reset_all_controllers_clears_pitch_bend);
  RUN_TEST(test_reset_all_controllers_clears_channel_volume);
  RUN_TEST(test_reset_all_controllers_only_affects_targeted_channel);
  RUN_TEST(test_all_sound_off_preserves_channel_state);
  RUN_TEST(test_all_notes_off_preserves_channel_state);
  RUN_TEST(test_sample_all_should_write_each_output_to_its_own_buffer_slot);

  UNITY_END();
}
int main(int argc, char **argv) {
  app_main();
}
