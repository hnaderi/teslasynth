#include "core/duration.hpp"
#include "percussion.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
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

void test_generate_bursts(void) {
  Hit hit;

  hit.start({127, 0}, 1_s, {30_ms, 1_khz});
  TEST_ASSERT_TRUE(hit.is_active());
  Duration32 last = 1_s;
  auto counter = 0;
  do {
    counter++;
    const auto &current = hit.current();
    assert_duration_equal(current.start, last);
    TEST_ASSERT_FALSE(current.period.is_zero());
    last += current.period;
  } while (counter < 30 && hit.next());

  TEST_ASSERT_FALSE(hit.is_active());
  TEST_ASSERT_EQUAL(30, counter);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start);
  RUN_TEST(test_generate_bursts);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
