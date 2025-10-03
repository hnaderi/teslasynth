#include <envelope.hpp>
#include <string>
#include <unity.h>

void test_level_comparison(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) > EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) < EnvelopeLevel(0.02));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.02) >= EnvelopeLevel(0.01));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) <= EnvelopeLevel(0.02));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.1) == EnvelopeLevel(0.1));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.2) != EnvelopeLevel(0.1));

  TEST_ASSERT_TRUE(EnvelopeLevel(-1) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(-1000) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(101) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(EnvelopeLevel(1e4f) == EnvelopeLevel(100));
  TEST_ASSERT_TRUE(EnvelopeLevel(1e4f) == EnvelopeLevel(1.f));
  TEST_ASSERT_TRUE(EnvelopeLevel(100) == EnvelopeLevel(1.f));

  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) != EnvelopeLevel(0.810));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) == EnvelopeLevel(0.801));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) == EnvelopeLevel(0.8001));

  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) != EnvelopeLevel(0.64));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) != EnvelopeLevel(0.632));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) == EnvelopeLevel(0.631));
}

void test_level_arithmetic(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(0) + EnvelopeLevel(1) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.1) + EnvelopeLevel(0.1) ==
                   EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(EnvelopeLevel(1) + EnvelopeLevel(0.01) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE((EnvelopeLevel(1) += EnvelopeLevel(0.01)) ==
                   EnvelopeLevel(1));
  TEST_ASSERT_TRUE((EnvelopeLevel(1) += EnvelopeLevel(1)) == EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(EnvelopeLevel(1) - EnvelopeLevel(0.4f), 0.6f);

  TEST_ASSERT_TRUE((EnvelopeLevel(1) += 0.5f) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE((EnvelopeLevel(1) += -0.5f) == EnvelopeLevel(0.5));
  TEST_ASSERT_TRUE((EnvelopeLevel(1) += -2.f) == EnvelopeLevel(0));
}

void test_level_to_duration(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) * 1_ms == 10_us);
  TEST_ASSERT_TRUE(EnvelopeLevel(0.1) * 1_ms == 100_us);
  TEST_ASSERT_TRUE(EnvelopeLevel(1) * 1_ms == 1_ms);
}

void test_curve_lin_positive(void) {
  Curve curve =
      Curve(EnvelopeLevel(0), EnvelopeLevel(1), 10_ms, CurveType::Lin);
  TEST_ASSERT_TRUE(curve.update(1_ms) == EnvelopeLevel(0.1));
  TEST_ASSERT_TRUE(curve.update(1_ms) == EnvelopeLevel(0.2));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(8_ms) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(10_ms) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_positive2(void) {
  Curve curve =
      Curve(EnvelopeLevel(0.35), EnvelopeLevel(1), 65_ms, CurveType::Lin);
  TEST_ASSERT_TRUE(curve.update(5_ms) == EnvelopeLevel(0.4));
  TEST_ASSERT_TRUE(curve.update(10_ms) == EnvelopeLevel(0.5));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(500_ms) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(10_ms) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.2), 10_ms, CurveType::Lin);
  TEST_ASSERT_TRUE(curve.update(5_ms) == EnvelopeLevel(0.6));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(5_ms) - EnvelopeLevel(0.2) <= 1e3);
  TEST_ASSERT_TRUE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(10_ms) == EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative2(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.7), 100_ms, CurveType::Lin);
  TEST_ASSERT_TRUE(curve.update(50_ms) == EnvelopeLevel(0.85));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(500_ms) - EnvelopeLevel(0.7) <= 1e3);
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative_small(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.65), 10_ms, CurveType::Lin);
  TEST_ASSERT_TRUE(curve.update(5_ns) - EnvelopeLevel(1) <= 1e3);
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(5_ns) - EnvelopeLevel(1) <= 1e3);
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(10_ms) - EnvelopeLevel(0.65) <= 1e3);
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_exp_positive(void) {
  Curve curve = Curve(EnvelopeLevel(0), EnvelopeLevel(1), 5_ms, CurveType::Exp);
  TEST_ASSERT_TRUE(curve.update(369_us) == EnvelopeLevel(0.4));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(1660_us) == EnvelopeLevel(0.94));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(10_ms) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_exp_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.4), 50_ms, CurveType::Exp);
  TEST_ASSERT_TRUE(curve.update(1_ms) == EnvelopeLevel(0.922));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(16993_us) == EnvelopeLevel(0.45));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_TRUE(curve.update(34_ms) == EnvelopeLevel(0.4));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_level_comparison);
  RUN_TEST(test_level_arithmetic);
  RUN_TEST(test_level_to_duration);
  RUN_TEST(test_curve_lin_positive);
  RUN_TEST(test_curve_lin_positive2);
  RUN_TEST(test_curve_lin_negative);
  RUN_TEST(test_curve_lin_negative2);
  RUN_TEST(test_curve_lin_negative_small);
  RUN_TEST(test_curve_exp_positive);
  RUN_TEST(test_curve_exp_negative);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
