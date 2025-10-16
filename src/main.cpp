#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "output/rmt_driver.h"
#include <input/ble_midi.hpp>

extern "C" void app_main(void) {
  ble_begin("Teslasynth");
  rmt_driver();
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
