// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "persistence.hpp"
#include <string>
#include <vector>

namespace teslasynth::app::console {

struct ConfigRequest {
  bool save = false;
  bool reload = false;
  bool reset = false;
  bool maintenance = false;
  std::vector<std::string> values;
};

struct ConfigOutcome {
  int code = 0;
  bool apply = false;
  bool save = false;
  bool reload = false;
  AppConfig config;
  std::string message;

  bool failed() const { return code != 0; }
};

ConfigOutcome run_config_command(const AppConfig &current, const ConfigRequest &request);

std::string synth_report(const AppConfig &config, const char *unconfigured_reason);
std::string hardware_report(const configuration::hardware::HardwareConfig &config,
                            const char *unconfigured_reason);
std::string wifi_report(const configuration::wifi::WifiConfig &config);

} // namespace teslasynth::app::console
