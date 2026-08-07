// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#include "server.hpp"
#include "../application.hpp"
#include "json.hpp"
#include "../helpers/sysinfo.h"
#include "../status.hpp"
#include "api.hpp"
#include "codec.hpp"
#include "hardware.hpp"
#include "configuration/storage.hpp"
#include "wifi.hpp"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"
#include "http_parser.h"
#include <optional>
#include <string>

extern const uint8_t index_html_gz[];
extern const size_t index_html_gz_len;

namespace teslasynth::app::web::server {

using namespace core;
using namespace configuration::codec;

namespace {

constexpr char TAG[] = "WEBSERVER";
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
  root.add_bool("otg", info.otg);
  root.add_bool("emb-flash", info.emb_flash);

  auto firmware = root.add_object("firmware");
  auto app_version = esp_app_get_description();
  firmware.add("version", app_version->version);
  firmware.add("compile-time", app_version->date);
  firmware.add("idf-version", esp_get_idf_version());
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

esp_err_t sys_reboot_handler(httpd_req_t *) {
  esp_restart();
}

esp_err_t send(httpd_req_t *req, const api::Response &res) {
  if (res.is_ok()) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, res.body.c_str());
    return ESP_OK;
  }

  httpd_err_code_t code = HTTPD_500_INTERNAL_SERVER_ERROR;
  if (res.status == api::status_code::bad_request)
    code = HTTPD_400_BAD_REQUEST;
  else if (res.status == api::status_code::too_large)
    code = HTTPD_413_CONTENT_TOO_LARGE;

  httpd_resp_send_err(req, code, res.body.c_str());
  return ESP_FAIL;
}

esp_err_t read_body(httpd_req_t *req, std::string &body) {
  const size_t length = req->content_len;
  if (!api::body_length_ok(length)) {
    send(req, {api::status_code::too_large, "Invalid content"});
    return ESP_FAIL;
  }

  body.resize(length);
  if (httpd_req_recv(req, body.data(), length) != static_cast<int>(length)) {
    send(req, {api::status_code::server_error, "Incomplete body"});
    return ESP_FAIL;
  }
  return ESP_OK;
}

void apply_synth(const AppConfig &config) {
  ui.config_set(config, true);
}

esp_err_t sys_status_handler(httpd_req_t *req) {
  const auto &boot = status::get();
  return send(req, api::sys_status(boot.maintenance, boot.button, boot.synth, boot.hardware));
}

esp_err_t synth_config_get_handler(httpd_req_t *req) {
  return send(req, api::synth_get(ui.config_read()));
}

esp_err_t synth_config_put_handler(httpd_req_t *req) {
  std::string body;
  ESP_RETURN_ON_ERROR(read_body(req, body), TAG, "Invalid body.");
  return send(req, api::synth_put(body, &apply_synth));
}

esp_err_t synth_config_del_handler(httpd_req_t *req) {
  return send(req, api::synth_reset(&apply_synth));
}

helpers::JSONEncoder instruments_list_json() {
  helpers::JSONEncoder encoder;
  auto root = encoder.array();
  for (const auto &name : synth::instrument_names) {
    root.add(name);
  }
  return encoder;
}

esp_err_t synth_instruments_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, instruments_list_json().print().value);
  return ESP_OK;
}

esp_err_t hardware_config_get_handler(httpd_req_t *req) {
  return send(req, api::hardware_get());
}

esp_err_t hardware_config_put_handler(httpd_req_t *req) {
  std::string body;
  ESP_RETURN_ON_ERROR(read_body(req, body), TAG, "Invalid body.");
  return send(req, api::hardware_put(body));
}

esp_err_t hardware_config_del_handler(httpd_req_t *req) {
  return send(req, api::hardware_reset());
}

esp_err_t wifi_config_get_handler(httpd_req_t *req) {
  return send(req, api::wifi_get());
}

esp_err_t wifi_config_put_handler(httpd_req_t *req) {
  std::string body;
  ESP_RETURN_ON_ERROR(read_body(req, body), TAG, "Invalid body.");

  configuration::Guard guard;
  return send(req, api::wifi_put(body));
}

esp_err_t wifi_config_del_handler(httpd_req_t *req) {
  configuration::Guard guard;
  return send(req, api::wifi_reset());
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

typedef esp_err_t (*EndpointHandler)(httpd_req_t *);

struct Resource {
  const char *uri;
  EndpointHandler get = nullptr;
  EndpointHandler post = nullptr;
  EndpointHandler put = nullptr;
  EndpointHandler del = nullptr;

  inline void register_on(httpd_handle_t server) const {
    httpd_uri_t uri_handler = {};
    uri_handler.uri = uri;
    uri_handler.user_ctx = nullptr;

    if (get) {
      uri_handler.method = HTTP_GET;
      uri_handler.handler = get;
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_handler));
    }
    if (post) {
      uri_handler.method = HTTP_POST;
      uri_handler.handler = post;
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_handler));
    }
    if (put) {
      uri_handler.method = HTTP_PUT;
      uri_handler.handler = put;
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_handler));
    }
    if (del) {
      uri_handler.method = HTTP_DELETE;
      uri_handler.handler = del;
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_handler));
    }
  }
};

constexpr Resource resources[] = {
    {
        .uri = "/",
        .get = index_handler,
    },
    {
        .uri = "/api/sys/info",
        .get = sysinfo_handler,
    },
    {
        .uri = "/api/sys/reboot",
        .post = sys_reboot_handler,
    },
    {
        .uri = "/api/sys/status",
        .get = sys_status_handler,
    },
    {
        .uri = "/api/config/synth",
        .get = synth_config_get_handler,
        .put = synth_config_put_handler,
        .del = synth_config_del_handler,
    },
    {
        .uri = "/api/synth/instruments",
        .get = synth_instruments_get_handler,
    },
    {
        .uri = "/api/config/hardware",
        .get = hardware_config_get_handler,
        .put = hardware_config_put_handler,
        .del = hardware_config_del_handler,
    },
    {
        .uri = "/api/config/wifi",
        .get = wifi_config_get_handler,
        .put = wifi_config_put_handler,
        .del = wifi_config_del_handler,
    },
};

void start(UIHandle handle) {
  ui = handle;
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 4 * (sizeof(resources) / sizeof(Resource));
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  for (const auto &res : resources) {
    res.register_on(server);
  }
}
} // namespace teslasynth::app::web::server
