#include <envelope.hpp>
#include <unity.h>

void test_level_comparison(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(1) > EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(1) < EnvelopeLevel(2));
  TEST_ASSERT_TRUE(EnvelopeLevel(2) >= EnvelopeLevel(1));
  TEST_ASSERT_TRUE(EnvelopeLevel(1) <= EnvelopeLevel(2));
  TEST_ASSERT_TRUE(EnvelopeLevel(10) == EnvelopeLevel(10));
  TEST_ASSERT_TRUE(EnvelopeLevel(20) != EnvelopeLevel(10));

  TEST_ASSERT_TRUE(EnvelopeLevel(-1) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(-1000) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(101) == EnvelopeLevel(100));
  TEST_ASSERT_TRUE(EnvelopeLevel(1e8) == EnvelopeLevel(100));
}

void test_level_arithmetic(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(0) + EnvelopeLevel(1) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(EnvelopeLevel(1) + EnvelopeLevel(1) == EnvelopeLevel(2));
  TEST_ASSERT_TRUE(EnvelopeLevel(100) + EnvelopeLevel(1) == EnvelopeLevel(100));
  TEST_ASSERT_TRUE((EnvelopeLevel(100) += EnvelopeLevel(1)) ==
                   EnvelopeLevel(100));
  TEST_ASSERT_TRUE((EnvelopeLevel(100) += EnvelopeLevel(100)) ==
                   EnvelopeLevel(100));
}

void test_level_to_duration(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(1) * 1_ms == 10_us);
  TEST_ASSERT_TRUE(EnvelopeLevel(10) * 1_ms == 100_us);
  TEST_ASSERT_TRUE(EnvelopeLevel(100) * 1_ms == 1_ms);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_level_comparison);
  RUN_TEST(test_level_arithmetic);
  RUN_TEST(test_level_to_duration);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
