#include "application.hpp"
#include "configuration/storage.hpp"
#include "devices/wifi.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "gui/setup.hpp"
#include "helpers/maintenance.hpp"
#include "teslasynth.hpp"
#include "web/server.hpp"

static const char *TAG = "TESLASYNTH";
using namespace teslasynth::app;

static configuration::hardware::HardwareConfig hconfig;
// = configuration::hardware::CYD;
// = configuration::hardware::lilygo_display;

static Application app;

extern "C" void app_main(void) {
  devices::storage::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  app.load(configuration::synth::read());
  cli::init(app.ui());

  const bool is_provisioned = configuration::hardware::read(hconfig);
  if ( // !is_provisioned ||
      helpers::maintenance::check()) {
    ESP_LOGI(TAG, "Entering maintenance mode.");
    devices::wifi::init();
    web::server::start(app.ui());
  } else {
    switch (hconfig.display.type) {
    case configuration::hardware::DisplayType::minimal:
      gui::init(hconfig.display.config.minimal);
      break;
    case configuration::hardware::DisplayType::full:
      gui::init(hconfig.display.config.full);
      break;
    case configuration::hardware::DisplayType::none:
      ESP_LOGI(TAG, "No display configured.");
    }

    devices::rmt::init();
    auto sbuf = synth::init(app.playback());
    devices::midi::init(sbuf);
  }

  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
