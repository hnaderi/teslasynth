// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "hardware.hpp"
#include "synth.hpp"
#include "wifi.hpp"
#include <optional>
#include <string>

namespace teslasynth::app::configuration {

// Backing store for configuration documents, supplied by the platform at
// startup: NVS on target, an in-memory map in the native tests. Until one is
// installed every read reports "no configuration stored".
namespace store {
using Loader = std::optional<std::string> (*)(const char *scope);
using Saver = bool (*)(const char *scope, const char *json);

void install(Loader loader, Saver saver);
} // namespace store

// On failure `config` is left holding factory defaults and `reason` explains
// what was wrong, for the maintenance-mode report.
struct ReadOutcome {
  bool ok = true;
  const char *reason = nullptr;

  explicit operator bool() const { return ok; }
};

namespace synth {
ReadOutcome read(AppConfig &config);
bool persist(const AppConfig &config);
} // namespace synth

namespace hardware {
ReadOutcome read(HardwareConfig &config);
bool persist(const HardwareConfig &config);
} // namespace hardware

namespace wifi {
ReadOutcome read(WifiConfig &config);
bool persist(const WifiConfig &config);
} // namespace wifi

} // namespace teslasynth::app::configuration
