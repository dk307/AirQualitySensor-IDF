#pragma once

#include <esp_http_server.h>

#include <filesystem>

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

    protected:
        const http_request *req_;
    };

    class fs_card_file_response : http_response
    {
    public:
        fs_card_file_response(const http_request *req, const std::filesystem::path &file_path, const std::string &content_type, bool download)
            : http_response(req), file_path_(file_path), content_type_(content_type), download_(download)
        {
        }

        esp_err_t send_response() override;

    private:
        const std::filesystem::path file_path_;
        const std::string content_type_;
        const bool download_;
    };

}