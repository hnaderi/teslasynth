#include "envelope.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <synth.hpp>
#include <unity.h>

Note note;
// Assume that base note is 100Hz to simplify calculations
constexpr Config config{.a440 = 100_hz};
constexpr MidiNote mnote1{69, 127}, mnote2{69 + 12, 127},
    mnote3{69 + 2 * 12, 127};
const Envelope envelope(EnvelopeLevel(1));

void setUp(void) { note.start(mnote1, 100_us, envelope, config); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().off.is_zero());
  TEST_ASSERT_TRUE(note.current().end.is_zero());

  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().off.is_zero());
  TEST_ASSERT_TRUE(note.current().end.is_zero());
}

void test_midi_note_frequency(void) {
  assert_hertz_equal(mnote1.frequency(config), 100_hz);
  assert_hertz_equal(mnote2.frequency(config), 200_hz);
  assert_hertz_equal(mnote3.frequency(config), 400_hz);

  assert_hertz_equal(mnote1.frequency({}), 440_hz);
  assert_hertz_equal(mnote2.frequency({}), 880_hz);
  assert_hertz_equal(mnote3.frequency({}), 1760_hz);
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());
  assert_duration_equal(note.current().start, 100_us);
  assert_duration_equal(note.current().off, 200_us);
  assert_duration_equal(note.current().end, 10100_us);
}

void test_note_next(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_duration_equal(note.current().off, 10200_us);
  assert_duration_equal(note.current().end, 20100_us);
}

void test_note_release(void) {
  note.release(30000_us);

  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  TEST_ASSERT_TRUE(note.next());

  note.next();
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().off, 20200_us);
  assert_duration_equal(note.current().end, 30100_us);

  TEST_ASSERT_FALSE(note.next());
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().off, 20200_us);
  assert_duration_equal(note.current().end, 30100_us);
}

void test_note_second_start(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_duration_equal(note.current().off, 10200_us);
  assert_duration_equal(note.current().end, 20100_us);

  note.start({69 + 12, 127}, 100_ms, envelope, config);
  assert_duration_equal(note.current().start, 100_ms);
  assert_duration_equal(note.current().off, 100_ms + 100_us);
  assert_duration_equal(note.current().end, 105_ms);
}

void test_note_start_after_release(void) {
  note.release(30000_us);
  test_note_second_start();
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_midi_note_frequency);
  RUN_TEST(test_started_note_initial_state);
  RUN_TEST(test_note_next);
  RUN_TEST(test_note_release);
  RUN_TEST(test_note_second_start);
  RUN_TEST(test_note_start_after_release);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
