// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "application.hpp"
#include "argtable3/argtable3.h"
#include "config_data.hpp"
#include "../../status.hpp"
#include "config_patch_update.hpp"
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

#define cstr(value) std::string(value).c_str()
#define instrument_value(config)                                                                   \
  (config.instrument.has_value() ? std::to_string(*config.instrument + 1).c_str() : "")

#define read_duration(out)                                                                         \
  if (!parse_duration(value, out)) {                                                               \
    return invalid_duration(value);                                                                \
  }

void print_output_config(uint8_t nr, const ChannelConfig &config) {
  printf("Output[%u] configuration:\n"
         "\t%s = %u\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         nr + 1, keys::notes, config.notes, keys::max_on_time, cstr(config.max_on_time),
         keys::min_deadtime, cstr(config.min_deadtime), keys::max_duty, cstr(config.max_duty),
         keys::duty_window, cstr(config.duty_window), keys::pulse_resolution,
         cstr(config.pulse_resolution), keys::instrument, instrument_value(config));
}

void print_routing_config(const AppMidiRoutingConfig &config) {
  printf("Routing configuration:\n"
         "\t%s = %s",
         keys::percussion, config.percussion ? "on" : "off");
  for (auto i = 0; i < config.mapping.size(); i++) {
    if (i % 4 == 0)
      printf("\n\t");
    auto v = config.mapping[i].value();
    if (v)
      printf("[%d -> %d] ", i + 1, *v + 1);
    else
      printf("[%d -> x] ", i + 1);
  }
  printf("\n");
}

void print_unconfigured(const char *scope, const char *reason) {
  if (reason)
    printf("!! %s configuration not in use: %s\n", scope, reason);
}

int print_config(AppConfig &config) {
  print_unconfigured("Synth", status::get().synth);
  printf("Synth configuration:\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         keys::tuning, cstr(config.synth().tuning), keys::instrument,
         instrument_value(config.synth()));

  uint8_t nr = 0;
  for (const auto &channel : config.channels()) {
    print_output_config(nr++, channel);
  }
  print_routing_config(config.routing());

  return 0;
}

int update_config(AppConfig &config, const char *val) {
  const auto res = config::patch::update(val, config);
  if (!res) {
    printf("\nError: %s\n", res.error().c_str());
    return 2;
  }
  return 0;
}

int config_cmd(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&config_args);

  if (nerrors != 0) {
    arg_print_errors(stderr, config_args.end, argv[0]);
    return 0;
  }

  const bool save = config_args.save->count != 0, reload = config_args.reload->count != 0,
             reset = config_args.reset->count != 0;
  const uint8_t value_count = config_args.value->count;

  AppConfig config = handle_.config_read();

  if (reset) {
    if (!maintenance_) {
      printf("Refusing to reset: factory defaults lift the duty limit to %g%% and would be "
             "applied to live outputs.\nReboot into maintenance mode first (see the "
             "'maintenance' command).\n",
             static_cast<double>(ChannelConfig::default_max_duty));
      return 2;
    }
    config = AppConfig();
    printf("Reset!\n");
  }

  for (auto i = 0; i < value_count; i++) {
    const auto res = update_config(config, config_args.value->sval[i]);
    if (res != 0)
      return res;
  }

  if (reset || value_count > 0) {
    handle_.config_set(config, reload, save);
    if (value_count > 0)
      printf("Updated %d config values!\n", value_count);
    if (save)
      printf("Saved!\n");
    else
      printf("Not saved; add -s to persist.\n");
  }

  print_config(config);

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
  print_unconfigured("Hardware", status::get().hardware);

  configuration::hardware::HardwareConfig hconfig;
  configuration::hardware::read(hconfig);

  printf("Number of outputs: %d\n", hconfig.output.size);
  for (auto i = 0; i < hconfig.output.size; i++) {
    const auto pin = hconfig.output.channels[i].pin;
    if (pin == GPIO_NUM_NC)
      printf("\tOutput#: %d not used.\n", i);
    else
      printf("\tOutput#: %d connected to GPIO %d\n", i, pin);
  }

  printf("Input button: ");
  if (hconfig.input.maintenance == GPIO_NUM_NC)
    printf("not configured.\n");
  else
    printf("GPIO %d\n", hconfig.input.maintenance);

  printf("Status LED: ");
  if (hconfig.led.pin == GPIO_NUM_NC)
    printf("not configured.\n");
  else
    printf("GPIO %d, active %s\n", hconfig.input.maintenance,
           static_cast<bool>(hconfig.led.logic) ? "high" : "low");
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
    if (configuration::wifi::persist(wconfig) != ESP_OK) {
      printf("Couldn't reset WiFi configuration.\n");
      return 1;
    }
    printf("WiFi configuration reset to factory defaults.\n");
  } else {
    configuration::wifi::read(wconfig);
  }

  printf("SSID: %s\n", wconfig.ssid);
  printf("Channel: %d\n", wconfig.channel);
  printf("Security: %s\n", wconfig.is_open() ? "open" : "protected");
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
