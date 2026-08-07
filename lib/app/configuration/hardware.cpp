// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "codec.hpp"
#include "hardware.hpp"
#include "nvs_store.hpp"
#include "storage.hpp"

namespace teslasynth::app::configuration::hardware {

namespace {
constexpr char TAG[] = "hw_config";
constexpr char NAMESPACE[] = "hwconf";
} // namespace

ReadOutcome read(HardwareConfig &config) {
  config = HardwareConfig();

  auto raw = nvs_store::load(NAMESPACE, TAG);
  if (!raw)
    return {false, "Hardware is not provisioned"};

  helpers::JSONParser parser(raw->c_str());
  if (parser.is_null())
    return {false, "Stored hardware configuration is not valid JSON"};
  if (!codec::has_version(parser, HardwareConfig::current_version))
    return {false, "Stored hardware configuration was written by another firmware version"};

  auto parsed = codec::parse_hwconfig(parser);
  if (!parsed)
    return {false, parsed.error()};
  if (!parsed.value().is_valid())
    return {false, "Stored hardware configuration is out of range"};

  config = parsed.value();
  return {};
}

esp_err_t persist(const HardwareConfig &config) {
  auto json = codec::encode(config).print();
  return nvs_store::store(NAMESPACE, TAG, json.value);
}

} // namespace teslasynth::app::configuration::hardware
