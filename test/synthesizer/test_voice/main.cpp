#include "channel_state.hpp"
#include "core/duration.hpp"
#include "core/envelope_level.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "presets.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "voice.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <unity.h>
#include <vector>

using namespace teslasynth::core;
using namespace teslasynth::synth;

constexpr Hertz tuning = 100_hz;
Instrument instrument{.envelope = EnvelopeLevel(1), .vibrato = Vibrato::none()};
PitchPreset preset{&instrument, tuning};

constexpr uint8_t mnotef(int i) { return static_cast<uint8_t>(69 + i); }
class FakeEvent {
  uint8_t number_;
  EnvelopeLevel amplitude_;
  Duration started_, released_;
  std::optional<SoundPreset> preset_;
  bool active = false, is_released_ = false;
  NotePulse pulse;
  ChannelState const *_channel;

public:
  void start(uint8_t number, EnvelopeLevel amplitude, Duration time,
             const SoundPreset &preset, const ChannelState *channel = nullptr) {
    started_ = time;
    preset_ = preset;
    number_ = number;
    amplitude_ = amplitude;
    pulse.start = time;
    active = true;
    _channel = channel;
  }
  void release(Duration time) {
    released_ = time;
    is_released_ = true;
  }
  void off() { active = false; }
  bool next() {
    if (!is_released_ || pulse.start < released_) {
      pulse.start += Note::frequency_for(number_, tuning).period();
    } else {
      active = false;
    }
    return active;
  }
  const NotePulse &current() const { return pulse; }
  bool is_active() const { return active; }
  bool is_released() const { return is_released_; }

  void assert_started(uint8_t number, EnvelopeLevel amplitude, Duration time,
                      const SoundPreset &preset) {
    TEST_ASSERT_TRUE(active);
    TEST_ASSERT_EQUAL_MESSAGE(number_, number, "Midi notes are not equal");
    assert_level_equal(amplitude_, amplitude);
    assert_duration_equal(started_, time);
    TEST_ASSERT_TRUE(preset_.has_value());
    TEST_ASSERT_TRUE_MESSAGE(*preset_ == preset, "SoundPresets are not equal");
  }

  void assert_started_with_channel_state(const ChannelState *state) {
    TEST_ASSERT_EQUAL_MESSAGE(_channel, state, "Channel state is not the same");
  }

  void assert_released(Duration time) {
    TEST_ASSERT_TRUE(is_released_);
    assert_duration_equal(released_, time);
  }
};

typedef Voice<4, FakeEvent> TestVoice;

void test_empty(void) {
  Voice<> voice;
  TEST_ASSERT_EQUAL(0, voice.active());

  auto &evt = voice.next();
  TEST_ASSERT_FALSE(evt.is_active());
  TEST_ASSERT_FALSE(evt.next());
}

void assert_note(TestVoice &voice, const uint8_t number, const Duration &time) {
  EnvelopeLevel amp = EnvelopeLevel::max();
  auto &evt = voice.start(number, amp, time, preset);

  TEST_ASSERT_TRUE(evt.is_active());
  evt.assert_started(number, amp, time, preset);
}

void test_start(void) {
  TestVoice voice;
  for (size_t i = 0; i < voice.max_size(); i++) {
    auto mnote = mnotef(i);
    Duration time = 1000_us * static_cast<int>(i);
    assert_note(voice, mnote, time);
    TEST_ASSERT_EQUAL(i + 1, voice.active());
  }
}

void test_should_limit_concurrent_voice(void) {
  for (size_t max = 1; max < 5; max++) {
    TestVoice voice(max);
    for (uint8_t i = 0; i < max; i++) {
      assert_note(voice, mnotef(i), 100_us * i);
      TEST_ASSERT_EQUAL(i + 1, voice.active());
    }
    TEST_ASSERT_EQUAL(max, voice.active());
    for (uint8_t i = 5; i < max; i++) {
      assert_note(voice, mnotef(i), 100_us * i);
      TEST_ASSERT_EQUAL(max, voice.active());
    }
  }
}

void test_should_restart_the_same_note(void) {
  TestVoice voice(2);
  assert_note(voice, mnotef(0), 200_us);
  assert_note(voice, mnotef(0), 100_us);
  TEST_ASSERT_EQUAL(1, voice.active());
  auto note = voice.next();
  assert_duration_equal(note.current().start, 100_us);
}

void test_should_return_the_note_with_least_time(void) {
  TestVoice voice;
  assert_note(voice, mnotef(1), 200_us);
  assert_note(voice, mnotef(2), 50_us);
  assert_note(voice, mnotef(3), 100_us);
  auto &evt = voice.next();
  evt.assert_started(mnotef(2), EnvelopeLevel::max(), 50_us, preset);
  assert_duration_equal(evt.current().start, 50_us);
}

void test_should_return_the_note_with_least_time_after_tick(void) {
  TestVoice voice(4);
  assert_note(voice, mnotef(1), 200_us);
  assert_note(voice, mnotef(2), 50_us);
  assert_note(voice, mnotef(3), 100_us);
  auto &note1 = voice.next();
  note1.assert_started(mnotef(2), EnvelopeLevel::max(), 50_us, preset);

  TEST_ASSERT_TRUE(note1.next());
  TEST_ASSERT_TRUE(note1.current().start > 200_us);

  auto &note2 = voice.next();
  note2.assert_started(mnotef(3), EnvelopeLevel::max(), 100_us, preset);

  TEST_ASSERT_TRUE(note2.next());
  TEST_ASSERT_TRUE(note2.current().start > 200_us);

  auto &note3 = voice.next();
  note3.assert_started(mnotef(1), EnvelopeLevel::max(), 200_us, preset);
}

void test_should_release_note(void) {
  TestVoice voice;
  auto mnote = mnotef(1);
  assert_note(voice, mnote, 200_ms);
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnote, 3000_ms);
  note = voice.next();
  note.assert_released(3000_ms);
}

void test_should_not_release_other_voice(void) {
  TestVoice voice;
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnotef(1), 3000_us);
  TEST_ASSERT_FALSE(note.is_released());
}

void test_should_allow_the_minimum_size_of_one(void) {
  TestVoice voice(1);
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(voice.active(), 1);
  TEST_ASSERT_EQUAL(voice.size(), 1);

  auto &note = voice.next();
  note.assert_started(mnotef(1), EnvelopeLevel::max(), 200_ms, preset);
}

void test_off(void) {
  TestVoice voice;
  assert_note(voice, mnotef(0), 200_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(2, voice.active());
  voice.off();
  TEST_ASSERT_EQUAL(0, voice.active());
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_active());
}

void test_should_return_the_note_with_least_time2(void) {
  TestVoice voice;
  auto *note1 = &voice.start(mnotef(1), EnvelopeLevel::max(), 200_us, preset),
       *note2 = &voice.start(mnotef(2), EnvelopeLevel::max(), 1_s, preset),
       *note3 = &voice.start(mnotef(3), EnvelopeLevel::max(), 2_s, preset);
  voice.release(mnotef(1), 1_s);
  voice.release(mnotef(2), 2_s);
  voice.release(mnotef(3), 3_s);

  TEST_ASSERT_EQUAL(3, voice.active());
  auto *note = &voice.next();
  TEST_ASSERT_EQUAL(note1, note);

  while (note->is_active()) {
    note->assert_started(mnotef(1), EnvelopeLevel::max(), 200_us, preset);
    note->next();
  }
  TEST_ASSERT_FALSE(note1->is_active());

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note2, note);
  TEST_ASSERT_EQUAL(2, voice.active());
  while (note->is_active()) {
    note->assert_started(mnotef(2), EnvelopeLevel::max(), 1_s, preset);
    note->next();
  }

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note3, note);
  TEST_ASSERT_EQUAL(1, voice.active());
  while (note->is_active()) {
    note->assert_started(mnotef(3), EnvelopeLevel::max(), 2_s, preset);
    note->next();
  }
  TEST_ASSERT_EQUAL(0, voice.active());
}

void test_adjust_size(void) {
  TestVoice voice(3);
  assert_note(voice, mnotef(0), 200_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(2, voice.active());
  TEST_ASSERT_EQUAL(3, voice.size());
  voice.adjust_size(2);
  TEST_ASSERT_EQUAL(0, voice.active());
  TEST_ASSERT_EQUAL(2, voice.size());
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_active());

  assert_note(voice, mnotef(0), 200_ms);
  TEST_ASSERT_EQUAL(1, voice.active());
  voice.adjust_size(2);
  TEST_ASSERT_EQUAL(1, voice.active());
}

void test_should_pass_channel_state(void) {
  TestVoice voice;
  ChannelState state;

  auto &evt =
      voice.start(mnotef(1), EnvelopeLevel::max(), 200_us, preset, &state);

  evt.assert_started_with_channel_state(&state);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start);
  RUN_TEST(test_should_limit_concurrent_voice);
  RUN_TEST(test_should_restart_the_same_note);
  RUN_TEST(test_should_return_the_note_with_least_time);
  RUN_TEST(test_should_return_the_note_with_least_time_after_tick);
  RUN_TEST(test_should_release_note);
  RUN_TEST(test_should_not_release_other_voice);
  RUN_TEST(test_should_allow_the_minimum_size_of_one);
  RUN_TEST(test_off);
  RUN_TEST(test_should_return_the_note_with_least_time2);
  RUN_TEST(test_adjust_size);
  RUN_TEST(test_should_pass_channel_state);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
