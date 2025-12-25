#include "application.hpp"
#include "argtable3/argtable3.h"
#include "config_data.hpp"
#include "config_parser.hpp"
#include "core.hpp"
#include "esp_console.h"
#include "freertos/task.h"
#include "instruments.hpp"
#include "midi_synth.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>

namespace teslasynth::app::cli {
using namespace synth;
using namespace app::configuration;
using namespace midisynth::config;

namespace {
namespace keys {
constexpr const char *max_on_time = "max-on-time";
constexpr const char *min_deadtime = "min-deadtime";
constexpr const char *max_duty = "max-duty";
constexpr const char *duty_window = "duty-window";
constexpr const char *tuning = "tuning";
constexpr const char *notes = "notes";
constexpr const char *instrument = "instrument";
constexpr const char *percussion = "percussion";
constexpr const char *routing = "routing";
}; // namespace keys

typedef struct {
  struct arg_lit *reload, *save, *reset;
  struct arg_str *value;
  struct arg_end *end;
} config_args_t;
config_args_t config_args;

bool set_duration(const std::string_view value, Duration16 &duration,
                  const char *key) {
  if (auto d = parser::parse_duration16(value)) {
    duration = *d;
    return true;
  } else {
    printf("Invalid duration value: %s "
           "for key: %s\n"
           "Valid values unsigned integer values "
           "followed by an optional time unit [us (default), ms]\n",
           std::string(value).c_str(), key);
    return false;
  }
}

bool set_instrument(const std::string_view value,
                    std::optional<uint8_t> &instrument) {
  auto d = parser::parse_number<uint8_t>(value);
  if (value == "-" || *d == 0) {
    instrument = std::nullopt;
  } else if (d && *d <= instruments_size) {
    instrument = *d - 1;
  } else {
    printf("Invalid instrument value: %s\n"
           "Valid values are optional integer numbers, negative values are "
           "considered as no value. Max allowed value is %du\n",
           std::string(value).c_str(), instruments_size);
    return false;
  }
  return true;
}

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

void print_channel_config(uint8_t nr, const ChannelConfig &config) {
  printf("Channel[%u] configuration:\n"
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
    print_channel_config(i, config.channel(i));
  }
  print_routing_config(config.routing());

  return 0;
}

int update_config(AppConfig &config, const char *val) {
  const auto result = parser::parse_config_value(val);
  if (!result) {
    printf("Invalid key value pair '%s'", val);
    return 1;
  }

  const auto key = result.value().key, value = result.value().value;
  const auto index = result.value().channel;
  if (index == 0) {
    if (key == keys::instrument) {
      if (!set_instrument(value, config.synth().instrument)) {
        return 3;
      }
    } else if (key == keys::tuning) {
      if (auto t = parser::parse_hertz(value)) {
        config.synth().tuning = *t;
      } else {
        printf("Invalid frequency value: %s\n"
               "Valid values are floating point numbers followed by an "
               "optional unit "
               "[Hz]\n",
               std::string(value).c_str());
        return 3;
      }
    } else if (key == keys::percussion) {
      auto n = parser::parse_number<int>(value);
      if (n.has_value()) {
        config.routing().percussion = *n > 0;
      } else {
        printf("Invalid percussion: %s, use 0 or less to turn off, larger "
               "values to turn on",
               std::string(value).c_str());
        return 3;
      }
    } else {
      printf("Unknown key: %s\n", std::string(key).c_str());
      return 3;
    }
  } else {
    if (key == keys::max_on_time) {
      if (index > config.channels_size()) {
        printf("Invalid channel number %d\n", index);
        return 2;
      }
      if (!set_duration(value, config.channel(index - 1).max_on_time,
                        keys::max_on_time)) {
        return 3;
      }
    } else if (key == keys::min_deadtime) {
      if (!set_duration(value, config.channel(index - 1).min_deadtime,
                        keys::min_deadtime)) {
        return 3;
      }
    } else if (key == keys::notes) {
      if (index > config.channels_size()) {
        printf("Invalid channel number %d\n", index);
        return 2;
      }
      auto n = parser::parse_number<uint8_t>(value);
      if (n.has_value() && n < config.channels_size()) {
        config.channel(index - 1).notes = *n;
      } else {
        printf("Invalid frequency: %s", std::string(value).c_str());
        return 3;
      }
    } else if (key == keys::max_duty) {
      if (index > config.channels_size()) {
        printf("Invalid channel number %d\n", index);
        return 2;
      }
      auto n = parser::parse_number<float>(value);
      if (n.has_value()) {
        config.channel(index - 1).max_duty = DutyCycle(*n);
      } else {
        printf("Invalid dutycycle: %s", std::string(value).c_str());
        return 3;
      }
    } else if (key == keys::duty_window) {
      if (index > config.channels_size()) {
        printf("Invalid channel number %d\n", index);
        return 2;
      }
      if (!set_duration(value, config.channel(index - 1).duty_window,
                        keys::duty_window)) {
        return 3;
      }
    } else if (key == keys::instrument) {
      if (index > config.channels_size()) {
        printf("Invalid channel number %d\n", index);
        return 2;
      }
      if (!set_instrument(value, config.channel(index - 1).instrument)) {
        return 3;
      }
    } else if (key == keys::routing) {
      auto n = parser::parse_number<int>(value);
      if (n.has_value()) {
        config.routing().mapping[index - 1] = *n - 1;
      } else {
        printf("Invalid mapping value: %s, use 0 or less to turn off, or "
               "a valid output number to turn on",
               std::string(value).c_str());
        return 3;
      }
    } else {
      printf("Unknown key: %s\n", std::string(key).c_str());
      return 3;
    }
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
  };
  for (auto &cmd : commands)
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

} // namespace teslasynth::app::cli
