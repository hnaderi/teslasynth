// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "codec.hpp"
#include "nvs_store.hpp"
#include "storage.hpp"
#include "synth.hpp"

namespace teslasynth::app::configuration::synth {

namespace {
constexpr char TAG[] = "synth_config";
constexpr char NAMESPACE[] = "synth";
} // namespace

ReadOutcome read(AppConfig &config) {
  config = AppConfig();

  auto raw = nvs_store::load(NAMESPACE, TAG);
  if (!raw)
    return {false, "No synth configuration stored"};

  helpers::JSONParser parser(raw->c_str());
  if (parser.is_null())
    return {false, "Stored synth configuration is not valid JSON"};
  if (!codec::has_version(parser, AppConfig::current_version))
    return {false, "Stored synth configuration was written by another firmware version"};

  auto parsed = codec::parse_appconfig(parser);
  if (!parsed)
    return {false, parsed.error()};
  if (!parsed.value().is_valid())
    return {false, "Stored synth configuration is out of range"};

  config = parsed.value();
  return {};
}

esp_err_t persist(const AppConfig &config) {
  auto json = codec::encode(config).print();
  return nvs_store::store(NAMESPACE, TAG, json.value);
}

} // namespace teslasynth::app::configuration::synth
