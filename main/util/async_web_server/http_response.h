#pragma once

#include <esp_http_server.h>
#include <string>

namespace esp32
{
class http_request;

class http_response
{
  public:
    http_response(const http_request *req) : req_(req)
    {
    }
    virtual ~http_response()
    {
    }

    void add_common_headers();

    void add_header(const char *field, const char *value);

    void redirect(const std::string &url);
    void send_empty_200();
    void send_error(httpd_err_code_t code, const char *message = nullptr);

  protected:
    const http_request *req_;
};

class array_response final : http_response
{
  public:
    array_response(const http_request *req, const uint8_t *buf, ssize_t buf_len, bool is_gz, const char *content_type)
        : http_response(req), buf_(buf), buf_len_(buf_len), content_type_(content_type), is_gz_(is_gz)
    {
    }

    void send_response();

    static void send_response(esp32::http_request *request, const std::string &data, const char *content_type);

  private:
    const uint8_t *buf_;
    const ssize_t buf_len_;
    const char *content_type_;
    const bool is_gz_;
};

class fs_card_file_response final : http_response
{
  public:
    fs_card_file_response(const http_request *req, const char *file_path, const char *content_type, bool download)
        : http_response(req), file_path_(file_path), content_type_(content_type), download_(download)
    {
    }

    void send_response();

  private:
    const char *file_path_;
    const char *content_type_;
    const bool download_;
};
} // namespace esp32