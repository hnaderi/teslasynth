#include "application.hpp"
#include "configuration/storage.hpp"
#include "devices/signal_led.hpp"
#include "devices/wifi.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "helpers/maintenance.hpp"
#include "teslasynth.hpp"
#include "web/server.hpp"

using namespace teslasynth::app;

static constexpr char TAG[] = "TESLASYNTH";
static configuration::hardware::HardwareConfig hconfig;
static Application app;

extern "C" void app_main(void) {
  devices::storage::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  if (app.reload_config())
    ESP_LOGW(TAG, "Synth config fallbacks to factory settings.");

  const bool is_provisioned = configuration::hardware::read(hconfig);
  if (!is_provisioned || helpers::maintenance::check()) {
    ESP_LOGI(TAG, "Entering maintenance mode.");
    devices::wifi::init();
    web::server::start(app.ui());
  } else {
    helpers::maintenance::init(hconfig.input);
    devices::signal_led::init(hconfig.led);
    devices::rmt::init(hconfig.output);
    auto sbuf = synth::init(app.playback());
    devices::midi::init(sbuf);
  }

  cli::init(app.ui());

  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
