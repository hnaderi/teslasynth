// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "codec.hpp"
#include "hardware.hpp"
#include "wifi.hpp"
#include <string>
#include <unity.h>

using namespace teslasynth::app::configuration;
using teslasynth::app::helpers::JSONParser;

namespace {

codec::Decoder<hardware::HardwareConfig> parse_hw(const std::string &json) {
  JSONParser parser(json.c_str());
  return codec::parse_hwconfig(parser);
}

codec::Decoder<AppConfig> parse_synth(const std::string &json) {
  JSONParser parser(json.c_str());
  return codec::parse_appconfig(parser);
}

codec::Decoder<wifi::WifiConfig> parse_wifi(const std::string &json,
                                            const wifi::WifiConfig &current) {
  JSONParser parser(json.c_str());
  return codec::parse_wificonfig(parser, current);
}

std::string hw_json(const std::string &channels) {
  return R"({"output":{"channels":[)" + channels +
         R"(]},"input":{"pin":0},"led":{"pin":8,"active-high":true}})";
}

std::string channel_json(int notes = 4, double duty = 5.0) {
  return R"({"notes":)" + std::to_string(notes) +
         R"(,"instrument":null,"max-on-time":100,"min-deadtime":100,"duty-window":10000,"max-duty":)" +
         std::to_string(duty) + "}";
}

std::string synth_json(const std::string &channels) {
  return R"({"tuning":440,"instrument":null,"channels":[)" + channels +
         R"(],"routing":{"percussion":false,"mapping":[0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1]}})";
}

std::string repeat_channels(int count) {
  std::string out;
  for (int i = 0; i < count; i++) {
    if (i)
      out += ",";
    out += channel_json();
  }
  return out;
}

std::string ascii(size_t len, char c = 'a') {
  return std::string(len, c);
}

std::string wifi_json(const std::string &body) {
  return "{" + body + "}";
}

} // namespace

void test_hardware_accepts_exactly_the_output_count(void) {
  auto parsed = parse_hw(hw_json("4,5,6,7"));
  TEST_ASSERT_TRUE_MESSAGE(parsed, parsed ? "" : parsed.error());
  TEST_ASSERT_EQUAL(4, parsed.value().output.channels[0].pin);
  TEST_ASSERT_EQUAL(7, parsed.value().output.channels[3].pin);
}

void test_hardware_rejects_too_many_channels(void) {
  TEST_ASSERT_FALSE(parse_hw(hw_json("4,5,6,7,8")));
  TEST_ASSERT_FALSE(parse_hw(hw_json("4,5,6,7,8,9")));
}

void test_hardware_rejects_too_few_channels(void) {
  TEST_ASSERT_FALSE(parse_hw(hw_json("4,5,6")));
  TEST_ASSERT_FALSE(parse_hw(hw_json("")));
}

void test_hardware_rejects_out_of_range_pins(void) {
  TEST_ASSERT_FALSE(parse_hw(hw_json("4,5,6,40")));
  TEST_ASSERT_FALSE(parse_hw(hw_json("4,5,6,-2")));
  TEST_ASSERT_TRUE(parse_hw(hw_json("4,5,6,-1")));
}

void test_hardware_roundtrips(void) {
  auto parsed = parse_hw(hw_json("4,5,6,7"));
  TEST_ASSERT_TRUE(parsed);
  auto json = codec::encode(parsed.value()).print();
  auto again = parse_hw(json.value);
  TEST_ASSERT_TRUE(again);
  TEST_ASSERT_EQUAL(7, again.value().output.channels[3].pin);
  TEST_ASSERT_TRUE(again.value().is_valid());
}

void test_default_hardware_config_is_valid(void) {
  TEST_ASSERT_TRUE(hardware::HardwareConfig().is_valid());
}

void test_synth_requires_every_channel(void) {
  TEST_ASSERT_TRUE(parse_synth(synth_json(repeat_channels(4))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(repeat_channels(3))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(repeat_channels(5))));
}

void test_synth_rejects_out_of_range_notes(void) {
  const auto max = teslasynth::midisynth::ChannelConfig::max_notes;
  TEST_ASSERT_TRUE(parse_synth(synth_json(channel_json(max) + "," + repeat_channels(3))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(channel_json(max + 1) + "," + repeat_channels(3))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(channel_json(0) + "," + repeat_channels(3))));
}

void test_synth_rejects_out_of_range_duty(void) {
  TEST_ASSERT_TRUE(parse_synth(synth_json(channel_json(4, 100.0) + "," + repeat_channels(3))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(channel_json(4, 100.5) + "," + repeat_channels(3))));
  TEST_ASSERT_FALSE(parse_synth(synth_json(channel_json(4, 0.0) + "," + repeat_channels(3))));
}

void test_synth_requires_routing(void) {
  const auto missing =
      R"({"tuning":440,"instrument":null,"channels":[)" + repeat_channels(4) + R"(]})";
  TEST_ASSERT_FALSE(parse_synth(missing));
}

void test_parsed_synth_config_is_valid(void) {
  auto parsed = parse_synth(synth_json(repeat_channels(4)));
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_TRUE(parsed.value().is_valid());
}

void test_wifi_ssid_bounds(void) {
  const wifi::WifiConfig current;
  TEST_ASSERT_FALSE(parse_wifi(wifi_json(R"("ssid":"","channel":1)"), current));
  TEST_ASSERT_TRUE(parse_wifi(wifi_json(R"("ssid":"a","channel":1)"), current));
  TEST_ASSERT_TRUE(parse_wifi(wifi_json(R"("ssid":")" + ascii(32) + R"(","channel":1)"), current));
  TEST_ASSERT_FALSE(parse_wifi(wifi_json(R"("ssid":")" + ascii(33) + R"(","channel":1)"), current));
}

void test_wifi_password_bounds(void) {
  const wifi::WifiConfig current;
  const auto with = [&](const std::string &pw) {
    return wifi_json(R"("ssid":"x","channel":1,"password":")" + pw + R"(")");
  };
  TEST_ASSERT_TRUE(parse_wifi(with(""), current));
  TEST_ASSERT_FALSE(parse_wifi(with(ascii(7)), current));
  TEST_ASSERT_TRUE(parse_wifi(with(ascii(8)), current));
  TEST_ASSERT_TRUE(parse_wifi(with(ascii(63)), current));
  TEST_ASSERT_FALSE(parse_wifi(with(ascii(64)), current));
}

void test_wifi_channel_bounds(void) {
  const wifi::WifiConfig current;
  const auto with = [&](int ch) {
    return wifi_json(R"("ssid":"x","channel":)" + std::to_string(ch));
  };
  TEST_ASSERT_FALSE(parse_wifi(with(0), current));
  TEST_ASSERT_TRUE(parse_wifi(with(1), current));
  TEST_ASSERT_TRUE(parse_wifi(with(13), current));
  TEST_ASSERT_FALSE(parse_wifi(with(14), current));
}

void test_wifi_password_is_write_only(void) {
  wifi::WifiConfig current;
  auto json = codec::encode(current).print();
  const std::string encoded(json.value);
  TEST_ASSERT_NULL_MESSAGE(strstr(encoded.c_str(), CONFIG_TESLASYNTH_WIFI_PASSWORD),
                           "encode() must never emit the password");
  TEST_ASSERT_NOT_NULL(strstr(encoded.c_str(), "password-set"));
}

void test_wifi_omitted_password_is_kept(void) {
  const wifi::WifiConfig current;
  auto parsed = parse_wifi(wifi_json(R"("ssid":"renamed","channel":7)"), current);
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_STRING("renamed", parsed.value().ssid);
  TEST_ASSERT_EQUAL(7, parsed.value().channel);
  TEST_ASSERT_EQUAL_STRING(current.password, parsed.value().password);
  TEST_ASSERT_FALSE(parsed.value().is_open());
}

void test_wifi_empty_password_opens_the_network(void) {
  const wifi::WifiConfig current;
  auto parsed = parse_wifi(wifi_json(R"("ssid":"x","channel":1,"password":"")"), current);
  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_TRUE(parsed.value().is_open());
  TEST_ASSERT_TRUE(parsed.value().is_valid());
}

void test_default_wifi_config_is_valid(void) {
  const wifi::WifiConfig config;
  TEST_ASSERT_TRUE(config.is_valid());
  TEST_ASSERT_FALSE(config.is_open());
  TEST_ASSERT_EQUAL_STRING(CONFIG_TESLASYNTH_DEVICE_NAME, config.ssid);
}

void test_encoders_emit_the_document_version(void) {
  const auto hw = codec::encode(hardware::HardwareConfig()).print();
  const auto synth = codec::encode(AppConfig()).print();
  const auto wifi_api = codec::encode(wifi::WifiConfig()).print();
  const auto wifi_stored = codec::encode_stored(wifi::WifiConfig()).print();

  for (const char *json : {hw.value, synth.value, wifi_api.value, wifi_stored.value})
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(json, "\"version\""), "every document carries a version");
}

void test_has_version_matches_only_the_current_version(void) {
  const auto json = codec::encode(hardware::HardwareConfig()).print();
  JSONParser parser(json.value);
  TEST_ASSERT_TRUE(codec::has_version(parser, hardware::HardwareConfig::current_version));
  TEST_ASSERT_FALSE(codec::has_version(parser, hardware::HardwareConfig::current_version + 1));

  JSONParser missing(R"({"output":{"channels":[4,5,6,7]}})");
  TEST_ASSERT_FALSE(codec::has_version(missing, hardware::HardwareConfig::current_version));
}

void test_stored_wifi_roundtrips_the_password(void) {
  wifi::WifiConfig original;
  strncpy(original.ssid, "MyCoil", sizeof(original.ssid));
  strncpy(original.password, "hunter2hunter2", sizeof(original.password));
  original.channel = 11;

  const auto json = codec::encode_stored(original).print();
  JSONParser parser(json.value);
  auto parsed = codec::parse_wificonfig(parser, wifi::WifiConfig());

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_STRING("MyCoil", parsed.value().ssid);
  TEST_ASSERT_EQUAL_STRING("hunter2hunter2", parsed.value().password);
  TEST_ASSERT_EQUAL(11, parsed.value().channel);
  TEST_ASSERT_TRUE(parsed.value().is_valid());
}

void test_stored_open_network_roundtrips(void) {
  wifi::WifiConfig original;
  original.password[0] = '\0';

  const auto json = codec::encode_stored(original).print();
  JSONParser parser(json.value);
  auto parsed = codec::parse_wificonfig(parser, wifi::WifiConfig());

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_TRUE(parsed.value().is_open());
}

void test_synth_roundtrips_through_storage_encoding(void) {
  AppConfig original;
  original.synth().tuning = teslasynth::core::Hertz(432);
  original.channels()[2].notes = 2;
  original.channels()[2].max_duty = teslasynth::midisynth::DutyCycle(7.5);

  const auto json = codec::encode(original).print();
  JSONParser parser(json.value);
  auto parsed = codec::parse_appconfig(parser);

  TEST_ASSERT_TRUE_MESSAGE(parsed, parsed ? "" : parsed.error());
  TEST_ASSERT_EQUAL_UINT8(2, parsed.value().channels()[2].notes);
  TEST_ASSERT_TRUE(parsed.value().channels()[2].max_duty == original.channels()[2].max_duty);
  TEST_ASSERT_TRUE(parsed.value().synth().tuning == original.synth().tuning);
  TEST_ASSERT_TRUE(parsed.value().is_valid());
}

void test_parsers_reject_garbage(void) {
  const wifi::WifiConfig current;
  TEST_ASSERT_FALSE(parse_hw("{}"));
  TEST_ASSERT_FALSE(parse_synth("{}"));
  TEST_ASSERT_FALSE(parse_wifi("{}", current));
  TEST_ASSERT_FALSE(parse_hw("[]"));
  TEST_ASSERT_FALSE(parse_synth("[]"));
  TEST_ASSERT_FALSE(parse_wifi("[]", current));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hardware_accepts_exactly_the_output_count);
  RUN_TEST(test_hardware_rejects_too_many_channels);
  RUN_TEST(test_hardware_rejects_too_few_channels);
  RUN_TEST(test_hardware_rejects_out_of_range_pins);
  RUN_TEST(test_hardware_roundtrips);
  RUN_TEST(test_default_hardware_config_is_valid);
  RUN_TEST(test_synth_requires_every_channel);
  RUN_TEST(test_synth_rejects_out_of_range_notes);
  RUN_TEST(test_synth_rejects_out_of_range_duty);
  RUN_TEST(test_synth_requires_routing);
  RUN_TEST(test_parsed_synth_config_is_valid);
  RUN_TEST(test_wifi_ssid_bounds);
  RUN_TEST(test_wifi_password_bounds);
  RUN_TEST(test_wifi_channel_bounds);
  RUN_TEST(test_wifi_password_is_write_only);
  RUN_TEST(test_wifi_omitted_password_is_kept);
  RUN_TEST(test_wifi_empty_password_opens_the_network);
  RUN_TEST(test_default_wifi_config_is_valid);
  RUN_TEST(test_encoders_emit_the_document_version);
  RUN_TEST(test_has_version_matches_only_the_current_version);
  RUN_TEST(test_stored_wifi_roundtrips_the_password);
  RUN_TEST(test_stored_open_network_roundtrips);
  RUN_TEST(test_synth_roundtrips_through_storage_encoding);
  RUN_TEST(test_parsers_reject_garbage);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
  return 0;
}
