#include "core.hpp"
#include "core/duration.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "notes.hpp"
#include "presets.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "voice.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <unity.h>
#include <vector>

constexpr Hertz tuning = 100_hz;
Instrument instrument{.envelope = EnvelopeLevel(1), .vibrato = Vibrato::none()};
PitchPreset preset{&instrument, tuning};

constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }
class FakeEvent {
  MidiNote mnote_;
  Duration started_, released_;
  std::optional<SoundPreset> preset_;
  bool active = false, is_released_ = false;
  NotePulse pulse;

public:
  void start(const MidiNote &mnote, Duration time, const SoundPreset &preset) {
    started_ = time;
    preset_ = preset;
    mnote_ = mnote;
    pulse.start = time;
    active = true;
  }
  void release(Duration time) {
    released_ = time;
    is_released_ = true;
  }
  void off() { active = false; }
  bool next() {
    if (!is_released_ || pulse.start < released_) {
      pulse.start += mnote_.frequency(tuning).period();
    } else {
      active = false;
    }
    return active;
  }
  const NotePulse &current() const { return pulse; }
  bool is_active() const { return active; }
  bool is_released() const { return is_released_; }

  void assert_started(const MidiNote &mnote, Duration time,
                      const SoundPreset &preset) {
    TEST_ASSERT_TRUE(active);
    TEST_ASSERT_TRUE_MESSAGE(mnote_ == mnote, "Midi notes are not equal");
    assert_duration_equal(started_, time);
    TEST_ASSERT_TRUE(preset_.has_value());
    TEST_ASSERT_TRUE_MESSAGE(*preset_ == preset, "SoundPresets are not equal");
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

void assert_note(TestVoice &voice, const MidiNote &mnote,
                 const Duration &time) {
  auto &evt = voice.start(mnote, time, preset);

  TEST_ASSERT_TRUE(evt.is_active());
  evt.assert_started(mnote, time, preset);
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
  evt.assert_started(mnotef(2), 50_us, preset);
  assert_duration_equal(evt.current().start, 50_us);
}

void test_should_return_the_note_with_least_time_after_tick(void) {
  TestVoice voice(4);
  assert_note(voice, mnotef(1), 200_us);
  assert_note(voice, mnotef(2), 50_us);
  assert_note(voice, mnotef(3), 100_us);
  auto &note1 = voice.next();
  note1.assert_started(mnotef(2), 50_us, preset);

  TEST_ASSERT_TRUE(note1.next());
  TEST_ASSERT_TRUE(note1.current().start > 200_us);

  auto &note2 = voice.next();
  note2.assert_started(mnotef(3), 100_us, preset);

  TEST_ASSERT_TRUE(note2.next());
  TEST_ASSERT_TRUE(note2.current().start > 200_us);

  auto &note3 = voice.next();
  note3.assert_started(mnotef(1), 200_us, preset);
}

void test_should_release_note(void) {
  TestVoice voice;
  auto mnote = mnotef(1);
  assert_note(voice, mnote, 200_ms);
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnote.number, 3000_ms);
  note = voice.next();
  note.assert_released(3000_ms);
}

void test_should_not_release_other_voice(void) {
  TestVoice voice;
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  auto &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnotef(1).number, 3000_us);
  TEST_ASSERT_FALSE(note.is_released());
}

void test_should_allow_the_minimum_size_of_one(void) {
  TestVoice voice(1);
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(voice.active(), 1);
  TEST_ASSERT_EQUAL(voice.size(), 1);

  auto &note = voice.next();
  note.assert_started(mnotef(1), 200_ms, preset);
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
  auto *note1 = &voice.start(mnotef(1), 200_us, preset),
       *note2 = &voice.start(mnotef(2), 1_s, preset),
       *note3 = &voice.start(mnotef(3), 2_s, preset);
  voice.release(mnotef(1), 1_s);
  voice.release(mnotef(2), 2_s);
  voice.release(mnotef(3), 3_s);

  TEST_ASSERT_EQUAL(3, voice.active());
  auto *note = &voice.next();
  TEST_ASSERT_EQUAL(note1, note);

  while (note->is_active()) {
    note->assert_started(mnotef(1), 200_us, preset);
    note->next();
  }
  TEST_ASSERT_FALSE(note1->is_active());

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note2, note);
  TEST_ASSERT_EQUAL(2, voice.active());
  while (note->is_active()) {
    note->assert_started(mnotef(2), 1_s, preset);
    note->next();
  }

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note3, note);
  TEST_ASSERT_EQUAL(1, voice.active());
  while (note->is_active()) {
    note->assert_started(mnotef(3), 2_s, preset);
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

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
