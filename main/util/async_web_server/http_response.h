#pragma once

#include "util/noncopyable.h"
#include <esp_http_server.h>
#include <optional>
#include <set>
#include <span>
#include <string_view>

namespace esp32
{
class http_request;

class http_response : esp32::noncopyable
{
  public:
    http_response(const http_request &req) : request_(req)
    {
    }
    virtual ~http_response() = default;

    void add_common_headers();
    void add_header(const char *field, const char *value);
    void redirect(const std::string &url);
    void send_empty_200();
    void send_error(httpd_err_code_t code, const char *message = nullptr);

  protected:
    const http_request &request_;
};

class array_response final : http_response
{
  public:
    array_response(const http_request &req, const std::span<const uint8_t> &buf, const std::optional<std::string_view> &sha256, bool is_gz,
                   const std::string_view &content_type)
        : http_response(req), buf_(buf), sha256_(sha256), content_type_(content_type), is_gz_(is_gz)
    {
    }

    void send_response();

    static void send_response(esp32::http_request &request, const std::string_view &data_str, const std::string_view &content_type);

  private:
    const std::span<const uint8_t> buf_;
    const std::optional<std::string_view> sha256_;
    const std::string_view content_type_;
    const bool is_gz_;
};

class fs_card_file_response final : http_response
{
  public:
    fs_card_file_response(const http_request &req, const std::string_view &file_path, const std::string_view &content_type, bool download)
        : http_response(req), file_path_(file_path), content_type_(content_type), download_(download)
    {
    }

    void send_response();

  private:
    const std::string_view file_path_;
    const std::string_view content_type_;
    const bool download_;
};

} // namespace esp32