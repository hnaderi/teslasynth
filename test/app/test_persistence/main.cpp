// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "codec.hpp"
#include "persistence.hpp"
#include <map>
#include <string>
#include <unity.h>

using namespace teslasynth::app::configuration;

namespace {

std::map<std::string, std::string> backing;
bool save_fails = false;

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

void reset_store() {
  backing.clear();
  save_fails = false;
  store::install(&fake_load, &fake_save);
}

} // namespace

void setUp(void) {
  reset_store();
}

void test_hardware_roundtrips_through_the_store(void) {
  hardware::HardwareConfig saved;
  saved.output.channels[0].pin = static_cast<gpio_num_t>(21);
  saved.led.pin = static_cast<gpio_num_t>(2);
  saved.led.logic = hardware::LogicType::active_low;

  TEST_ASSERT_TRUE(hardware::persist(saved));

  hardware::HardwareConfig loaded;
  auto outcome = hardware::read(loaded);
  TEST_ASSERT_TRUE_MESSAGE(outcome, outcome.reason);
  TEST_ASSERT_EQUAL(21, loaded.output.channels[0].pin);
  TEST_ASSERT_EQUAL(2, loaded.led.pin);
  TEST_ASSERT_TRUE(loaded.led.logic == hardware::LogicType::active_low);
}

void test_synth_roundtrips_through_the_store(void) {
  AppConfig saved;
  saved.synth().tuning = teslasynth::core::Hertz(432);
  saved.channels()[1].notes = 2;
  saved.channels()[1].max_duty = teslasynth::midisynth::DutyCycle(7.5);

  TEST_ASSERT_TRUE(synth::persist(saved));

  AppConfig loaded;
  auto outcome = synth::read(loaded);
  TEST_ASSERT_TRUE_MESSAGE(outcome, outcome.reason);
  TEST_ASSERT_EQUAL_UINT8(2, loaded.channels()[1].notes);
  TEST_ASSERT_TRUE(loaded.channels()[1].max_duty == saved.channels()[1].max_duty);
  TEST_ASSERT_TRUE(loaded.synth().tuning == saved.synth().tuning);
}

void test_wifi_roundtrips_the_password_through_the_store(void) {
  wifi::WifiConfig saved;
  strncpy(saved.ssid, "MyCoil", sizeof(saved.ssid));
  strncpy(saved.password, "hunter2hunter2", sizeof(saved.password));
  saved.channel = 11;

  TEST_ASSERT_TRUE(wifi::persist(saved));

  wifi::WifiConfig loaded;
  auto outcome = wifi::read(loaded);
  TEST_ASSERT_TRUE_MESSAGE(outcome, outcome.reason);
  TEST_ASSERT_EQUAL_STRING("MyCoil", loaded.ssid);
  TEST_ASSERT_EQUAL_STRING("hunter2hunter2", loaded.password);
  TEST_ASSERT_EQUAL(11, loaded.channel);
}

void test_empty_store_reports_unprovisioned(void) {
  hardware::HardwareConfig hw;
  auto hw_outcome = hardware::read(hw);
  TEST_ASSERT_FALSE(hw_outcome);
  TEST_ASSERT_EQUAL_STRING("Hardware is not provisioned", hw_outcome.reason);

  AppConfig cfg;
  auto synth_outcome = synth::read(cfg);
  TEST_ASSERT_FALSE(synth_outcome);
  TEST_ASSERT_NOT_NULL(synth_outcome.reason);
}

void test_failed_read_leaves_factory_defaults(void) {
  hardware::HardwareConfig hw;
  hw.output.channels[0].pin = static_cast<gpio_num_t>(21);

  auto outcome = hardware::read(hw);
  TEST_ASSERT_FALSE(outcome);
  TEST_ASSERT_EQUAL(hardware::HardwareConfig().output.channels[0].pin, hw.output.channels[0].pin);
}

void test_document_from_another_version_is_rejected(void) {
  hardware::HardwareConfig saved;
  TEST_ASSERT_TRUE(hardware::persist(saved));

  auto &doc = backing["hardware"];
  const auto pos = doc.find("\"version\":");
  TEST_ASSERT_TRUE(pos != std::string::npos);
  doc.replace(pos, 12, "\"version\":9");

  hardware::HardwareConfig loaded;
  auto outcome = hardware::read(loaded);
  TEST_ASSERT_FALSE(outcome);
  TEST_ASSERT_EQUAL_STRING("Stored configuration was written by another firmware version",
                           outcome.reason);
}

void test_document_without_a_version_is_rejected(void) {
  backing["hardware"] = R"({"output":{"channels":[4,5,6,7]},"input":{"pin":0},)"
                        R"("led":{"pin":8,"active-high":true}})";

  hardware::HardwareConfig loaded;
  TEST_ASSERT_FALSE(hardware::read(loaded));
}

void test_corrupt_document_is_rejected(void) {
  backing["wifi"] = "{not json";

  wifi::WifiConfig loaded;
  auto outcome = wifi::read(loaded);
  TEST_ASSERT_FALSE(outcome);
  TEST_ASSERT_EQUAL_STRING("Stored configuration is not valid JSON", outcome.reason);
}

void test_out_of_range_document_is_rejected(void) {
  wifi::WifiConfig saved;
  TEST_ASSERT_TRUE(wifi::persist(saved));
  auto &doc = backing["wifi"];
  const auto pos = doc.find("\"channel\":");
  TEST_ASSERT_TRUE(pos != std::string::npos);
  doc.replace(pos, 11, "\"channel\":99");

  wifi::WifiConfig loaded;
  TEST_ASSERT_FALSE(wifi::read(loaded));
  TEST_ASSERT_TRUE(loaded.is_valid());
}

void test_persist_reports_store_failure(void) {
  save_fails = true;
  TEST_ASSERT_FALSE(hardware::persist(hardware::HardwareConfig()));
  TEST_ASSERT_FALSE(synth::persist(AppConfig()));
  TEST_ASSERT_FALSE(wifi::persist(wifi::WifiConfig()));
}

void test_scopes_do_not_collide(void) {
  TEST_ASSERT_TRUE(hardware::persist(hardware::HardwareConfig()));
  TEST_ASSERT_TRUE(synth::persist(AppConfig()));
  TEST_ASSERT_TRUE(wifi::persist(wifi::WifiConfig()));
  TEST_ASSERT_EQUAL(3, backing.size());

  hardware::HardwareConfig hw;
  AppConfig cfg;
  wifi::WifiConfig wc;
  TEST_ASSERT_TRUE(hardware::read(hw));
  TEST_ASSERT_TRUE(synth::read(cfg));
  TEST_ASSERT_TRUE(wifi::read(wc));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hardware_roundtrips_through_the_store);
  RUN_TEST(test_synth_roundtrips_through_the_store);
  RUN_TEST(test_wifi_roundtrips_the_password_through_the_store);
  RUN_TEST(test_empty_store_reports_unprovisioned);
  RUN_TEST(test_failed_read_leaves_factory_defaults);
  RUN_TEST(test_document_from_another_version_is_rejected);
  RUN_TEST(test_document_without_a_version_is_rejected);
  RUN_TEST(test_corrupt_document_is_rejected);
  RUN_TEST(test_out_of_range_document_is_rejected);
  RUN_TEST(test_persist_reports_store_failure);
  RUN_TEST(test_scopes_do_not_collide);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
  return 0;
}
