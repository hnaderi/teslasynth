#include "channel_mapping.hpp"
#include "config_data.hpp"
#include "config_patch_update.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "unity_internals.h"
#include <cstdio>
#include <iostream>
#include <unity.h>

using namespace teslasynth::midisynth;
using namespace teslasynth::midisynth::config::parser;

#define ASSERT_UPDATES(key, config)                                            \
  {                                                                            \
    auto res = config::patch::update(key, config);                             \
    TEST_ASSERT_TRUE_MESSAGE(res, res.error().c_str());                        \
  }

#define ASSERT_NO_UPDATES(key, config)                                         \
  {                                                                            \
    TEST_ASSERT_FALSE(config::patch::update(key, config));                     \
  }

void test_empty(void) {
  Configuration<3> config;
  auto res = config::patch::update("", config);
  TEST_ASSERT_FALSE(res);
}

void test_synth_tuning(void) {
  Configuration<3> config;
  ASSERT_UPDATES("synth.tuning=220", config);

  assert_hertz_equal(220_hz, config.synth().tuning);
}

void test_synth_error(void) {
  Configuration<3> config, bkp = config;
  ASSERT_NO_UPDATES("synth.tunin=220", config);
  TEST_ASSERT_TRUE(config.synth() == bkp.synth());
}

void test_synth_instrument(void) {
  Configuration<3> config;
  ASSERT_UPDATES("synth.instrument=2", config);
  TEST_ASSERT_TRUE(config.synth().instrument == 1);
}

void test_synth_config(void) {
  Configuration<3> config;
  config.synth().instrument = 2;

  ASSERT_UPDATES("synth.instrument=-1 synth.tuning=330hz", config);
  TEST_ASSERT_TRUE(config.synth().instrument == std::nullopt);
  TEST_ASSERT_TRUE(config.synth().tuning == 330_hz);
}

void test_channel_config(void) {
  Configuration<3> config;

  ASSERT_UPDATES("output.1.instrument=3 "
                 "output.3.max-on-time=123us "
                 "output.2.max-duty=32.5",
                 config);
  TEST_ASSERT_TRUE(config.channel(0).instrument == 2);
  assert_duration_equal(123_us, config.channel(2).max_on_time);
  TEST_ASSERT_TRUE(DutyCycle(32.5) == config.channel(1).max_duty);
}

void test_channel_config_error(void) {
  Configuration<3> config;

  ASSERT_NO_UPDATES("output.4.instrument=3 ", config);
}

void test_channel_config_multiple(void) {
  Configuration<3> config;

  ASSERT_UPDATES("output.1.instrument="
                 "output.2.instrument="
                 "output.3.instrument=3",
                 config);
  TEST_ASSERT_TRUE(config.channel(0).instrument == 2);
  TEST_ASSERT_TRUE(config.channel(1).instrument == 2);
  TEST_ASSERT_TRUE(config.channel(2).instrument == 2);
}

void test_channel_config_wildcard(void) {
  Configuration<10> config;

  ASSERT_UPDATES("output.*.instrument=5", config);
  for (int i = 0; i < config.channels_size(); i++)
    TEST_ASSERT_TRUE(config.channel(i).instrument == 4);
}

void test_routing(void) {
  Configuration<3> config;

  ASSERT_UPDATES("routing.percussion=y "
                 "routing.channel.10=1 "
                 "routing.channel.3=2 "
                 "routing.channel.2=3 "
                 "routing.channel.1=1",
                 config);

  TEST_ASSERT_TRUE(config.routing().percussion);
  TEST_ASSERT_TRUE(config.routing().mapping[9] == 0);
  TEST_ASSERT_TRUE(config.routing().mapping[2] == 1);
  TEST_ASSERT_TRUE(config.routing().mapping[1] == 2);
  TEST_ASSERT_TRUE(config.routing().mapping[0] == 0);
}

void test_routing_wildcard(void) {
  Configuration<3> config;

  ASSERT_UPDATES("routing.channel.*=2", config);
  for (auto &m : config.routing().mapping) {
    TEST_ASSERT_TRUE(m == 1);
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_synth_tuning);
  RUN_TEST(test_synth_error);
  RUN_TEST(test_synth_instrument);
  RUN_TEST(test_synth_config);
  RUN_TEST(test_channel_config);
  RUN_TEST(test_channel_config_multiple);
  RUN_TEST(test_channel_config_wildcard);
  RUN_TEST(test_channel_config_error);
  RUN_TEST(test_routing);
  RUN_TEST(test_routing_wildcard);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
