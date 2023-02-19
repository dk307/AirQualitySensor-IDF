#pragma once

#include <esp_http_server.h>

namespace esp32
{
    class http_request;

    class http_response
    {
    public:
        http_response(const http_request *req) : req_(req)
        {
        }
        virtual ~http_response() {}

        virtual esp_err_t send_response() = 0;

        esp_err_t add_common_headers();

    protected:
        const http_request *req_;
    };

    class array_gz_response final : http_response
    {
    public:
        array_gz_response(const http_request *req, const uint8_t *buf, ssize_t buf_len, const char *content_type)
            : http_response(req), buf_(buf), buf_len_(buf_len), content_type_(content_type)
        {
        }

        esp_err_t send_response() override;

    private:
        const uint8_t *buf_;
        const ssize_t buf_len_;
        const char *content_type_;
    };

    class fs_card_file_response final : http_response
    {
    public:
        fs_card_file_response(const http_request *req, const char *file_path, const char *content_type, bool download)
            : http_response(req), file_path_(file_path), content_type_(content_type), download_(download)
        {
        }

        esp_err_t send_response() override;

    private:
        const char *file_path_;
        const char *content_type_;
        const bool download_;
    };

}