// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "persistence.hpp"
#include <string>

namespace teslasynth::app::api {

namespace status_code {
constexpr int ok = 200;
constexpr int bad_request = 400;
constexpr int too_large = 413;
constexpr int server_error = 500;
} // namespace status_code

struct Response {
  int status = status_code::ok;
  std::string body;

  bool is_ok() const { return status == status_code::ok; }
};

constexpr size_t max_body_length = 4096;
bool body_length_ok(size_t length);

using ApplyConfig = void (*)(const AppConfig &);

Response sys_status(bool maintenance, bool button, const char *synth_reason,
                    const char *hardware_reason);

Response synth_get(const AppConfig &current);
Response synth_put(const std::string &body, ApplyConfig apply);
Response synth_reset(ApplyConfig apply);

Response hardware_get();
Response hardware_put(const std::string &body);
Response hardware_reset();

Response wifi_get();
Response wifi_put(const std::string &body);
Response wifi_reset();

} // namespace teslasynth::app::api
