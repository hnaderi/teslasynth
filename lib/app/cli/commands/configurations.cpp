#include "application.hpp"
#include "argtable3/argtable3.h"
#include "config_data.hpp"
#include "config_patch_update.hpp"
#include "esp_console.h"
#include "freertos/task.h"
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

#define cstr(value) std::string(value).c_str()
#define instrument_value(config)                                               \
  (config.instrument.has_value()                                               \
       ? std::to_string(*config.instrument + 1).c_str()                        \
       : "")

#define read_duration(out)                                                     \
  if (!parse_duration(value, out)) {                                           \
    return invalid_duration(value);                                            \
  }

void print_output_config(uint8_t nr, const ChannelConfig &config) {
  printf("Output[%u] configuration:\n"
         "\t%s = %u\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         nr + 1, keys::notes, config.notes, keys::max_on_time,
         cstr(config.max_on_time), keys::min_deadtime,
         cstr(config.min_deadtime), keys::max_duty, cstr(config.max_duty),
         keys::duty_window, cstr(config.duty_window), keys::instrument,
         instrument_value(config));
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

int print_config(AppConfig &config) {
  printf("Synth configuration:\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         keys::tuning, cstr(config.synth().tuning), keys::instrument,
         instrument_value(config.synth()));

  for (auto i = 0; i < config.channels_size(); i++) {
    print_output_config(i, config.channel(i));
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

  const bool save = config_args.save->count != 0,
             reload = config_args.reload->count != 0,
             reset = config_args.reset->count != 0;
  const uint8_t value_count = config_args.value->count;

  AppConfig config = handle_.config_read();

  if (reset) {
    config = AppConfig();
    printf("Reset!\n");
  }

  for (auto i = 0; i < value_count; i++) {
    const auto res = update_config(config, config_args.value->sval[i]);
    if (res != 0)
      return res;
  }

  if (value_count > 0) {
    handle_.config_set(config, reload, save);
    printf("Updated %d config values!\n", value_count);
    if (save)
      printf("Saved!\n");
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
} // namespace

void register_configuration_commands(UIHandle handle) {
  handle_ = handle;

  config_args.save = arg_lit0("s", "save", "Persist configuration");
  config_args.reload = arg_lit0("r", "reload", "Reload configuration");
  config_args.reset =
      arg_lit0(nullptr, "reset", "Reset configuration values to defaults");
  config_args.value = arg_strn(nullptr, nullptr, "<key[:ch]=value>", 0, 50,
                               "Set configuration value");
  config_args.end = arg_end(20);

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
  };
  for (auto &cmd : commands)
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

} // namespace teslasynth::app::cli
