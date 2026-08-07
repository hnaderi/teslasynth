// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "application.hpp"
#include "configuration/nvs_store.hpp"
#include "configuration/storage.hpp"
#include "devices/signal_led.hpp"
#include "devices/access_point.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "helpers/maintenance.hpp"
#include "status.hpp"
#include "teslasynth.hpp"
#include "web/server.hpp"

using namespace teslasynth::app;

static constexpr char TAG[] = "TESLASYNTH";
static configuration::hardware::HardwareConfig hconfig;
static Application app;

extern "C" void app_main(void) {
  devices::storage::init();
  configuration::nvs_store::install();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  const auto synth_outcome = app.reload_config();
  const auto hardware_outcome = configuration::hardware::read(hconfig);

  status::BootStatus boot;
  boot.button = helpers::maintenance::check();
  boot.synth = synth_outcome.reason;
  boot.hardware = hardware_outcome.reason;
  boot.maintenance = !boot.configured() || boot.button;
  status::set(boot);

  if (boot.maintenance) {
    if (boot.hardware)
      ESP_LOGW(TAG, "Hardware configuration unusable: %s", boot.hardware);
    if (boot.synth)
      ESP_LOGW(TAG, "Synth configuration unusable: %s", boot.synth);

    ESP_LOGI(TAG, "Entering maintenance mode.");
    configuration::wifi::WifiConfig wconfig;
    configuration::wifi::read(wconfig);
    devices::access_point::init(wconfig);
    web::server::start(app.ui());
  } else {
    helpers::maintenance::init(hconfig.input);
    devices::signal_led::init(hconfig.led);
    devices::rmt::init(hconfig.output);
    auto sbuf = synth::init(app.playback());
    devices::midi::init(sbuf);
  }

  cli::init(app.ui(), boot.maintenance);

  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
