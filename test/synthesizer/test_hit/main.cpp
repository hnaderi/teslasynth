// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "core/duration.hpp"
#include "core/envelope_level.hpp"
#include "voices/hit.hpp"
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

  hit.start(127, EnvelopeLevel::zero(), 1_s, {30_ms, 1_khz});
  TEST_ASSERT_TRUE(hit.is_active());
  TEST_ASSERT_TRUE(hit.current().start >= 1_s);
  TEST_ASSERT_FALSE(hit.current().period.is_zero());
  assert_level_equal(hit.current().volume, EnvelopeLevel::zero());
}

void test_generate_bursts_pitched(void) {
  Hit hit;

  hit.start(127, EnvelopeLevel::max(), 1_s, {30_ms, 1_khz});
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
  hit.start(127, EnvelopeLevel::max(), 1_s, params);
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

  // Mostly random, but should not be silent
  TEST_ASSERT_GREATER_THAN(10, total_non_silent);
  TEST_ASSERT_TRUE(total_ontime > 0);
  TEST_ASSERT_FALSE(hit.is_active());
  TEST_ASSERT_GREATER_THAN(10, counter);
}

// Regardless of jitter magnitude, the clipped PRF must keep periods inside
// [period(4kHz), period(20Hz)] = [250us, 50ms].
void test_pitched_period_bounds_high_prf(void) {
  // 3kHz PRF + max noise: positive jitter can push above 4kHz ceiling
  Percussion params{.burst = 500_ms, .prf = 3_khz, .noise = Probability(1)};
  Hit hit;
  hit.start(127, EnvelopeLevel::max(), 0_s, params);

  const auto min_period = Duration32::micros(250); // 4kHz
  const auto max_period = Duration32::millis(50);  // 20Hz

  int count = 0;
  do {
    count++;
    const Duration32 p = hit.current().period;
    TEST_ASSERT_TRUE_MESSAGE(p >= min_period, "Period too short (above 4kHz)");
    TEST_ASSERT_TRUE_MESSAGE(p <= max_period, "Period too long (below 20Hz)");
  } while (count < 500 && hit.next());

  TEST_ASSERT_GREATER_THAN(10, count);
}

void test_pitched_period_bounds_low_prf(void) {
  // 50Hz PRF + max noise: negative jitter can push near 0Hz (below 20Hz floor)
  Percussion params{.burst = 500_ms, .prf = 50_hz, .noise = Probability(1)};
  Hit hit;
  hit.start(127, EnvelopeLevel::max(), 0_s, params);

  const auto min_period = Duration32::micros(250);
  const auto max_period = Duration32::millis(50);

  int count = 0;
  do {
    count++;
    const Duration32 p = hit.current().period;
    TEST_ASSERT_TRUE_MESSAGE(p >= min_period, "Period too short (above 4kHz)");
    TEST_ASSERT_TRUE_MESSAGE(p <= max_period, "Period too long (below 20Hz)");
  } while (count < 500 && hit.next());

  TEST_ASSERT_GREATER_THAN(10, count);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start);
  RUN_TEST(test_generate_bursts_pitched);
  RUN_TEST(test_generate_bursts_random);
  RUN_TEST(test_pitched_period_bounds_high_prf);
  RUN_TEST(test_pitched_period_bounds_low_prf);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
}
