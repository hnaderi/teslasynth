#include "app.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "synth.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

QueueHandle_t xQueue;
const TickType_t xTicksToWait = pdMS_TO_TICKS(10);

static const char *TAG = "APP";

void cbk(const MidiChannelMessage &msg) {
  ESP_LOGI(TAG, "Received: %s", std::string(msg).c_str());
}

void task(void *pvParams) {
  StreamBufferHandle_t sbuf = static_cast<StreamBufferHandle_t>(pvParams);
  MidiParser parser(cbk);
  uint8_t buffer[64];
  size_t read = 0;

  while (true) {
    read = xStreamBufferReceive(sbuf, buffer, sizeof(buffer), xTicksToWait);
    parser.feed(buffer, read);
  }
}

void play(StreamBufferHandle_t sbuf) {
  xTaskCreatePinnedToCore(task, "Synth", 8 * 1024, sbuf, 1, NULL, 1);
}
