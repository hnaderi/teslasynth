// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "application.hpp"
#include "argtable3/argtable3.h"
#include "config_data.hpp"
#include "../../status.hpp"
#include "config_patch_update.hpp"
#include "console.hpp"
#include "hardware.hpp"
#include "wifi.hpp"
#include "esp_console.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <string>

namespace teslasynth::app::cli {
using namespace synth;
using namespace app::configuration;
using namespace midisynth::config;

namespace {
namespace keys {
constexpr char max_on_time[] = "max-on-time";
constexpr char min_deadtime[] = "min-deadtime";
constexpr char max_duty[] = "max-duty";
constexpr char duty_window[] = "duty-window";
constexpr char pulse_resolution[] = "pulse-resolution";
constexpr char tuning[] = "tuning";
constexpr char notes[] = "notes";
constexpr char instrument[] = "instrument";
constexpr char percussion[] = "percussion";
constexpr char routing[] = "routing";
}; // namespace keys

typedef struct {
  struct arg_lit *reload, *save, *reset;
  struct arg_str *value;
  struct arg_end *end;
} config_args_t;
config_args_t config_args;

UIHandle handle_;
bool maintenance_ = false;

int config_cmd(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&config_args);

  if (nerrors != 0) {
    arg_print_errors(stderr, config_args.end, argv[0]);
    return 0;
  }

  console::ConfigRequest request;
  request.save = config_args.save->count != 0;
  request.reload = config_args.reload->count != 0;
  request.reset = config_args.reset->count != 0;
  request.maintenance = maintenance_;
  for (auto i = 0; i < config_args.value->count; i++)
    request.values.emplace_back(config_args.value->sval[i]);

  auto outcome = console::run_config_command(handle_.config_read(), request);
  if (!outcome.message.empty())
    printf("%s\n", outcome.message.c_str());
  if (outcome.failed())
    return outcome.code;

  if (outcome.apply)
    handle_.config_set(outcome.config, outcome.reload, outcome.save);

  printf("%s", console::synth_report(outcome.config, status::get().synth).c_str());
  return 0;
}

int playbackoff_cmd(int, char **) {
  handle_.playback_off();
  return 0;
}

int device_limits_cmd(int, char **) {
  printf("Max notes: %d\n", ChannelConfig::max_notes);
  return 0;
}

int hwconfig_cmd(int, char **) {
  configuration::hardware::HardwareConfig hconfig;
  configuration::hardware::read(hconfig);

  printf("%s", console::hardware_report(hconfig, status::get().hardware).c_str());
  return 0;
}

struct {
  struct arg_lit *reset;
  struct arg_end *end;
} wificonfig_args;

int wificonfig_cmd(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&wificonfig_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, wificonfig_args.end, argv[0]);
    return 1;
  }

  configuration::Guard guard;
  configuration::wifi::WifiConfig wconfig;
  if (wificonfig_args.reset->count > 0) {
    if (!configuration::wifi::persist(wconfig)) {
      printf("Couldn't reset WiFi configuration.\n");
      return 1;
    }
    printf("WiFi configuration reset to factory defaults.\n");
  } else {
    configuration::wifi::read(wconfig);
  }

  printf("%s", console::wifi_report(wconfig).c_str());
  return 0;
}
} // namespace

void register_configuration_commands(UIHandle handle, bool maintenance) {
  handle_ = handle;
  maintenance_ = maintenance;

  config_args.save = arg_lit0("s", "save", "Persist configuration");
  config_args.reload = arg_lit0("r", "reload", "Reload configuration");
  config_args.reset = arg_lit0(nullptr, "reset", "Reset configuration values to defaults");
  config_args.value =
      arg_strn(nullptr, nullptr, "<key[:ch]=value>", 0, 50, "Set configuration value");
  config_args.end = arg_end(20);

  wificonfig_args.reset =
      arg_lit0(nullptr, "reset", "Reset WiFi configuration to firmware defaults");
  wificonfig_args.end = arg_end(2);

  const std::array commands = {
      esp_console_cmd_t{
          .command = "config",
          .help = "Configuration management",
          .func = config_cmd,
          .argtable = &config_args,
      },
      esp_console_cmd_t{
          .command = "off",
          .help = "All notes off instantly",
          .func = playbackoff_cmd,
      },
      esp_console_cmd_t{
          .command = "limits",
          .help = "Print device hard limits set in firmware at compile time",
          .func = device_limits_cmd,
      },
      esp_console_cmd_t{
          .command = "hwconfig",
          .help = "Print hardware configuration",
          .func = hwconfig_cmd,
      },
      esp_console_cmd_t{
          .command = "wificonfig",
          .help = "Print maintenance-mode WiFi access point configuration",
          .func = wificonfig_cmd,
          .argtable = &wificonfig_args,
      },
  };
  for (auto &cmd : commands)
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

} // namespace teslasynth::app::cli
