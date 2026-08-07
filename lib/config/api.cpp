// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "api.hpp"
#include "codec.hpp"
#include <optional>

namespace teslasynth::app::api {

using configuration::codec::Decoder;
using helpers::JSONParser;

namespace {

Response ok(helpers::JSONEncoder &&encoder) {
  auto json = std::move(encoder).print();
  return {status_code::ok, json.value ? std::string(json.value) : std::string()};
}

Response error(int status, const char *message) {
  return {status, std::string(message)};
}

Response saved_or_error(bool saved, helpers::JSONEncoder &&encoder) {
  if (!saved)
    return error(status_code::server_error, "Error while setting configuration");
  return ok(std::move(encoder));
}

// Parses `body` into `parser`, or returns the response to send instead. `body`
// must outlive `parser`.
std::optional<Response> read_body(const std::string &body, JSONParser &parser) {
  if (body.empty())
    return error(status_code::bad_request, "Empty body");
  if (!body_length_ok(body.size()))
    return error(status_code::too_large, "Invalid content");

  parser = JSONParser(body.c_str());
  if (parser.is_null())
    return error(status_code::bad_request, "Invalid JSON");

  return {};
}

} // namespace

bool body_length_ok(size_t length) {
  return length >= 1 && length <= max_body_length;
}

Response sys_status(bool maintenance, bool button, const char *synth_reason,
                    const char *hardware_reason) {
  helpers::JSONEncoder encoder;
  auto root = encoder.object();
  root.add_bool("maintenance", maintenance);
  root.add_bool("configured", synth_reason == nullptr && hardware_reason == nullptr);
  root.add_bool("button", button);

  auto reasons = root.add_object("reasons");
  if (synth_reason)
    reasons.add("synth", synth_reason);
  else
    reasons.add_null("synth");
  if (hardware_reason)
    reasons.add("hardware", hardware_reason);
  else
    reasons.add_null("hardware");

  return ok(std::move(encoder));
}

Response synth_get(const AppConfig &current) {
  return ok(configuration::codec::encode(current));
}

Response synth_put(const std::string &body, ApplyConfig apply) {
  JSONParser parser;
  if (auto early = read_body(body, parser))
    return *early;

  auto parsed = configuration::codec::parse_appconfig(parser);
  if (!parsed)
    return error(status_code::bad_request, parsed.error());

  const auto config = parsed.value();
  if (apply)
    apply(config);
  return saved_or_error(configuration::synth::persist(config),
                        configuration::codec::encode(config));
}

Response synth_reset(ApplyConfig apply) {
  const AppConfig config;
  if (apply)
    apply(config);
  return saved_or_error(configuration::synth::persist(config),
                        configuration::codec::encode(config));
}

Response hardware_get() {
  configuration::hardware::HardwareConfig config;
  configuration::hardware::read(config);
  return ok(configuration::codec::encode(config));
}

Response hardware_put(const std::string &body) {
  JSONParser parser;
  if (auto early = read_body(body, parser))
    return *early;

  auto parsed = configuration::codec::parse_hwconfig(parser);
  if (!parsed)
    return error(status_code::bad_request, parsed.error());

  const auto config = parsed.value();
  return saved_or_error(configuration::hardware::persist(config),
                        configuration::codec::encode(config));
}

Response hardware_reset() {
  const configuration::hardware::HardwareConfig config;
  return saved_or_error(configuration::hardware::persist(config),
                        configuration::codec::encode(config));
}

Response wifi_get() {
  configuration::wifi::WifiConfig config;
  configuration::wifi::read(config);
  return ok(configuration::codec::encode(config));
}

Response wifi_put(const std::string &body) {
  JSONParser parser;
  if (auto early = read_body(body, parser))
    return *early;

  configuration::wifi::WifiConfig current;
  configuration::wifi::read(current);

  auto parsed = configuration::codec::parse_wificonfig(parser, current);
  if (!parsed)
    return error(status_code::bad_request, parsed.error());

  const auto config = parsed.value();
  return saved_or_error(configuration::wifi::persist(config), configuration::codec::encode(config));
}

Response wifi_reset() {
  const configuration::wifi::WifiConfig config;
  return saved_or_error(configuration::wifi::persist(config), configuration::codec::encode(config));
}

} // namespace teslasynth::app::api
