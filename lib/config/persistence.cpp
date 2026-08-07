// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "persistence.hpp"
#include "codec.hpp"

namespace teslasynth::app::configuration {

namespace store {
namespace {
Loader loader_ = nullptr;
Saver saver_ = nullptr;
} // namespace

void install(Loader loader, Saver saver) {
  loader_ = loader;
  saver_ = saver;
}

std::optional<std::string> load(const char *scope) {
  return loader_ ? loader_(scope) : std::nullopt;
}
bool save(const char *scope, const char *json) {
  return saver_ && saver_(scope, json);
}
} // namespace store

namespace {

constexpr char SYNTH[] = "synth";
constexpr char HARDWARE[] = "hardware";
constexpr char WIFI[] = "wifi";

template <typename T, typename Decode>
ReadOutcome load_document(const char *scope, uint32_t version, const char *missing, T &config,
                          Decode decode) {
  config = T();

  auto raw = store::load(scope);
  if (!raw)
    return {false, missing};

  helpers::JSONParser parser(raw->c_str());
  if (parser.is_null())
    return {false, "Stored configuration is not valid JSON"};
  if (!codec::has_version(parser, version))
    return {false, "Stored configuration was written by another firmware version"};

  auto parsed = decode(parser);
  if (!parsed)
    return {false, parsed.error()};
  if (!parsed.value().is_valid())
    return {false, "Stored configuration is out of range"};

  config = parsed.value();
  return {};
}

} // namespace

namespace synth {

ReadOutcome read(AppConfig &config) {
  return load_document(SYNTH, AppConfig::current_version, "No synth configuration stored", config,
                       [](helpers::JSONParser &p) { return codec::parse_appconfig(p); });
}

bool persist(const AppConfig &config) {
  return store::save(SYNTH, codec::encode(config).print().value);
}

} // namespace synth

namespace hardware {

ReadOutcome read(HardwareConfig &config) {
  return load_document(HARDWARE, HardwareConfig::current_version, "Hardware is not provisioned",
                       config, [](helpers::JSONParser &p) { return codec::parse_hwconfig(p); });
}

bool persist(const HardwareConfig &config) {
  return store::save(HARDWARE, codec::encode(config).print().value);
}

} // namespace hardware

namespace wifi {

ReadOutcome read(WifiConfig &config) {
  return load_document(
      WIFI, WifiConfig::current_version, "No WiFi configuration stored", config,
      [](helpers::JSONParser &p) { return codec::parse_wificonfig(p, WifiConfig()); });
}

bool persist(const WifiConfig &config) {
  return store::save(WIFI, codec::encode_stored(config).print().value);
}

} // namespace wifi

} // namespace teslasynth::app::configuration
