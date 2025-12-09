#include "hardware.hpp"
#include "cbor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include <cstdint>
#include <vector>

namespace teslasynth::app::configuration::hardware {

namespace {
const char *TAG = "hw_config";
const char *KEY = "config";

esp_err_t init(nvs_handle_t &handle) {
  esp_err_t err = nvs_open("hwconf", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  return err;
}

bool decode(HardwareConfig &config, const uint8_t *buf, size_t len) {
  CborParser parser;
  CborValue it;
  CborError err = cbor_parser_init(buf, len, 0, &parser, &it);
  if (err)
    return false;

  return true;
}
bool encode(const HardwareConfig &config, uint8_t *buf, size_t bufsize,
            size_t &out_len) {
  CborEncoder encoder;
  cbor_encoder_init(&encoder, buf, bufsize, 0);

  out_len = cbor_encoder_get_buffer_size(&encoder, buf);
  return true;
}
} // namespace

bool read(HardwareConfig &config) {
  bool success = false;
  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));
  size_t len = 0;
  esp_err_t err = nvs_get_blob(handle, KEY, NULL, &len);
  if (err == ESP_OK) {
    std::vector<uint8_t> buf(len);
    esp_err_t err = nvs_get_blob(handle, KEY, buf.data(), &len);
    if (err == ESP_OK) {
      success = decode(config, buf.data(), len);
    }
  }
  nvs_close(handle);
  return success;
}
void persist(const HardwareConfig &config) {
  size_t len;
  // Measure required buffer size
  encode(config, nullptr, 0, len);

  std::vector<uint8_t> buf(len);
  encode(config, buf.data(), len, len);

  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));

  auto err = nvs_set_blob(handle, KEY, &config, len);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Couldn't persist configuration!");
  } else {
    nvs_commit(handle);
  }
  nvs_close(handle);
}
} // namespace teslasynth::app::configuration::hardware
