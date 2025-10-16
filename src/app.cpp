#include "app.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "midi_core.hpp"
#include "synth.hpp"
#include <cstddef>
#include <cstdint>

QueueHandle_t xQueue;
const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

void task(void *) {
  MidiData data;
  Config config;
  Notes notes(config);
  SynthChannel channel(config, notes);

  TickType_t now = xTaskGetTickCount();
  uint64_t noww = esp_timer_get_time();
  int msg;
  for (;;) {
    auto status = xQueueReceive(xQueue, &msg, xTicksToWait);
    if (status == pdPASS) {
      channel.on_note_on(70, 127, 0_ms);
    } else {
      notes.tick();
    }
  }
}

void play() {
  xQueue = xQueueCreate(1, sizeof(int));
  if (xQueue == NULL) {
    return;
  }
  xTaskCreate(task, "Synth", 1000, NULL, 1, NULL);
}
