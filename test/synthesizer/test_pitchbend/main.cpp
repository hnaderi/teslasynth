#include "pitchbend.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <unity.h>

using namespace teslasynth::synth;

void test_flat(void) {
  PitchBend pb;

  TEST_ASSERT_EQUAL(pb.multiplier(), 1);
}

void test_bend(void) {
  PitchBend pb_up(1), pb_down(-1);

  assert_hertz_equal(pb_up * 100_hz, 100_hz * exp2f(2 / 12.f));
  assert_hertz_equal(pb_down * 100_hz, 100_hz * exp2f(-2 / 12.f));
}

void test_comparision(void) {
  PitchBend pb1(1), pb2(2), pb3(0), pb4(-1), pb5(-2), pbd;

  TEST_ASSERT_TRUE(pb1 == pb2);
  TEST_ASSERT_FALSE(pb1 == pb3);

  TEST_ASSERT_TRUE(pb3 == pbd);
  TEST_ASSERT_FALSE(pbd == pb4);

  TEST_ASSERT_TRUE(pb4 == pb5);
  TEST_ASSERT_FALSE(pb4 == pbd);

  TEST_ASSERT_FALSE(pb1 == pb5);
  TEST_ASSERT_TRUE(pb1 != pb5);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_flat);
  RUN_TEST(test_bend);
  RUN_TEST(test_comparision);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
