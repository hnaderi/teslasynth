// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

namespace teslasynth::app::status {

struct BootStatus {
  bool maintenance = false;
  bool button = false;
  const char *synth = nullptr;
  const char *hardware = nullptr;

  bool configured() const { return synth == nullptr && hardware == nullptr; }
};

void set(const BootStatus &status);
const BootStatus &get();

} // namespace teslasynth::app::status
