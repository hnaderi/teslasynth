#include "core.hpp"
#include "envelope.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <unity.h>

using namespace teslasynth::midisynth;

void test_no_limit(void) {
  DutyLimiter limiter;
  for (auto i = 0; i < 100; i++)
    TEST_ASSERT_TRUE(limiter.can_use(10_ms));
}

void test_cant_use_more_than_budget(void) {
  DutyLimiter limiter(DutyCycle(10), 10_ms);
  assert_duration_equal(limiter.budget(), 1_ms);
  TEST_ASSERT_FALSE(limiter.can_use(10_ms));
  assert_duration_equal(limiter.budget(), 1_ms);
}

void test_limit_by_duty_and_window(void) {
  DutyLimiter limiter(DutyCycle(10), 10_ms);
  for (auto i = 10; i > 0; i--) {
    assert_duration_equal(limiter.budget(), 100_us * i);
    TEST_ASSERT_TRUE(limiter.can_use(100_us));
  }
  assert_duration_equal(limiter.budget(), 0_us);
  TEST_ASSERT_FALSE(limiter.can_use(1_us));
}

void test_replenish(void) {
  DutyLimiter limiter(DutyCycle(10), 10_ms);
  assert_duration_equal(limiter.budget(), 1_ms);
  TEST_ASSERT_TRUE(limiter.can_use(1_ms));
  assert_duration_equal(limiter.budget(), 0_ms);
  TEST_ASSERT_FALSE(limiter.can_use(1_us));

  for (auto i = 0; i < 10; i++) {
    assert_duration_equal(limiter.budget(), 0_us);
    limiter.replenish(1_ms);
  }
  assert_duration_equal(limiter.budget(), 1_ms);
}

void test_replenish_cant_exceed_window_limit(void) {
  DutyLimiter limiter(DutyCycle(10), 10_ms);
  assert_duration_equal(limiter.budget(), 1_ms);
  limiter.replenish(1_ms);
  assert_duration_equal(limiter.budget(), 1_ms);

  TEST_ASSERT_TRUE(limiter.can_use(100_us));
  assert_duration_equal(limiter.budget(), 900_us);

  limiter.replenish(10_ms);
  assert_duration_equal(limiter.budget(), 1_ms);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_no_limit);
  RUN_TEST(test_cant_use_more_than_budget);
  RUN_TEST(test_limit_by_duty_and_window);
  RUN_TEST(test_replenish);
  RUN_TEST(test_replenish_cant_exceed_window_limit);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
