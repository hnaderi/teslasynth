// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace teslasynth::app::configuration::wifi {

struct WifiConfig {
  constexpr static uint32_t current_version = 0;
  constexpr static size_t ssid_size = 33;     // 32 octets + NUL
  constexpr static size_t password_size = 64; // 63 chars + NUL
  constexpr static size_t min_password_len = 8;
  constexpr static uint8_t min_channel = 1;
  constexpr static uint8_t max_channel = 13;

  uint32_t version = current_version;
  char ssid[ssid_size];
  char password[password_size];
  uint8_t channel;
  uint8_t reserved[2] = {};

  WifiConfig();

  bool is_open() const { return password[0] == '\0'; }
  bool is_valid() const;
};

static_assert(std::has_unique_object_representations_v<WifiConfig>,
              "WifiConfig is persisted as a blob and must have no padding");

} // namespace teslasynth::app::configuration::wifi
