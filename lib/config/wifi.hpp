// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <cstring>

namespace teslasynth::app::configuration::wifi {

struct WifiConfig {
  constexpr static uint32_t current_version = 1;
  constexpr static size_t ssid_size = 33;     // 32 octets + NUL
  constexpr static size_t password_size = 64; // 63 chars + NUL
  constexpr static size_t min_password_len = 8;
  constexpr static uint8_t min_channel = 1;
  constexpr static uint8_t max_channel = 13;

  char ssid[ssid_size];
  char password[password_size];
  uint8_t channel;

  WifiConfig();

  bool is_open() const { return password[0] == '\0'; }

  bool is_valid() const {
    const size_t ssid_len = strnlen(ssid, ssid_size);
    if (ssid_len == 0 || ssid_len >= ssid_size)
      return false;

    const size_t password_len = strnlen(password, password_size);
    if (password_len >= password_size)
      return false;
    if (password_len != 0 && password_len < min_password_len)
      return false;

    return channel >= min_channel && channel <= max_channel;
  }
};

} // namespace teslasynth::app::configuration::wifi
