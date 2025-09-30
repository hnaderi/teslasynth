#include <synth.hpp>
#include <unity.h>

void setUp(void) {}

void tearDown(void) {}

void test_should_not_be_active(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
}

void test_started_note_initial_state(void) {
  Note note(69, 127, 0, 100);
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_EQUAL(note.number(), 69);
  TEST_ASSERT_EQUAL(note.time(), 100);
}

void test_note_tick_on(void) {
  Note note(69, 127, 0, 100);
  // Assume to base note is 100Hz to simplify calculations
  Config config{.a440 = 100};
  NotePulse pulse;
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL(100, pulse.start);
  TEST_ASSERT_EQUAL(300, pulse.off);
  TEST_ASSERT_EQUAL(20100, pulse.end);
  TEST_ASSERT_EQUAL(20100, note.time());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_should_not_be_active);
  RUN_TEST(test_started_note_initial_state);
  RUN_TEST(test_note_tick_on);
  UNITY_END();
}
