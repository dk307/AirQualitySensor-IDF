#pragma once

#include "logging/logging_tags.h"
#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_response.h"
#include "util/exceptions.h"
#include "util/noncopyable.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <type_traits>
#include <utility>

namespace esp32
{
class http_server : esp32::noncopyable
{
  public:
    typedef esp_err_t (*url_handler)(httpd_req_t *r);
    typedef void (*url_handler_with_exception)(httpd_req_t *r);

    http_server(uint16_t port) : port_(port){};
    ~http_server();

    virtual void begin();
    virtual void end();

  protected:
    void add_handler(const char *url, httpd_method_t method, url_handler request_handler, const void *user_ctx);

    template <void (*ftn)(httpd_req_t *r)> void add_handler_with_exceptions(const char *url, httpd_method_t method, const void *user_ctx)
    {
        add_handler(url, method, exception_wrapper<ftn>, user_ctx);
    }

    template <class T, void (T::*ftn)(esp32::http_request &)> inline void add_handler_ftn(const auto url, httpd_method_t method)
    {
        add_handler_with_exceptions<server_url_ftn<T, ftn>>(url, method, this);
    }

    template <const auto file_pathT, const auto content_typeT> inline void add_fs_file_handler(const char *url, httpd_method_t method = HTTP_GET)
    {
        add_handler_with_exceptions<serve_fs_file<file_pathT, content_typeT>>(url, method, nullptr);
    }

    template <const auto buf, const auto len, const auto sha_256, bool is_gz, const auto content_type>
    inline void add_array_handler(const char *url, httpd_method_t method = HTTP_GET)
    {
        add_handler_with_exceptions<serve_array<buf, len, sha_256, is_gz, content_type>>(url, method, nullptr);
    }

    template <class T, class Y, void (T::*ftn)(Y)> inline void queue_work(Y arg)
    {
        const auto pair = new std::pair<T *, Y>(reinterpret_cast<T *>(this), std::forward<Y>(arg));
        const auto error = httpd_queue_work(server_, http_work_ftn<T, Y, ftn>, pair);
        if (error != ESP_OK)
        {
            delete pair;
            CHECK_THROW_ESP(error);
        }
    }

  private:
    template <class T, class Y, void (T::*ftn)(Y)> static __attribute__((noinline)) void http_work_ftn(void *arg)
    {
        try
        {
            const auto pair = reinterpret_cast<std::pair<T *, Y> *>(arg);
            const auto p_this = std::get<0>(*pair);
            (p_this->*ftn)(std::forward<Y>(std::get<1>(*pair)));
            delete pair;
        }
        catch (const std::exception &ex)
        {
            ESP_LOGW(WEBSERVER_TAG, "Failed to run http aync work with:%s", ex.what());
        }
    }

    // general url handler
    template <class T, void (T::*ftn)(esp32::http_request &)> static void __attribute__((noinline)) server_url_ftn(httpd_req_t *request_p)
    {
        auto p_this = reinterpret_cast<T *>(request_p->user_ctx);
        esp32::http_request request(request_p);
        (p_this->*ftn)(request);
    }

    // fs handler
    template <const auto file_pathT, const auto content_typeT> static void __attribute__((noinline)) serve_fs_file(httpd_req_t *request_p)
    {
        esp32::http_request request(request_p);
        esp32::fs_card_file_response response(request, file_pathT, content_typeT, false);
        response.send_response();
    }

    // array handler
    template <const auto buf, const auto len, const auto sha_256, bool is_gz, const auto content_type>
    static void __attribute__((noinline)) serve_array(httpd_req_t *request_p)
    {
        esp32::http_request request(request_p);
        esp32::array_response response(request, {buf, len}, sha_256, is_gz, content_type);
        response.send_response();
    }

    template <void (*url_handler)(httpd_req_t *)> static __attribute__((noinline)) esp_err_t exception_wrapper(httpd_req_t *r)
    {
        try
        {
            url_handler(r);
            return ESP_OK;
        }
        catch (const esp32::esp_exception &ex)
        {
            ESP_LOGW(WEBSERVER_TAG, "Failed to process request with:%s", ex.what());
            return ex.get_error();
        }
        catch (const std::exception &ex)
        {
            ESP_LOGW(WEBSERVER_TAG, "Failed to process request with:%s", ex.what());
            return ESP_FAIL;
        }
    }

  private:
    const uint16_t port_{};

  protected:
    httpd_handle_t server_{};
};
} // namespace esp32
