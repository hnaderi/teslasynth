#include "teslasynth.hpp"
#include "configuration/synth.hpp"
#include "esp_event.h"

extern "C" void app_main(void) {
  init_storage();
  load_config();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifndef CONFIG_TESLASYNTH_GUI_NONE
  init_gui();
#endif
  init_cli();
  auto sbuf = init_ble_midi();
  init_synth(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
