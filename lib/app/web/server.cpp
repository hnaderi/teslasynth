#include "server.hpp"
#include "../application.hpp"
#include "../helpers/sysinfo.h"
#include "cJSON.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

extern const uint8_t index_html_gz[];
extern const size_t index_html_gz_len;

namespace teslasynth::app::web::server {
namespace {
constexpr char const *TAG = "WEBSERVER";
static UIHandle ui;
#define cstr(value) std::string(value).c_str()

esp_err_t sysinfo_handler(httpd_req_t *req) {
  ChipInfo result;
  ESP_RETURN_ON_ERROR(get_chip_info(result), TAG, "Couldn't get chip info!");

  httpd_resp_set_type(req, "text/plain");

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "model", result.model);
  cJSON_AddNumberToObject(root, "cores", result.cores);
  cJSON_AddNumberToObject(root, "flash-size", result.flash_size);
  cJSON_AddNumberToObject(root, "revision", result.revision);

  cJSON_AddBoolToObject(root, "wifi", result.wifi);
  cJSON_AddBoolToObject(root, "ble", result.ble);
  cJSON_AddBoolToObject(root, "bt", result.bt);
  cJSON_AddBoolToObject(root, "emb-flash", result.emb_flash);

  const char *json = cJSON_Print(root);
  httpd_resp_sendstr(req, json);
  free((void *)json);
  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t synth_config_get_handler(httpd_req_t *req) {
  AppConfig config = ui.config_read();

  httpd_resp_set_type(req, "text/plain");

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "tuning", cstr(config.synth().a440));
  if (config.synth().instrument.has_value())
    cJSON_AddNumberToObject(root, "instrument", *config.synth().instrument + 1);

  cJSON *channels = cJSON_CreateArray();
  for (auto &ch : config.channel_configs) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "notes", ch.notes);
    cJSON_AddStringToObject(obj, "max-on-time", cstr(ch.max_on_time));
    cJSON_AddStringToObject(obj, "min-dead-time", cstr(ch.min_deadtime));
    cJSON_AddStringToObject(obj, "max-duty", cstr(ch.max_duty));
    cJSON_AddStringToObject(obj, "duty-window", cstr(ch.duty_window));
    if (ch.instrument.has_value())
      cJSON_AddNumberToObject(obj, "instrument", *ch.instrument + 1);
    cJSON_AddItemToArray(channels, obj);
  }
  cJSON_AddItemToObject(root, "channels", channels);

  const char *json = cJSON_Print(root);
  httpd_resp_sendstr(req, json);
  free((void *)json);
  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");

  const size_t chunk_size = 1024;
  size_t remaining = index_html_gz_len;
  const uint8_t *ptr = index_html_gz;

  while (remaining > 0) {
    size_t to_send = remaining > chunk_size ? chunk_size : remaining;
    esp_err_t err = httpd_resp_send_chunk(req, (const char *)ptr, to_send);
    if (err != ESP_OK) {
      return err;
    }
    ptr += to_send;
    remaining -= to_send;
  }

  return httpd_resp_send_chunk(req, NULL, 0);
}
} // namespace

void start(UIHandle handle) {
  ui = handle;
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  httpd_uri_t index_get_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
  };
  httpd_register_uri_handler(server, &index_get_uri);

  httpd_uri_t sysinfo_get_uri = {
      .uri = "/api/sysinfo",
      .method = HTTP_GET,
      .handler = sysinfo_handler,
  };
  httpd_register_uri_handler(server, &sysinfo_get_uri);

  httpd_uri_t config_synth_get_uri = {
      .uri = "/api/config/synth",
      .method = HTTP_GET,
      .handler = synth_config_get_handler,
  };
  httpd_register_uri_handler(server, &config_synth_get_uri);
}
} // namespace teslasynth::app::web::server
