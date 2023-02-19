#pragma once

#include <vector>
#include <esp_http_server.h>

#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_response.h"

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
        void add_handler(const char *url, httpd_method_t method, url_handler request_handler);

        template <const auto file_pathT, const auto content_typeT>
        static esp_err_t serve_fs_file(httpd_req_t *request_p)
        {
            esp32::http_request request(request_p);
            esp32::fs_card_file_response response(&request, file_pathT, content_typeT, false);
            return response.send_response();
        }

        template <const auto file_pathT, const auto content_typeT>
        inline void add_fs_file_handler(const char *url, httpd_method_t method = HTTP_GET)
        {
            add_handler(url, method, serve_fs_file<file_pathT, content_typeT>);
        }

    private:
        const uint16_t port_{};
        httpd_handle_t server_{};
        std::vector<httpd_uri_t> handlers_;
    };
}
