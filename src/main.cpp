#include "app.hpp"
#include "sdkconfig.h"
#include <input/ble_midi.hpp>

extern "C" void app_main(void) {
  auto sbuf = ble_begin(CONFIG_TESLASYNTH_DEVICE_NAME);
  play(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
