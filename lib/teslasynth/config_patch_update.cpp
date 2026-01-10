#include "./config_patch_update.hpp"
#include "config_data.hpp"
#include "config_parser.hpp"
#include <cstdint>

namespace teslasynth::midisynth {
namespace config::patch {

std::string invalid_value(const ConfigPath &path, const ConfigValue value,
                          const char description[]) {
  std::string msg = "Invalid config value: ";
  msg += value;
  msg += " for key: ";

  int count = 0;
  for (const auto &p : path) {
    msg += p;
    count++;
    if (count < path.size())
      msg += ".";
  }

  msg += "\n";
  msg += description;
  msg += "\n";
  return msg;
}

std::string invalid_key(const ConfigPath &path, int idx,
                        const char description[]) {
  const bool valid_idx = idx >= 0 && idx < path.size();
  std::string msg = "Invalid key: ";

  int count = 0;
  for (const auto &p : path) {
    const bool highlight = count == idx && valid_idx;
    if (highlight)
      msg += "[";
    msg += p;
    if (highlight)
      msg += "]";
    count++;
    if (count < path.size())
      msg += ".";
  }

  msg += "\n";
  msg += description;
  msg += "\n";
  return msg;
}

Parser<Duration16> duration16(const ConfigPath &path, const ConfigValue value) {
  if (auto d = parse_duration16(value))
    return *d;
  else {
    return invalid_value(
        path, value,
        "Valid values are unsigned integers "
        "followed by an optional time unit [us (default), ms]");
  }
}

Parser<Hertz> hertz(const ConfigPath &path, const ConfigValue value) {
  if (auto f = parse_hertz(value))
    return *f;
  else {
    return invalid_value(
        path, value,
        "Valid values are floating point numbers followed by an "
        "optional unit [Hz]\n");
  }
}

Parser<DutyCycle> duty(const ConfigPath &path, const ConfigValue value) {
  auto f = parser::parse_number<float>(value);
  if (*f && f > 0 && f <= 100)
    return DutyCycle(*f);
  else {
    return invalid_value(
        path, value, "Valid values are floating point numbers in (0, 100]\n");
  }
}

Parser<std::optional<uint8_t>> instrument(const ConfigPath &path,
                                          const ConfigValue value) {
  auto d = parser::parse_number<int8_t>(value);
  if (value == "-" || *d < 1) {
    return std::optional<uint8_t>{};
  } else if (d && *d <= instruments_size) {
    return std::optional<uint8_t>{*d - 1};
  } else {
    return invalid_value(
        path, value,
        "Valid values are optional instrument numbers, "
        "values less than 1 and - are considered as no value.\n");
  }
}

Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    SynthConfig &config) {
  if (path.size() != 2)
    return invalid_key(path);

  const auto &key = path[1];
  if (key == "tuning") {
    config.tuning = TRY(hertz(path, value));
  } else if (key == "instrument") {
    config.instrument = TRY(instrument(path, value));
  } else {
    return invalid_key(path, 1);
  }

  return unit;
}
Parser<uint8_t> notes(const ConfigPath &path, const ConfigValue value) {
  auto n = parser::parse_number<uint8_t>(value);
  if (n && *n < ChannelConfig::max_notes && *n >= 1) {
    return *n;
  }
  return invalid_value(path, value,
                       "Must be an unsigned integer between 1 and max notes.");
}

Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    ChannelConfig &config) {
  if (path.size() != 3)
    return invalid_key(path);
  const auto &key = path[2];

  if (key == "max-on-time") {
    config.max_on_time = TRY(duration16(path, value));
  } else if (key == "min-deadtime") {
    config.min_deadtime = TRY(duration16(path, value));
  } else if (key == "instrument") {
    config.instrument = TRY(instrument(path, value));
  } else if (key == "notes") {
    config.notes = TRY(notes(path, value));
  } else if (key == "duty-window") {
    config.duty_window = TRY(duration16(path, value));
  } else if (key == "max-duty") {
    config.max_duty = TRY(duty(path, value));
  } else {
    return invalid_key(path, 2);
  }

  return unit;
}

} // namespace config::patch
} // namespace teslasynth::midisynth
