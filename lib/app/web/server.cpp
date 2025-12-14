#include "server.hpp"
#include "../application.hpp"
#include "../helpers/json.hpp"
#include "../helpers/sysinfo.h"
#include "cJSON.h"
#include "configuration/codec.hpp"
#include "configuration/storage.hpp"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <optional>
#include <string_view>
#include <vector>

extern const uint8_t index_html_gz[];
extern const size_t index_html_gz_len;

namespace teslasynth::app::web::server {

using namespace core;
using namespace configuration::codec;
using helpers::JSONParser;

namespace {

constexpr char const *TAG = "WEBSERVER";
constexpr size_t max_body_length = 1024;
static UIHandle ui;

#define cstr(value) std::string(value).c_str()

helpers::JSONEncoder encode(const ChipInfo &info) {
  helpers::JSONEncoder encoder;
  auto root = encoder.object();
  root.add("model", info.model);
  root.add("cores", info.cores);
  root.add("flash-size", info.flash_size);
  root.add("revision", info.revision);

  root.add_bool("wifi", info.wifi);
  root.add_bool("ble", info.ble);
  root.add_bool("bt", info.bt);
  root.add_bool("emb-flash", info.emb_flash);
  return encoder;
}

esp_err_t sysinfo_handler(httpd_req_t *req) {
  ChipInfo result;
  ESP_RETURN_ON_ERROR(get_chip_info(result), TAG, "Couldn't get chip info!");

  httpd_resp_set_type(req, "application/json");
  auto json = encode(result).print();
  httpd_resp_sendstr(req, json.value);
  return ESP_OK;
}

esp_err_t synth_config_get_handler(httpd_req_t *req) {
  AppConfig config = ui.config_read();

  httpd_resp_set_type(req, "application/json");
  auto json = configuration::codec::encode(config).print();
  httpd_resp_sendstr(req, json.value);
  return ESP_OK;
}

esp_err_t parseBody(httpd_req_t *req, std::vector<char> &body,
                    JSONParser &parser) {
  size_t content_len = req->content_len;
  if (content_len > max_body_length || content_len < 1) {
    httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Invalid content");
    return ESP_FAIL;
  }

  body.resize(content_len + 1);
  size_t received = httpd_req_recv(req, body.data(), content_len);
  if (received != content_len) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Incomplete body");
    return ESP_FAIL;
  }
  body[content_len] = '\0';

  parser = JSONParser(body.data());
  if (parser.is_null()) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t synth_config_put_handler(httpd_req_t *req) {
  std::vector<char> body;
  JSONParser parser;
  ESP_RETURN_ON_ERROR(parseBody(req, body, parser), TAG, "Invalid json body.");

  httpd_resp_set_type(req, "application/json");
  AppConfig config = ui.config_read();
  if (parse(parser, config)) {
    ui.config_set(config, true);
    auto res = configuration::synth::persist(config);
    if (res == ESP_OK) {
      auto json = configuration::codec::encode(config).print();
      httpd_resp_sendstr(req, json.value);
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Error while setting configuration");
    }
    return res;
  }
  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request.");
  return ESP_FAIL;
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

  httpd_uri_t config_synth_put_uri = {
      .uri = "/api/config/synth",
      .method = HTTP_PUT,
      .handler = synth_config_put_handler,
  };
  httpd_register_uri_handler(server, &config_synth_put_uri);
}
} // namespace teslasynth::app::web::server
