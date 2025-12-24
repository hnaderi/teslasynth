#include "synthesizer/helpers/assertions.hpp"
#include <cmath>
#include <envelope.hpp>
#include <unity.h>

using namespace teslasynth::synth;

const envelopes::ADSR lin_adsr{10_ms, 20_ms, EnvelopeLevel(0.5), 30_ms,
                               CurveType::Lin};
const envelopes::ADSR exp_adsr{10_ms, 20_ms, EnvelopeLevel(0.5), 30_ms,
                               CurveType::Exp};

void test_envelope_lin_full(void) {
  Envelope env(lin_adsr);
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(5_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(1_ms, true), EnvelopeLevel(0.6));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(4_ms, true), EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(1, env.stage());
  assert_level_equal(env.update(20_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(2, env.stage());
  assert_level_equal(env.update(2000_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(2, env.stage());
  assert_level_equal(env.update(3_ms, false), EnvelopeLevel(0.45));
  TEST_ASSERT_EQUAL(3, env.stage());
  assert_level_equal(env.update(3_ms, false), EnvelopeLevel(0.40));
  TEST_ASSERT_EQUAL(3, env.stage());
  assert_level_equal(env.update(24_ms, false), EnvelopeLevel(0));
  TEST_ASSERT_TRUE(env.is_off());
}
void test_envelope_exp_full(void) {
  Envelope env(exp_adsr);
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(10_ms, true), EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(1, env.stage());
  assert_level_equal(env.update(20_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(2, env.stage());
  assert_level_equal(env.update(300_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(300_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(2, env.stage());
  TEST_ASSERT_TRUE(env.update(1_us, false) < EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(3, env.stage());
  auto lvl = env.update(10_ms, false);
  TEST_ASSERT_TRUE(lvl < EnvelopeLevel(0.5));
  TEST_ASSERT_TRUE(lvl > EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(3, env.stage());
  assert_level_equal(env.update(20_ms, false), EnvelopeLevel(0));
  TEST_ASSERT_TRUE(env.is_off());
}
void test_envelope_const_full(void) {
  Envelope env(EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(1_us, false), EnvelopeLevel(0));
  TEST_ASSERT_TRUE(env.is_off());
}

void test_envelope_const_zero(void) {
  Envelope env(EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(1_us, false), EnvelopeLevel(0));
  TEST_ASSERT_TRUE(env.is_off());
}
void test_envelope_const_value(void) {
  Envelope env(EnvelopeLevel(0.4));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0.4));
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0.4));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0.4));
  TEST_ASSERT_EQUAL(0, env.stage());
  assert_level_equal(env.update(0_us, false), EnvelopeLevel(0));
  TEST_ASSERT_TRUE(env.is_off());
}

void test_envelope_comparison(void) {
  TEST_ASSERT_TRUE(lin_adsr == lin_adsr);
  TEST_ASSERT_FALSE(lin_adsr != lin_adsr);
  TEST_ASSERT_TRUE(exp_adsr == exp_adsr);
  TEST_ASSERT_FALSE(exp_adsr != exp_adsr);
  // TEST_ASSERT_TRUE(const_adsr == const_adsr);
  // TEST_ASSERT_FALSE(const_adsr != const_adsr);

  TEST_ASSERT_TRUE(lin_adsr != exp_adsr);
  TEST_ASSERT_FALSE(lin_adsr == exp_adsr);
  // TEST_ASSERT_TRUE(lin_adsr != const_adsr);
  // TEST_ASSERT_FALSE(lin_adsr == const_adsr);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_envelope_lin_full);
  RUN_TEST(test_envelope_exp_full);
  RUN_TEST(test_envelope_const_full);
  RUN_TEST(test_envelope_const_zero);
  RUN_TEST(test_envelope_const_value);
  RUN_TEST(test_envelope_comparison);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
