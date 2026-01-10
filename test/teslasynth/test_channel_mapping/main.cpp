#include "channel_mapping.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include <unity.h>

using namespace teslasynth::midisynth;

void test_output_number_default(void) {
  TEST_ASSERT_FALSE(OutputNumberOpt<1>());
  TEST_ASSERT_FALSE(OutputNumberOpt<2>());
  TEST_ASSERT_FALSE(OutputNumberOpt<3>());
}
void test_output_number(void) {
  TEST_ASSERT_FALSE(OutputNumberOpt<1>(2));
  TEST_ASSERT_FALSE(OutputNumberOpt<2>(2));
  TEST_ASSERT_TRUE(OutputNumberOpt<3>(2));

  const auto a = OutputNumberOpt<1>(1);
  TEST_ASSERT_TRUE(a.value() == std::nullopt);

  const auto b = OutputNumberOpt<1>(2);
  TEST_ASSERT_TRUE(b.value() == std::nullopt);

  const auto c = OutputNumberOpt<2>(2);
  TEST_ASSERT_TRUE(c.value() == std::nullopt);

  const auto d = OutputNumberOpt<3>(2);
  TEST_ASSERT_TRUE(d.value() == OutputNumber<3>::from(2));
}

void test_midi_router_config(void) {
  constexpr auto MAX = 5;
  ChannelMapping<MAX> mapping;

  for (auto i = 0; i < MAX; i++) {
    TEST_ASSERT_TRUE(mapping[i]);
  }
  for (auto i = MAX; i < 16; i++) {
    TEST_ASSERT_FALSE(mapping[i]);
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_output_number_default);
  RUN_TEST(test_output_number);
  RUN_TEST(test_midi_router_config);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
