// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "core/duration.hpp"
#include "core/envelope_level.hpp"
#include "bank/instruments.hpp"
#include "percussion.hpp"
#include "presets.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "voice_event.hpp"
#include <cstdint>
#include <unity.h>

using namespace teslasynth::synth;

void test_empty(void) {
  VoiceEvent event;
  TEST_ASSERT_FALSE(event.is_active());
  TEST_ASSERT_FALSE(event.next());
  TEST_ASSERT_TRUE(event.current().start.is_zero());
  TEST_ASSERT_TRUE(event.current().period.is_zero());
  TEST_ASSERT_TRUE(event.current().volume.is_zero());
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
}

static void start_tone(VoiceEvent &event) {
  const Instrument instrument;
  const PitchPreset preset{&instrument, 100_hz};
  event.start(69, EnvelopeLevel::max(), 1_s, preset);
}

void test_start_tone(void) {
  VoiceEvent event;
  start_tone(event);
  const Duration32 period = 10_ms;
  Duration32 now = 1_s;

  for (uint32_t i = 0; i < 100; i++) {
    TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Tone);
    TEST_ASSERT_TRUE(event.is_active());
    assert_duration_equal(event.current().start, now);
    assert_duration_equal(event.current().period, period);
    assert_level_equal(event.current().volume, EnvelopeLevel(1));
    now += period;
    TEST_ASSERT_TRUE(event.next());
  }

  assert_duration_equal(event.current().start, 2_s);
  TEST_ASSERT_TRUE(event.is_active());
  event.release(2_s + 1_ms);
  TEST_ASSERT_TRUE(event.is_active());

  TEST_ASSERT_TRUE(event.next());
  TEST_ASSERT_FALSE(event.next());
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
}

void test_start_hit(void) {
  VoiceEvent event;
  const Percussion percussion{20_ms, 1_khz};
  const PercussivePreset preset{&percussion};
  event.start(127, EnvelopeLevel::zero(), 1_s, preset);

  const Duration32 period = 1_ms;
  Duration32 now = 1_s;

  for (uint8_t i = 1; i < 10; i++) {
    TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);
    TEST_ASSERT_TRUE(event.is_active());
    assert_duration_equal(event.current().start, now);
    assert_duration_equal(event.current().period, period);
    now += period;
    TEST_ASSERT_TRUE(event.next());
  }

  TEST_ASSERT_FALSE(event.is_active());
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);
  TEST_ASSERT_FALSE(event.next());
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
}

static void start_hit(VoiceEvent &event) {
  static const Percussion percussion = Percussion{20_ms, 1_khz};
  const PercussivePreset preset{&percussion};
  event.start(127, EnvelopeLevel::max(), 1_s, preset);
}

void test_release_on_inactive(void) {
  VoiceEvent event;
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
  event.release(1_s); // must not crash
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
  TEST_ASSERT_FALSE(event.is_active());
}

void test_release_on_hit_is_noop(void) {
  VoiceEvent event;
  start_hit(event);
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);
  TEST_ASSERT_TRUE(event.is_active());

  event.release(1_s); // no-op for Hit — no release stage
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);
  TEST_ASSERT_TRUE(event.is_active());
}

void test_off_clears_state(void) {
  VoiceEvent event;
  start_tone(event);
  TEST_ASSERT_TRUE(event.is_active());
  event.off();
  TEST_ASSERT_FALSE(event.is_active());
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::None);
}

void test_reassign_tone_to_hit(void) {
  VoiceEvent event;
  start_tone(event);
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Tone);

  start_hit(event); // overwrite mid-flight
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);
  TEST_ASSERT_TRUE(event.is_active());
}

void test_reassign_hit_to_tone(void) {
  VoiceEvent event;
  start_hit(event);
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Hit);

  start_tone(event); // overwrite mid-flight
  TEST_ASSERT_TRUE(event.type() == VoiceEvent::Type::Tone);
  TEST_ASSERT_TRUE(event.is_active());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start_tone);
  RUN_TEST(test_start_hit);
  RUN_TEST(test_release_on_inactive);
  RUN_TEST(test_release_on_hit_is_noop);
  RUN_TEST(test_off_clears_state);
  RUN_TEST(test_reassign_tone_to_hit);
  RUN_TEST(test_reassign_hit_to_tone);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
}
