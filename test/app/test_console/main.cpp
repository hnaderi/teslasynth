// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "console.hpp"
#include <string>
#include <unity.h>

using namespace teslasynth::app;
using namespace teslasynth::app::console;

namespace {

ConfigRequest reset_request(bool maintenance) {
  ConfigRequest request;
  request.reset = true;
  request.maintenance = maintenance;
  return request;
}

ConfigRequest values_request(std::vector<std::string> values) {
  ConfigRequest request;
  request.values = std::move(values);
  request.maintenance = true;
  return request;
}

bool contains(const std::string &text, const char *needle) {
  return text.find(needle) != std::string::npos;
}

AppConfig tuned(float hz) {
  AppConfig config;
  config.synth().tuning = teslasynth::core::Hertz(hz);
  return config;
}

} // namespace

void test_reset_hands_the_defaults_back_to_be_applied(void) {
  auto outcome = run_config_command(tuned(432), reset_request(true));

  TEST_ASSERT_FALSE(outcome.failed());
  TEST_ASSERT_TRUE_MESSAGE(outcome.apply, "a bare --reset must still be applied");
  TEST_ASSERT_TRUE(outcome.config == AppConfig());
  TEST_ASSERT_TRUE(contains(outcome.message, "Reset!"));
}

void test_reset_is_refused_outside_maintenance(void) {
  auto outcome = run_config_command(tuned(432), reset_request(false));

  TEST_ASSERT_TRUE(outcome.failed());
  TEST_ASSERT_FALSE(outcome.apply);
  TEST_ASSERT_TRUE(outcome.config == tuned(432));
  TEST_ASSERT_TRUE(contains(outcome.message, "Refusing to reset"));
}

void test_a_bare_query_changes_nothing(void) {
  ConfigRequest request;
  auto outcome = run_config_command(tuned(432), request);

  TEST_ASSERT_FALSE(outcome.failed());
  TEST_ASSERT_FALSE(outcome.apply);
  TEST_ASSERT_TRUE(outcome.message.empty());
}

void test_updates_are_applied(void) {
  auto outcome = run_config_command(AppConfig(), values_request({"synth.tuning=432"}));

  TEST_ASSERT_FALSE(outcome.failed());
  TEST_ASSERT_TRUE(outcome.apply);
  TEST_ASSERT_TRUE(outcome.config.synth().tuning == teslasynth::core::Hertz(432));
  TEST_ASSERT_TRUE(contains(outcome.message, "Updated 1 config values!"));
}

void test_a_failing_update_discards_the_earlier_ones(void) {
  auto outcome =
      run_config_command(AppConfig(), values_request({"synth.tuning=432", "synth.bogus=1"}));

  TEST_ASSERT_TRUE(outcome.failed());
  TEST_ASSERT_FALSE_MESSAGE(outcome.apply, "a partially applied command must not be applied");
  TEST_ASSERT_TRUE(outcome.config == AppConfig());
  TEST_ASSERT_TRUE(contains(outcome.message, "Error:"));
}

void test_save_flag_is_carried_through(void) {
  auto request = values_request({"synth.tuning=432"});
  request.save = true;
  auto outcome = run_config_command(AppConfig(), request);

  TEST_ASSERT_TRUE(outcome.save);
  TEST_ASSERT_TRUE(contains(outcome.message, "Saved!"));
}

void test_unsaved_changes_say_so(void) {
  auto outcome = run_config_command(AppConfig(), values_request({"synth.tuning=432"}));

  TEST_ASSERT_FALSE(outcome.save);
  TEST_ASSERT_TRUE(contains(outcome.message, "add -s to persist"));
}

void test_reload_flag_is_carried_through(void) {
  auto request = values_request({"synth.tuning=432"});
  request.reload = true;
  TEST_ASSERT_TRUE(run_config_command(AppConfig(), request).reload);
}

void test_reset_then_update_applies_both(void) {
  auto request = reset_request(true);
  request.values = {"synth.tuning=432"};
  auto outcome = run_config_command(tuned(220), request);

  TEST_ASSERT_FALSE(outcome.failed());
  TEST_ASSERT_TRUE(outcome.apply);
  TEST_ASSERT_TRUE(outcome.config.synth().tuning == teslasynth::core::Hertz(432));
  TEST_ASSERT_TRUE(contains(outcome.message, "Reset!"));
  TEST_ASSERT_TRUE(contains(outcome.message, "Updated 1 config values!"));
}

void test_synth_report_flags_an_unconfigured_device(void) {
  const auto clean = synth_report(AppConfig(), nullptr);
  TEST_ASSERT_FALSE(contains(clean, "not in use"));
  TEST_ASSERT_TRUE(contains(clean, "Synth configuration:"));
  TEST_ASSERT_TRUE(contains(clean, "Routing configuration:"));

  const auto flagged = synth_report(AppConfig(), "no synth configuration stored");
  TEST_ASSERT_TRUE(contains(flagged, "!! Synth configuration not in use"));
  TEST_ASSERT_TRUE(contains(flagged, "no synth configuration stored"));
}

void test_synth_report_lists_every_output(void) {
  const auto report = synth_report(AppConfig(), nullptr);
  for (uint8_t i = 1; i <= AppConfig().channels_size(); i++)
    TEST_ASSERT_TRUE(contains(report, ("Output[" + std::to_string(i) + "]").c_str()));
}

void test_hardware_report_names_the_led_pin(void) {
  configuration::hardware::HardwareConfig config;
  config.input.maintenance = static_cast<gpio_num_t>(3);
  config.led.pin = static_cast<gpio_num_t>(17);
  config.led.logic = configuration::hardware::LogicType::active_low;

  const auto report = hardware_report(config, nullptr);
  TEST_ASSERT_TRUE(contains(report, "Input button: GPIO 3"));
  TEST_ASSERT_TRUE_MESSAGE(contains(report, "Status LED: GPIO 17, active low"),
                           "the LED line must report the LED pin, not the input pin");
}

void test_hardware_report_marks_unused_pins(void) {
  configuration::hardware::HardwareConfig config;
  config.led.pin = gpio_num_t::GPIO_NUM_NC;
  config.input.maintenance = gpio_num_t::GPIO_NUM_NC;

  const auto report = hardware_report(config, "Hardware is not provisioned");
  TEST_ASSERT_TRUE(contains(report, "!! Hardware configuration not in use"));
  TEST_ASSERT_TRUE(contains(report, "Input button: not configured."));
  TEST_ASSERT_TRUE(contains(report, "Status LED: not configured."));
}

void test_wifi_report_never_shows_the_password(void) {
  configuration::wifi::WifiConfig config;
  strncpy(config.password, "hunter2hunter2", sizeof(config.password));

  const auto report = wifi_report(config);
  TEST_ASSERT_FALSE_MESSAGE(contains(report, "hunter2hunter2"), "the report leaked the password");
  TEST_ASSERT_TRUE(contains(report, "Security: protected"));

  config.password[0] = '\0';
  TEST_ASSERT_TRUE(contains(wifi_report(config), "Security: open"));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_reset_hands_the_defaults_back_to_be_applied);
  RUN_TEST(test_reset_is_refused_outside_maintenance);
  RUN_TEST(test_a_bare_query_changes_nothing);
  RUN_TEST(test_updates_are_applied);
  RUN_TEST(test_a_failing_update_discards_the_earlier_ones);
  RUN_TEST(test_save_flag_is_carried_through);
  RUN_TEST(test_unsaved_changes_say_so);
  RUN_TEST(test_reload_flag_is_carried_through);
  RUN_TEST(test_reset_then_update_applies_both);
  RUN_TEST(test_synth_report_flags_an_unconfigured_device);
  RUN_TEST(test_synth_report_lists_every_output);
  RUN_TEST(test_hardware_report_names_the_led_pin);
  RUN_TEST(test_hardware_report_marks_unused_pins);
  RUN_TEST(test_wifi_report_never_shows_the_password);
  UNITY_END();
}

int main(int argc, char **argv) {
  app_main();
  return 0;
}
