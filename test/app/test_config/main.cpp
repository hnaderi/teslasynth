#include "configuration/storage.hpp"
#include "esp_log.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <unity.h>
static const char *TAG = "TEST";

using namespace teslasynth::app::configuration::hardware;

void test_empty(void) {
  ESP_LOGI(TAG, "Hardware config size: %u", sizeof(HardwareConfig));
}

void test_encode_calculates_required_length(void) {
  HardwareConfig a;
  size_t len = 0;
  encode(a, nullptr, 0, len);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
