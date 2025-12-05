#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unity.h>

constexpr Hertz tuning = 100_hz;
Instrument instrument{.envelope = ADSR::constant(EnvelopeLevel(1)),
                      .vibrato = Vibrato::none()};
constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }

void test_empty(void) {
  Voice<> voice;
  TEST_ASSERT_EQUAL(0, voice.active());

  Note &note = voice.next();
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.next());
}

void assert_note(Voice<> &voice, const MidiNote &mnote, const Duration &time) {
  Note &note = voice.start(mnote, time, instrument, tuning);

  TEST_ASSERT_TRUE(note.is_active());
  assert_hertz_equal(note.frequency(), mnote.frequency(tuning));
  assert_duration_equal(time, note.current().start);
}

void test_start(void) {
  Voice<> voice;
  for (size_t i = 0; i < voice.max_size(); i++) {
    auto mnote = mnotef(i);
    Duration time = 1000_us * static_cast<int>(i);
    assert_note(voice, mnote, time);
    TEST_ASSERT_EQUAL(i + 1, voice.active());
  }
}

void test_should_limit_concurrent_voice(void) {
  for (size_t max = 1; max < 5; max++) {
    Voice<> voice(max);
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
  Voice<> voice(2);
  assert_note(voice, mnotef(0), 200_us);
  assert_note(voice, mnotef(0), 100_us);
  TEST_ASSERT_EQUAL(1, voice.active());
  auto note = voice.next();
  assert_duration_equal(note.current().start, 100_us);
}

void test_should_return_the_note_with_least_time(void) {
  Voice<> voice;
  assert_note(voice, mnotef(1), 200_us);
  assert_note(voice, mnotef(2), 50_us);
  assert_note(voice, mnotef(3), 100_us);
  Note &note = voice.next();
  assert_hertz_equal(note.frequency(), mnotef(2).frequency(tuning));
  assert_duration_equal(note.current().start, 50_us);
}

void test_should_return_the_note_with_least_time_after_tick(void) {
  Voice<> voice(4);
  assert_note(voice, mnotef(1), 200_us);
  assert_note(voice, mnotef(2), 50_us);
  assert_note(voice, mnotef(3), 100_us);
  Note &note1 = voice.next();
  assert_hertz_equal(note1.frequency(), mnotef(2).frequency(tuning));
  assert_duration_equal(note1.current().start, 50_us);
  TEST_ASSERT_TRUE(note1.now() > 200_us);

  TEST_ASSERT_TRUE(note1.next());
  TEST_ASSERT_TRUE(note1.current().start > 200_us);

  Note &note2 = voice.next();
  assert_hertz_equal(note2.frequency(), mnotef(3).frequency(tuning));
  assert_duration_equal(note2.current().start, 100_us);
  TEST_ASSERT_TRUE(note2.now() > 200_us);

  TEST_ASSERT_TRUE(note2.next());
  TEST_ASSERT_TRUE(note2.current().start > 200_us);

  Note &note3 = voice.next();
  assert_duration_equal(note3.current().start, 200_us);
  assert_hertz_equal(note3.frequency(), mnotef(1).frequency(tuning));
}

void test_should_release_note(void) {
  Voice<> voice;
  auto mnote = mnotef(1);
  assert_note(voice, mnote, 200_ms);
  Note &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnote.number, 3000_ms);
  note = voice.next();
  TEST_ASSERT_TRUE(note.is_released());
}

void test_should_release_note_on_start_with_zero_velocity(void) {
  Voice<> voice;
  auto mnote = mnotef(1);
  assert_note(voice, mnote, 200_ms);
  Note &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.start({mnote.number, 0}, 3000_ms, instrument, tuning);
  note = voice.next();
  TEST_ASSERT_TRUE(note.is_released());
}

void test_should_not_release_other_voice(void) {
  Voice<> voice;
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  Note &note = voice.next();
  TEST_ASSERT_FALSE(note.is_released());
  voice.release(mnotef(1).number, 3000_us);
  TEST_ASSERT_FALSE(note.is_released());
}

void test_should_allow_the_minimum_size_of_one(void) {
  Voice<> voice(1);
  assert_note(voice, mnotef(0), 100_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(voice.active(), 1);
  TEST_ASSERT_EQUAL(voice.size(), 1);

  Note &note = voice.next();
  assert_hertz_equal(note.frequency(), mnotef(1).frequency(tuning));
}

void test_off(void) {
  Voice<> voice;
  assert_note(voice, mnotef(0), 200_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(2, voice.active());
  voice.off();
  TEST_ASSERT_EQUAL(0, voice.active());
  Note &note = voice.next();
  TEST_ASSERT_FALSE(note.is_active());
}

void test_should_return_the_note_with_least_time2(void) {
  Voice<> voice;
  Note *note1 = &voice.start(mnotef(1), 200_us, instrument, tuning),
       *note2 = &voice.start(mnotef(2), 1_s, instrument, tuning),
       *note3 = &voice.start(mnotef(3), 2_s, instrument, tuning);
  voice.release(mnotef(1), 1_s);
  voice.release(mnotef(2), 2_s);
  voice.release(mnotef(3), 3_s);

  TEST_ASSERT_EQUAL(3, voice.active());
  Note *note = &voice.next();
  TEST_ASSERT_EQUAL(note1, note);

  while (note->is_active()) {
    assert_hertz_equal(note->frequency(), mnotef(1).frequency(tuning));
    note->next();
  }
  TEST_ASSERT_FALSE(note1->is_active());

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note2, note);
  TEST_ASSERT_EQUAL(2, voice.active());
  while (note->is_active()) {
    assert_hertz_equal(note->frequency(), mnotef(2).frequency(tuning));
    note->next();
  }

  note->next();
  note = &voice.next();
  TEST_ASSERT_EQUAL(note3, note);
  TEST_ASSERT_EQUAL(1, voice.active());
  while (note->is_active()) {
    assert_hertz_equal(note->frequency(), mnotef(3).frequency(tuning));
    note->next();
  }
  TEST_ASSERT_EQUAL(0, voice.active());
}

void test_adjust_size(void) {
  Voice<4> voice(3);
  assert_note(voice, mnotef(0), 200_ms);
  assert_note(voice, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(2, voice.active());
  TEST_ASSERT_EQUAL(3, voice.size());
  voice.adjust_size(2);
  TEST_ASSERT_EQUAL(0, voice.active());
  TEST_ASSERT_EQUAL(2, voice.size());
  Note &note = voice.next();
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
  RUN_TEST(test_should_release_note_on_start_with_zero_velocity);
  RUN_TEST(test_should_not_release_other_voice);
  RUN_TEST(test_should_allow_the_minimum_size_of_one);
  RUN_TEST(test_off);
  RUN_TEST(test_should_return_the_note_with_least_time2);
  RUN_TEST(test_adjust_size);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
