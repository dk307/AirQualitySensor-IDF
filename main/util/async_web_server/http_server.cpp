#include "http_server.h"
#include "http_request.h"

#include "logging/logging_tags.h"
#include <esp_log.h>

namespace esp32
{
void http_server::begin()
{
    if (server_)
    {
        end();
    }

    ESP_LOGI(WEBSERVER_TAG, "Starting web server on port:%d", port_);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port_;
    config.max_uri_handlers = 30;
    config.ctrl_port = 32760;

    ESP_ERROR_CHECK(httpd_start(&server_, &config));
    ESP_LOGI(WEBSERVER_TAG, "Started web server on port:%d", port_);
}

void http_server::end()
{
    if (server_)
    {
        httpd_stop(server_);
        server_ = nullptr;
    }
}

void http_server::add_handler(const char *url, httpd_method_t method, url_handler request_handler, const void *user_ctx)
{
    httpd_uri_t handler{};
    handler.uri = url;
    handler.method = method;
    handler.handler = request_handler;
    handler.user_ctx = const_cast<void *>(user_ctx);

    handlers_.emplace_back(handler);
    httpd_register_uri_handler(server_, &(*handlers_.rbegin()));
}
} // namespace esp32
