// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "codec.hpp"
#include "nvs_store.hpp"
#include "storage.hpp"
#include "wifi.hpp"

namespace teslasynth::app::configuration::wifi {

namespace {
constexpr char TAG[] = "wifi_config";
constexpr char NAMESPACE[] = "wificonf";
} // namespace

ReadOutcome read(WifiConfig &config) {
  config = WifiConfig();

  auto raw = nvs_store::load(NAMESPACE, TAG);
  if (!raw)
    return {false, "No WiFi configuration stored"};

  helpers::JSONParser parser(raw->c_str());
  if (parser.is_null())
    return {false, "Stored WiFi configuration is not valid JSON"};
  if (!codec::has_version(parser, WifiConfig::current_version))
    return {false, "Stored WiFi configuration was written by another firmware version"};

  auto parsed = codec::parse_wificonfig(parser, WifiConfig());
  if (!parsed)
    return {false, parsed.error()};
  if (!parsed.value().is_valid())
    return {false, "Stored WiFi configuration is out of range"};

  config = parsed.value();
  return {};
}

esp_err_t persist(const WifiConfig &config) {
  auto json = codec::encode_stored(config).print();
  return nvs_store::store(NAMESPACE, TAG, json.value);
}

} // namespace teslasynth::app::configuration::wifi
