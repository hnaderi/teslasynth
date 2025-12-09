#include "application.hpp"
#include "configuration/storage.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "teslasynth.hpp"

static const char *TAG = "TESLASYNTH";
using namespace teslasynth::app;

static configuration::hardware::HardwareConfig hconfig;

extern "C" void app_main(void) {
  if (configuration::hardware::read(hconfig)) {
    ESP_LOGI(TAG, "Hardware config not found.");
  }
  devices::storage::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  Application app(configuration::synth::read());

#ifndef CONFIG_TESLASYNTH_GUI_NONE
  gui::init();
#endif
  cli::init(app.ui());
  devices::rmt::init();
  auto sbuf = devices::ble_midi::init();
  synth::init(sbuf, app.playback());
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
