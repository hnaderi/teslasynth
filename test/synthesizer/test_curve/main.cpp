#include "core/duration.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "unity_internals.h"
#include <cmath>
#include <envelope.hpp>
#include <unity.h>

using namespace teslasynth::synth;

void test_curve_lin_positive(void) {
  Curve curve =
      Curve(EnvelopeLevel(0), EnvelopeLevel(1), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.1));
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(8_ms), 0_ms);
  assert_level_equal(curve.update(8_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(10_ms), 10_ms);
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_positive2(void) {
  Curve curve =
      Curve(EnvelopeLevel(0.35), EnvelopeLevel(1), 65_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.4));
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.5));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(500_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_positive3(void) {
  Curve curve =
      Curve(EnvelopeLevel(0), EnvelopeLevel(1), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.5));
  TEST_ASSERT_FALSE(curve.is_target_reached());
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.2), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.6));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(10_ms), 5_ms);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(10_ms), 10_ms);
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_lin_negative2(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.7), 100_ms, CurveType::Lin);
  assert_level_equal(curve.update(50_ms), EnvelopeLevel(0.85));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(500_ms), EnvelopeLevel(0.7));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative_small(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.65), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_us), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(5_us), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.65));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_exp_positive(void) {
  Curve curve = Curve(EnvelopeLevel(0), EnvelopeLevel(1), 5_ms, CurveType::Exp);
  assert_level_equal(curve.update(369_us), EnvelopeLevel(0.4));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(5_ms), 369_us);
  TEST_ASSERT_FALSE(curve.how_much_remains_after(1660_us));
  assert_level_equal(curve.update(1660_us), EnvelopeLevel(0.94));
  TEST_ASSERT_FALSE(curve.is_target_reached());
  TEST_ASSERT_FALSE(curve.how_much_remains_after(2_ms));

  assert_duration_equal(curve.how_much_remains_after(2971_us), 0_us);
  assert_duration_equal(curve.how_much_remains_after(12971_us), 10_ms);
  assert_duration_equal(curve.how_much_remains_after(10_ms), 7029_us);

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  assert_duration_equal(curve.how_much_remains_after(10_ms), 10_ms);
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_exp_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.4), 50_ms, CurveType::Exp);
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.922));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(16993_us), EnvelopeLevel(0.45));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(34_ms), EnvelopeLevel(0.4));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_must_be_only_time_dependent_after_creation(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(1), 100_ms, CurveType::Exp);

  TEST_ASSERT_FALSE(curve.how_much_remains_after(7_us));
  assert_level_equal(curve.update(7_us), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  TEST_ASSERT_FALSE(curve.how_much_remains_after(3120_us));
  assert_level_equal(curve.update(3120_us), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.how_much_remains_after(100_ms), 3127_us);
  assert_level_equal(curve.update(100_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_constant(void) {
  Curve curve = Curve(EnvelopeLevel(0.6));
  TEST_ASSERT_FALSE_MESSAGE(curve.is_target_reached(),
                            "Constant curve never reaches target!");
  TEST_ASSERT_FALSE_MESSAGE(curve.how_much_remains_after(10_s).has_value(),
                            "Constant curve never reaches target!");
  assert_level_equal(curve.update(0_us), EnvelopeLevel(0.6));
}
void test_curve_constant_zero(void) {
  Curve curve = Curve(EnvelopeLevel(0));
  TEST_ASSERT_FALSE_MESSAGE(curve.is_target_reached(),
                            "Constant curve never reaches target!");
  TEST_ASSERT_FALSE_MESSAGE(curve.how_much_remains_after(10_s).has_value(),
                            "Constant curve never reaches target!");
  assert_level_equal(curve.update(0_us), EnvelopeLevel(0));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_curve_lin_positive);
  RUN_TEST(test_curve_lin_positive2);
  RUN_TEST(test_curve_lin_positive3);
  RUN_TEST(test_curve_lin_negative);
  RUN_TEST(test_curve_lin_negative2);
  RUN_TEST(test_curve_lin_negative_small);
  RUN_TEST(test_curve_exp_positive);
  RUN_TEST(test_curve_exp_negative);
  RUN_TEST(test_curve_must_be_only_time_dependent_after_creation);
  RUN_TEST(test_curve_constant);
  RUN_TEST(test_curve_constant_zero);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
