// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "api.hpp"
#include "codec.hpp"
#include <map>
#include <string>
#include <unity.h>

using namespace teslasynth::app;
using namespace teslasynth::app::api;
namespace store = teslasynth::app::configuration::store;

namespace {

std::map<std::string, std::string> backing;
bool save_fails = false;
int applied = 0;
AppConfig last_applied;

std::optional<std::string> fake_load(const char *scope) {
  auto it = backing.find(scope);
  if (it == backing.end())
    return {};
  return it->second;
}

bool fake_save(const char *scope, const char *json) {
  if (save_fails)
    return false;
  backing[scope] = json;
  return true;
}

void record_apply(const AppConfig &config) {
  applied++;
  last_applied = config;
}

bool contains(const Response &res, const char *needle) {
  return res.body.find(needle) != std::string::npos;
}

std::string channel_json(int notes = 4, double duty = 5.0) {
  return R"({"notes":)" + std::to_string(notes) +
         R"(,"instrument":null,"max-on-time":100,"min-deadtime":100,"duty-window":10000,)" +
         R"("max-duty":)" + std::to_string(duty) + "}";
}

std::string synth_body() {
  std::string channels;
  for (int i = 0; i < 4; i++) {
    if (i)
      channels += ",";
    channels += channel_json();
  }
  return R"({"tuning":440,"instrument":null,"channels":[)" + channels +
         R"(],"routing":{"percussion":false,)" +
         R"("mapping":[0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1]}})";
}

const char *hardware_body() {
  return R"({"output":{"channels":[4,5,6,7]},"input":{"pin":0},)"
         R"("led":{"pin":8,"active-high":true}})";
}

} // namespace

void setUp(void) {
  backing.clear();
  save_fails = false;
  applied = 0;
  store::install(&fake_load, &fake_save);
}

void test_status_reports_configured_when_no_reasons(void) {
  auto res = sys_status(false, false, nullptr, nullptr);
  TEST_ASSERT_EQUAL(status_code::ok, res.status);
  TEST_ASSERT_TRUE(contains(res, "\"configured\":\ttrue"));
}

void test_status_reports_each_reason(void) {
  auto res = sys_status(true, false, "synth broke", "hardware broke");
  TEST_ASSERT_EQUAL(status_code::ok, res.status);
  TEST_ASSERT_TRUE(contains(res, "\"configured\":\tfalse"));
  TEST_ASSERT_TRUE(contains(res, "\"maintenance\":\ttrue"));
  TEST_ASSERT_TRUE(contains(res, "synth broke"));
  TEST_ASSERT_TRUE(contains(res, "hardware broke"));
}

void test_empty_body_is_rejected(void) {
  TEST_ASSERT_EQUAL(status_code::bad_request, hardware_put("").status);
  TEST_ASSERT_EQUAL(status_code::bad_request, wifi_put("").status);
  TEST_ASSERT_EQUAL(status_code::bad_request, synth_put("", nullptr).status);
}

void test_oversized_body_is_rejected(void) {
  const std::string huge(max_body_length + 1, 'x');
  TEST_ASSERT_EQUAL(status_code::too_large, hardware_put(huge).status);
  TEST_ASSERT_FALSE(body_length_ok(0));
  TEST_ASSERT_TRUE(body_length_ok(1));
  TEST_ASSERT_TRUE(body_length_ok(max_body_length));
  TEST_ASSERT_FALSE(body_length_ok(max_body_length + 1));
}

void test_malformed_json_is_rejected(void) {
  auto res = hardware_put("{not json");
  TEST_ASSERT_EQUAL(status_code::bad_request, res.status);
  TEST_ASSERT_TRUE(contains(res, "Invalid JSON"));
}

void test_decoder_error_is_returned_to_the_client(void) {
  auto res = hardware_put(R"({"output":{"channels":[4,5,6,7,8]},"input":{"pin":0},)"
                          R"("led":{"pin":8,"active-high":true}})");
  TEST_ASSERT_EQUAL(status_code::bad_request, res.status);
  TEST_ASSERT_TRUE(contains(res, "Cannot be more than max outputs"));
}

void test_hardware_put_persists_and_echoes(void) {
  auto res = hardware_put(hardware_body());
  TEST_ASSERT_EQUAL(status_code::ok, res.status);
  TEST_ASSERT_TRUE(contains(res, "\"version\""));
  TEST_ASSERT_EQUAL(1, backing.count("hardware"));

  auto fetched = hardware_get();
  TEST_ASSERT_EQUAL(status_code::ok, fetched.status);
  TEST_ASSERT_TRUE(contains(fetched, "\"channels\""));
}

void test_persist_failure_is_a_server_error(void) {
  save_fails = true;
  TEST_ASSERT_EQUAL(status_code::server_error, hardware_put(hardware_body()).status);
  TEST_ASSERT_EQUAL(status_code::server_error, hardware_reset().status);
  TEST_ASSERT_EQUAL(status_code::server_error, wifi_reset().status);
  TEST_ASSERT_EQUAL(status_code::server_error, synth_reset(nullptr).status);
}

void test_hardware_reset_persists_defaults(void) {
  TEST_ASSERT_EQUAL(status_code::ok, hardware_put(hardware_body()).status);
  TEST_ASSERT_EQUAL(status_code::ok, hardware_reset().status);

  configuration::hardware::HardwareConfig loaded;
  TEST_ASSERT_TRUE(configuration::hardware::read(loaded));
  TEST_ASSERT_EQUAL(configuration::hardware::HardwareConfig().output.channels[0].pin,
                    loaded.output.channels[0].pin);
}

void test_synth_reset_persists_defaults(void) {
  TEST_ASSERT_EQUAL(status_code::ok, synth_put(synth_body(), nullptr).status);

  auto res = synth_reset(&record_apply);
  TEST_ASSERT_EQUAL(status_code::ok, res.status);
  TEST_ASSERT_EQUAL(1, applied);

  AppConfig loaded;
  TEST_ASSERT_TRUE(configuration::synth::read(loaded));
  TEST_ASSERT_TRUE(loaded == AppConfig());
}

void test_synth_put_applies_before_persisting(void) {
  auto res = synth_put(synth_body(), &record_apply);
  TEST_ASSERT_EQUAL(status_code::ok, res.status);
  TEST_ASSERT_EQUAL(1, applied);
  TEST_ASSERT_EQUAL(1, backing.count("synth"));
}

void test_synth_put_does_not_apply_a_rejected_body(void) {
  auto res = synth_put("{}", &record_apply);
  TEST_ASSERT_EQUAL(status_code::bad_request, res.status);
  TEST_ASSERT_EQUAL(0, applied);
  TEST_ASSERT_EQUAL(0, backing.count("synth"));
}

void test_wifi_put_keeps_the_stored_password(void) {
  configuration::wifi::WifiConfig stored;
  strncpy(stored.password, "hunter2hunter2", sizeof(stored.password));
  TEST_ASSERT_TRUE(configuration::wifi::persist(stored));

  auto res = wifi_put(R"({"ssid":"renamed","channel":7})");
  TEST_ASSERT_EQUAL(status_code::ok, res.status);

  configuration::wifi::WifiConfig loaded;
  TEST_ASSERT_TRUE(configuration::wifi::read(loaded));
  TEST_ASSERT_EQUAL_STRING("renamed", loaded.ssid);
  TEST_ASSERT_EQUAL(7, loaded.channel);
  TEST_ASSERT_EQUAL_STRING("hunter2hunter2", loaded.password);
}

void test_wifi_responses_never_carry_the_password(void) {
  configuration::wifi::WifiConfig stored;
  strncpy(stored.password, "hunter2hunter2", sizeof(stored.password));
  TEST_ASSERT_TRUE(configuration::wifi::persist(stored));

  for (const auto &res : {wifi_get(), wifi_put(R"({"ssid":"x","channel":1})"), wifi_reset()}) {
    TEST_ASSERT_EQUAL(status_code::ok, res.status);
    TEST_ASSERT_FALSE_MESSAGE(contains(res, "hunter2hunter2"),
                              "a response leaked the stored password");
    TEST_ASSERT_TRUE(contains(res, "password-set"));
  }
}

void test_wifi_put_rejects_a_short_password(void) {
  auto res = wifi_put(R"({"ssid":"x","channel":1,"password":"short"})");
  TEST_ASSERT_EQUAL(status_code::bad_request, res.status);
  TEST_ASSERT_TRUE(contains(res, "at least 8 characters"));
}

void test_get_on_an_empty_store_returns_defaults(void) {
  auto hw = hardware_get();
  TEST_ASSERT_EQUAL(status_code::ok, hw.status);
  TEST_ASSERT_TRUE(contains(hw, "\"channels\""));

  auto wifi = wifi_get();
  TEST_ASSERT_EQUAL(status_code::ok, wifi.status);
  TEST_ASSERT_TRUE(contains(wifi, CONFIG_TESLASYNTH_DEVICE_NAME));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_status_reports_configured_when_no_reasons);
  RUN_TEST(test_status_reports_each_reason);
  RUN_TEST(test_empty_body_is_rejected);
  RUN_TEST(test_oversized_body_is_rejected);
  RUN_TEST(test_malformed_json_is_rejected);
  RUN_TEST(test_decoder_error_is_returned_to_the_client);
  RUN_TEST(test_hardware_put_persists_and_echoes);
  RUN_TEST(test_persist_failure_is_a_server_error);
  RUN_TEST(test_hardware_reset_persists_defaults);
  RUN_TEST(test_synth_reset_persists_defaults);
  RUN_TEST(test_synth_put_applies_before_persisting);
  RUN_TEST(test_synth_put_does_not_apply_a_rejected_body);
  RUN_TEST(test_wifi_put_keeps_the_stored_password);
  RUN_TEST(test_wifi_responses_never_carry_the_password);
  RUN_TEST(test_wifi_put_rejects_a_short_password);
  RUN_TEST(test_get_on_an_empty_store_returns_defaults);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
  return 0;
}
