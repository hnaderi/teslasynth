#include "core.hpp"
#include "envelope.hpp"
#include "midi_synth.hpp"
#include "pulse.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <unity.h>

using namespace teslasynth::midisynth;

void test_note_pulse_empty(void) {
  NotePulse pulse;
  TEST_ASSERT_TRUE(pulse.is_zero());
  assert_duration_equal(pulse.start, Duration::zero());
  assert_level_equal(pulse.volume, EnvelopeLevel());
}

void test_duty_default() {
  DutyCycle p;
  TEST_ASSERT_EQUAL(0, p);
}

void test_duty() {
  DutyCycle p1(10);
  TEST_ASSERT_EQUAL(20, p1.value());
  TEST_ASSERT_EQUAL(180, p1.inverse());
  TEST_ASSERT_EQUAL(10, p1.percent());
  TEST_ASSERT_EQUAL(0.1, p1);

  DutyCycle p2(15);
  TEST_ASSERT_EQUAL(30, p2.value());
  TEST_ASSERT_EQUAL(170, p2.inverse());
  TEST_ASSERT_EQUAL(15, p2.percent());
  TEST_ASSERT_EQUAL(0.15, p2);

  DutyCycle p3(25);
  TEST_ASSERT_EQUAL(50, p3.value());
  TEST_ASSERT_EQUAL(150, p3.inverse());
  TEST_ASSERT_EQUAL(25, p3.percent());
  TEST_ASSERT_EQUAL(0.25, p3);

  DutyCycle p4(50);
  TEST_ASSERT_EQUAL(100, p4.value());
  TEST_ASSERT_EQUAL(100, p4.inverse());
  TEST_ASSERT_EQUAL(50, p4.percent());
  TEST_ASSERT_EQUAL(0.5, p4);

  DutyCycle p5(100);
  TEST_ASSERT_EQUAL(200, p5.value());
  TEST_ASSERT_EQUAL(0, p5.inverse());
  TEST_ASSERT_EQUAL(100, p5.percent());
  TEST_ASSERT_EQUAL(1.f, p5);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_note_pulse_empty);
  RUN_TEST(test_duty_default);
  RUN_TEST(test_duty);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
