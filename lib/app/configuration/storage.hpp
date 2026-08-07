// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "persistence.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::ChannelConfig;

class Guard {
public:
  Guard();
  ~Guard();

  Guard(const Guard &) = delete;
  Guard &operator=(const Guard &) = delete;
};

} // namespace teslasynth::app::configuration
