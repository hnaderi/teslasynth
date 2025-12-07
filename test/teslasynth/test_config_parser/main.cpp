#include "config_parser.hpp"
#include "core.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <unity.h>

using namespace teslasynth::midisynth;
using namespace teslasynth::midisynth::config::parser;

void test_parse_kv_errors(void) {
  TEST_ASSERT_FALSE(parse_config_value("").has_value());
  TEST_ASSERT_FALSE(parse_config_value("=").has_value());
  TEST_ASSERT_FALSE(parse_config_value("=:").has_value());
  TEST_ASSERT_FALSE(parse_config_value("=1:2").has_value());
  TEST_ASSERT_FALSE(parse_config_value("a:=12").has_value());
}

void test_parse_kv_success(void) {
  auto result = parse_config_value("ABc:12=abc123:456");
  TEST_ASSERT_TRUE(result.has_value());

  auto value = *result;
  TEST_ASSERT_EQUAL(12, value.channel);
  TEST_ASSERT_EQUAL_STRING("ABc", std::string(value.key).c_str());
  TEST_ASSERT_EQUAL_STRING("abc123:456", std::string(value.value).c_str());
}

void test_parse_kv_success2(void) {
  auto result = parse_config_value("ABc=abc123:456");
  TEST_ASSERT_TRUE(result.has_value());

  auto value = *result;
  TEST_ASSERT_EQUAL(0, value.channel);
  TEST_ASSERT_EQUAL_STRING("ABc", std::string(value.key).c_str());
  TEST_ASSERT_EQUAL_STRING("abc123:456", std::string(value.value).c_str());
}

void test_parse_kv_success3(void) {
  auto result = parse_config_value("instrument:3=15");
  TEST_ASSERT_TRUE(result.has_value());

  auto value = *result;
  TEST_ASSERT_EQUAL(3, value.channel);
  TEST_ASSERT_EQUAL_STRING("instrument", std::string(value.key).c_str());
  TEST_ASSERT_EQUAL_STRING("15", std::string(value.value).c_str());
}

void test_parse_duration_success(void) {
  auto result = parse_duration16("10ms");
  TEST_ASSERT_TRUE(result.has_value());
  assert_duration_equal(result.value(), 10_ms);

  result = parse_duration16("100us");
  TEST_ASSERT_TRUE(result.has_value());
  assert_duration_equal(result.value(), 100_us);
}

void test_parse_hertz_success(void) {
  auto result = parse_hertz("10");
  TEST_ASSERT_TRUE(result.has_value());
  assert_hertz_equal(result.value(), 10_hz);

  result = parse_hertz("10khz");
  TEST_ASSERT_TRUE(result.has_value());
  assert_hertz_equal(result.value(), 10_khz);

  result = parse_hertz("520hz");
  TEST_ASSERT_TRUE(result.has_value());
  assert_hertz_equal(result.value(), 520_hz);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_kv_errors);
  RUN_TEST(test_parse_kv_success);
  RUN_TEST(test_parse_kv_success2);
  RUN_TEST(test_parse_kv_success3);
  RUN_TEST(test_parse_duration_success);
  RUN_TEST(test_parse_hertz_success);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
