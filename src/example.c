#include "example.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "musical_score_encoder.h"
#include <stddef.h>
#include <stdio.h>

#define RMT_BUZZER_RESOLUTION_HZ 1000000 // 1MHz resolution
#define RMT_BUZZER_GPIO_NUM 0

static const char *TAG = "example";

const char *model_info(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32:
    return "ESP32";
  case CHIP_ESP32S2:
    return "ESP32S2";
  case CHIP_ESP32S3:
    return "ESP32S3";
  case CHIP_ESP32C3:
    return "ESP32C3";
  case CHIP_ESP32H2:
    return "ESP32H2";
  case CHIP_ESP32C2:
    return "ESP32C2";
  default:
    return "Unknown";
  }
}

void print_chip_info() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("Chip model %s with %d CPU core(s), WiFi%s%s, ",
         model_info(chip_info.model), chip_info.cores,
         (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d\n", major_rev, minor_rev);
}

void print_version() {
  printf("ESP-IDF version: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR,
         ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
}

void print_task() {
  for (;;) {
    print_chip_info();
    print_version();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  };
}

/**
 * @brief Musical Score: Beethoven's Ode to joy
 */
static const buzzer_musical_score_t score[] = {
    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {740, 400}, {740, 200}, {659, 200}, {659, 800},

    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},

    {659, 400}, {659, 400}, {740, 400}, {587, 400}, {659, 400}, {740, 200},
    {784, 200}, {740, 400}, {587, 400}, {659, 400}, {740, 200}, {784, 200},
    {740, 400}, {659, 400}, {587, 400}, {659, 400}, {440, 400}, {440, 400},

    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},
};

void example(void) {
  ESP_LOGI(TAG, "Create RMT TX channel");
  rmt_channel_handle_t buzzer_chan = NULL;
  rmt_tx_channel_config_t tx_chan_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
      .gpio_num = RMT_BUZZER_GPIO_NUM,
      .mem_block_symbols = 64,
      .resolution_hz = RMT_BUZZER_RESOLUTION_HZ,
      .trans_queue_depth = 10, // set the maximum number of transactions that
                               // can pend in the background
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &buzzer_chan));

  ESP_LOGI(TAG, "Install musical score encoder");
  rmt_encoder_handle_t score_encoder = NULL;
  musical_score_encoder_config_t encoder_config = {
      .resolution = RMT_BUZZER_RESOLUTION_HZ};
  ESP_ERROR_CHECK(
      rmt_new_musical_score_encoder(&encoder_config, &score_encoder));

  ESP_LOGI(TAG, "Enable RMT TX channel");
  ESP_ERROR_CHECK(rmt_enable(buzzer_chan));
  ESP_LOGI(TAG, "Playing Beethoven's Ode to joy...");

  for (size_t i = 0; i < sizeof(score) / sizeof(score[0]); i++) {
    size_t count = score[i].duration_ms * score[i].freq_hz / 1000;
    for (; count > 0; count--) {
      rmt_transmit_config_t tx_config = {.loop_count = 0};
      ESP_ERROR_CHECK(rmt_transmit(buzzer_chan, score_encoder, &score[i],
                                   sizeof(buzzer_musical_score_t), &tx_config));
    }
  }
}
