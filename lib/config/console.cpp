// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "console.hpp"
#include "config_patch_update.hpp"
#include <cstdarg>
#include <cstdio>

namespace teslasynth::app::console {

using teslasynth::midisynth::ChannelConfig;

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
} // namespace keys

__attribute__((format(printf, 2, 3))) void appendf(std::string &out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  va_list measure;
  va_copy(measure, args);
  const int length = vsnprintf(nullptr, 0, fmt, measure);
  va_end(measure);

  if (length > 0) {
    const size_t offset = out.size();
    out.resize(offset + length);
    vsnprintf(out.data() + offset, length + 1, fmt, args);
  }
  va_end(args);
}

std::string instrument_value(const std::optional<uint8_t> &instrument) {
  return instrument.has_value() ? std::to_string(*instrument + 1) : std::string();
}

void append_line(std::string &out, const char *line) {
  if (!out.empty())
    out += "\n";
  out += line;
}

void append_unconfigured(std::string &out, const char *scope, const char *reason) {
  if (reason)
    appendf(out, "!! %s configuration not in use: %s\n", scope, reason);
}

void append_output(std::string &out, uint8_t nr, const ChannelConfig &config) {
  appendf(out,
          "Output[%u] configuration:\n"
          "\t%s = %u\n"
          "\t%s = %s\n"
          "\t%s = %s\n"
          "\t%s = %s\n"
          "\t%s = %s\n"
          "\t%s = %s\n"
          "\t%s = <%s>\n",
          nr + 1, keys::notes, config.notes, keys::max_on_time,
          std::string(config.max_on_time).c_str(), keys::min_deadtime,
          std::string(config.min_deadtime).c_str(), keys::max_duty,
          std::string(config.max_duty).c_str(), keys::duty_window,
          std::string(config.duty_window).c_str(), keys::pulse_resolution,
          std::string(config.pulse_resolution).c_str(), keys::instrument,
          instrument_value(config.instrument).c_str());
}

void append_routing(std::string &out, const AppMidiRoutingConfig &config) {
  appendf(out, "Routing configuration:\n\t%s = %s", keys::percussion,
          config.percussion ? "on" : "off");

  for (int i = 0; i < static_cast<int>(config.mapping.size()); i++) {
    if (i % 4 == 0)
      out += "\n\t";
    auto target = config.mapping[i].value();
    if (target)
      appendf(out, "[%d -> %d] ", i + 1, *target + 1);
    else
      appendf(out, "[%d -> x] ", i + 1);
  }
  out += "\n";
}

} // namespace

ConfigOutcome run_config_command(const AppConfig &current, const ConfigRequest &request) {
  ConfigOutcome outcome;
  outcome.config = current;
  outcome.save = request.save;
  outcome.reload = request.reload;

  if (request.reset) {
    if (!request.maintenance) {
      outcome.code = 2;
      appendf(outcome.message,
              "Refusing to reset: factory defaults set the duty limit to %g%% and would be "
              "applied to live outputs.\nReboot into maintenance mode first (see the "
              "'maintenance' command).",
              static_cast<double>(ChannelConfig::default_max_duty));
      return outcome;
    }
    outcome.config = AppConfig();
    outcome.message = "Reset!";
  }

  for (const auto &value : request.values) {
    auto updated = midisynth::config::patch::update(value, outcome.config);
    if (!updated) {
      outcome.code = 2;
      outcome.message = "Error: " + updated.error();
      outcome.config = current;
      outcome.apply = false;
      return outcome;
    }
  }

  if (request.reset || !request.values.empty()) {
    outcome.apply = true;
    if (!request.values.empty()) {
      if (!outcome.message.empty())
        outcome.message += "\n";
      appendf(outcome.message, "Updated %u config values!",
              static_cast<unsigned>(request.values.size()));
    }
    append_line(outcome.message, request.save ? "Saved!" : "Not saved; add -s to persist.");
  }

  return outcome;
}

std::string synth_report(const AppConfig &config, const char *unconfigured_reason) {
  std::string out;
  append_unconfigured(out, "Synth", unconfigured_reason);

  appendf(out,
          "Synth configuration:\n"
          "\t%s = %s\n"
          "\t%s = <%s>\n",
          keys::tuning, std::string(config.synth().tuning).c_str(), keys::instrument,
          instrument_value(config.synth().instrument).c_str());

  uint8_t nr = 0;
  for (const auto &channel : config.channels())
    append_output(out, nr++, channel);

  append_routing(out, config.routing());
  return out;
}

std::string hardware_report(const configuration::hardware::HardwareConfig &config,
                            const char *unconfigured_reason) {
  std::string out;
  append_unconfigured(out, "Hardware", unconfigured_reason);

  appendf(out, "Number of outputs: %d\n", config.output.size);
  uint8_t nr = 0;
  for (const auto &channel : config.output.channels) {
    if (channel.pin == gpio_num_t::GPIO_NUM_NC)
      appendf(out, "\tOutput#: %u not used.\n", nr++);
    else
      appendf(out, "\tOutput#: %u connected to GPIO %d\n", nr++, channel.pin);
  }

  if (config.input.maintenance == gpio_num_t::GPIO_NUM_NC)
    out += "Input button: not configured.\n";
  else
    appendf(out, "Input button: GPIO %d\n", config.input.maintenance);

  if (config.led.pin == gpio_num_t::GPIO_NUM_NC)
    out += "Status LED: not configured.\n";
  else
    appendf(out, "Status LED: GPIO %d, active %s\n", config.led.pin,
            config.led.logic == configuration::hardware::LogicType::active_high ? "high" : "low");

  return out;
}

std::string wifi_report(const configuration::wifi::WifiConfig &config) {
  std::string out;
  appendf(out, "SSID: %s\nChannel: %d\nSecurity: %s\n", config.ssid, config.channel,
          config.is_open() ? "open" : "protected");
  return out;
}

} // namespace teslasynth::app::console
