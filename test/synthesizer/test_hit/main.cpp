#include "core/duration.hpp"
#include "core/probability.hpp"
#include "percussion.hpp"
#include "presets.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <cstdio>
#include <unity.h>

using namespace teslasynth::synth;

void test_empty(void) {
  Hit hit;
  TEST_ASSERT_FALSE(hit.is_active());
  TEST_ASSERT_FALSE(hit.next());
  TEST_ASSERT_TRUE(hit.current().start.is_zero());
  TEST_ASSERT_TRUE(hit.current().period.is_zero());
  TEST_ASSERT_TRUE(hit.current().volume.is_zero());
}

void test_start(void) {
  Hit hit;

  hit.start({127, 0}, 1_s, {30_ms, 1_khz});
  TEST_ASSERT_TRUE(hit.is_active());
  TEST_ASSERT_TRUE(hit.current().start >= 1_s);
  TEST_ASSERT_FALSE(hit.current().period.is_zero());
  TEST_ASSERT_FALSE(hit.current().volume.is_zero());
}

void test_generate_bursts_pitched(void) {
  Hit hit;

  hit.start({127, 127}, 1_s, {30_ms, 1_khz});
  TEST_ASSERT_TRUE(hit.is_active());
  Duration32 last = 1_s;
  auto counter = 0;
  float total_ontime = 0;
  int total_non_silent = 0;
  do {
    counter++;
    const auto &current = hit.current();
    assert_duration_equal(current.start, last);
    TEST_ASSERT_FALSE(current.period.is_zero());
    last += current.period;
    total_ontime += current.volume;
    if (!current.volume.is_zero())
      total_non_silent++;
  } while (counter < 100 && hit.next());

  TEST_ASSERT_GREATER_OR_EQUAL(20, total_non_silent);
  TEST_ASSERT_TRUE(total_ontime > 0);
  TEST_ASSERT_FALSE(hit.is_active());
  TEST_ASSERT_GREATER_OR_EQUAL(20, counter);
}

void test_generate_bursts_random(void) {
  Hit hit;
  Percussion params{.burst = 30_ms, .prf = 0_hz, .noise = Probability(1)};
  hit.start({127, 0}, 1_s, params);
  TEST_ASSERT_TRUE(hit.is_active());
  Duration32 last = 1_s;
  auto counter = 0;
  float total_ontime = 0;
  int total_non_silent = 0;
  do {
    counter++;
    const auto &current = hit.current();
    assert_duration_equal(current.start, last);
    TEST_ASSERT_FALSE(current.period.is_zero());
    last += current.period;
    total_ontime += current.volume;
    if (!current.volume.is_zero())
      total_non_silent++;
  } while (counter < 30 && hit.next());

  // Mostly random, but should not be silent
  TEST_ASSERT_GREATER_THAN(10, total_non_silent);
  TEST_ASSERT_TRUE(total_ontime > 0);
  TEST_ASSERT_FALSE(hit.is_active());
  TEST_ASSERT_GREATER_THAN(10, counter);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start);
  RUN_TEST(test_generate_bursts_pitched);
  RUN_TEST(test_generate_bursts_random);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
