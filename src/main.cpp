#include "application.hpp"
#include "configuration/storage.hpp"
#include "devices/wifi.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "gui/setup.hpp"
#include "teslasynth.hpp"
#include "web/server.hpp"

static const char *TAG = "TESLASYNTH";
using namespace teslasynth::app;

static configuration::hardware::HardwareConfig hconfig;
static Application app;

extern "C" void app_main(void) {
  devices::storage::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  app.load(configuration::synth::read());
  cli::init(app.ui());

  if (!configuration::hardware::read(hconfig)) {
    ESP_LOGI(TAG, "Hardware config not found.");
  }
  devices::wifi::init();
  web::server::start(app.ui());

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
  auto sbuf = devices::ble_midi::init();
  synth::init(sbuf, app.playback());

  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
