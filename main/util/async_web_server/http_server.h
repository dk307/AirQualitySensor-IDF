#pragma once

#include <esp_http_server.h>
#include <vector>


#include "logging/logging_tags.h"
#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_response.h"
#include <esp_log.h>


namespace esp32
{
class http_server
{
  public:
    typedef esp_err_t (*url_handler)(httpd_req_t *r);

    http_server(uint16_t port) : port_(port){};
    ~http_server()
    {
        end();
    }

    virtual void begin();
    virtual void end();

  protected:
    void add_handler(const char *url, httpd_method_t method, url_handler request_handler, const void *user_ctx);

    // general url handler
    template <class T, void (T::*ftn)(esp32::http_request *)> static esp_err_t server_url_ftn(httpd_req_t *request_p)
    {
        try
        {
            auto p_this = reinterpret_cast<T *>(request_p->user_ctx);
            esp32::http_request request(request_p);
            (p_this->*ftn)(&request);
            return ESP_OK;
        }
        catch (const esp32::esp_exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process request with:%s", ex.what());
            return ex.get_error();
        }
        catch (const std::exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process file request with:%s", ex.what());
            return ESP_FAIL;
        }
    }

    template <class T, void (T::*ftn)(esp32::http_request *)> inline void add_handler_ftn(const auto url, httpd_method_t method)
    {
        add_handler(url, method, server_url_ftn<T, ftn>, this);
    }

    // fs handler
    template <const auto file_pathT, const auto content_typeT> static esp_err_t serve_fs_file(httpd_req_t *request_p)
    {
        try
        {
            esp32::http_request request(request_p);
            esp32::fs_card_file_response response(&request, file_pathT, content_typeT, false);
            response.send_response();
            return ESP_OK;
        }
        catch (const esp32::esp_exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process file request with:%s", ex.what());
            return ex.get_error();
        }
        catch (const std::exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process file request with:%s", ex.what());
            return ESP_FAIL;
        }
    }

    template <const auto file_pathT, const auto content_typeT> inline void add_fs_file_handler(const char *url, httpd_method_t method = HTTP_GET)
    {
        add_handler(url, method, serve_fs_file<file_pathT, content_typeT>, nullptr);
    }

    // array handler
    template <const uint8_t buf[], const auto len, bool is_gz, const auto content_type> static esp_err_t serve_array(httpd_req_t *request_p)
    {
        try
        {
            esp32::http_request request(request_p);
            esp32::array_response response(&request, reinterpret_cast<const uint8_t *>(buf), len, is_gz, content_type);
            response.send_response();
            return ESP_OK;
        }
        catch (const esp32::esp_exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process file request with:%s", ex.what());
            return ex.get_error();
        }
        catch (const std::exception &ex)
        {
            ESP_LOGI(WEBSERVER_TAG, "Failed to process file request with:%s", ex.what());
            return ESP_FAIL;
        }
    }

    template <const uint8_t buf[], const auto len, bool is_gz, const auto content_type>
    inline void add_array_handler(const char *url, httpd_method_t method = HTTP_GET)
    {
        add_handler(url, method, serve_array<buf, len, is_gz, content_type>, nullptr);
    }

  private:
    const uint16_t port_{};
    httpd_handle_t server_{};
    std::vector<httpd_uri_t> handlers_;
};
} // namespace esp32
