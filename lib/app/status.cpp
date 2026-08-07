// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "status.hpp"

namespace teslasynth::app::status {

namespace {
BootStatus current;
}

void set(const BootStatus &status) {
  current = status;
}
const BootStatus &get() {
  return current;
}

} // namespace teslasynth::app::status
