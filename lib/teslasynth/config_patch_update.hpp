#pragma once

#include "./config_parser.hpp"
#include "channel_mapping.hpp"
#include "config_data.hpp"
#include "result.hpp"
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <midi_synth.hpp>
#include <optional>
#include <string_view>

namespace teslasynth::midisynth {
namespace config::patch {
using namespace config::parser;

using ConfigPath = parser::SplittedString;
using ConfigValue = std::string_view;
template <typename T> using Parser = helpers::Result<T, std::string>;

std::string invalid_value(const ConfigPath &path, const ConfigValue value,
                          const char description[] = "");
std::string invalid_key(const ConfigPath &path, int idx = -1,
                        const char description[] = "");

constexpr static struct Unit {
} unit;

Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    SynthConfig &config);
Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    ChannelConfig &config);

template <std::uint8_t OUTPUT>
Parser<OutputNumberOpt<OUTPUT>> output_number(const ConfigPath &path,
                                              const ConfigValue value) {
  auto o = parser::parse_number<int8_t>(value);
  if (o)
    return OutputNumberOpt<OUTPUT>(*o - 1);

  return invalid_value(path, value,
                       "Must be an output number, numbers out of range are "
                       "considered no output");
}

template <std::uint8_t OUTPUT>
Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    MidiRoutingConfig<OUTPUT> &config) {
  if (path.size() < 2)
    return invalid_key(path);

  const auto key = path[1];
  if (key == "percussion") {
    if (value == "y")
      config.percussion = true;
    return unit;
  } else if (key == "channel" && path.size() == 3) {
    const auto selector = path[2];
    if (selector == "*") {
      auto _r = output_number<OUTPUT>(path, value);
      if (!_r) return _r.error();
      for (auto &m : config.mapping)
        m = _r.value();
      return unit;
    }

    const auto idx = parser::parse_number<uint8_t>(selector);
    if (idx && *idx > 0 && *idx <= config.mapping.size()) {
      auto _r = output_number<OUTPUT>(path, value);
      if (!_r) return _r.error();
      config.mapping[*idx - 1] = _r.value();
      return unit;
    }
  }

  return invalid_key(path, 1);
}

template <std::uint8_t OUTPUT>
Parser<Unit> update_output(const ConfigPath &path, const ConfigValue value,
                           Configuration<OUTPUT> &config) {
  if (path.size() < 2)
    return invalid_key(path);

  const auto selector = path[1];
  if (selector == "*") {
    for (auto i = 0; i < config.channels_size(); i++) {
      auto _r = update(path, value, config.channel(i));
      if (!_r) return _r.error();
    }
    return unit;
  }

  const auto idx = parser::parse_number<uint8_t>(selector);
  if (idx && *idx <= config.channels_size()) {
    auto _r = update(path, value, config.channel(*idx - 1));
    if (!_r) return _r.error();
    return unit;
  }
  return invalid_key(path, 1);
}

template <std::uint8_t OUTPUT>
Parser<Unit> update(const ConfigPath &path, const ConfigValue value,
                    Configuration<OUTPUT> &config) {
  if (path.size() < 1)
    return std::string("No config path!");
  const auto &key = path[0];
  if (key == "synth") {
    auto _r = update(path, value, config.synth());
    if (!_r) return _r.error();
  } else if (key == "output") {
    auto _r = update_output(path, value, config);
    if (!_r) return _r.error();
  } else if (key == "routing") {
    auto _r = update(path, value, config.routing());
    if (!_r) return _r.error();
  } else {
    return invalid_key(path, 1);
  }

  return unit;
}
template <std::uint8_t OUTPUT>
Parser<Unit> update(const std::string_view args,
                    Configuration<OUTPUT> &config) {
  const auto arg_list = split(args, ' ');

  for (auto &arg : arg_list) {
    const auto exp = split(arg, '=');
    if (exp.size() < 2)
      return std::string("expressions must be like key1=[key2|...|keyn=]value, "
                         "at least one key, with a right side value");
    auto value = exp[exp.size() - 1];
    for (int i = 0; i < exp.size() - 1; i++) {
      const auto path = split(exp[i], '.');
      auto _r = update(path, value, config);
      if (!_r) return _r.error();
    }
  }

  return unit;
}
} // namespace config::patch
} // namespace teslasynth::midisynth
