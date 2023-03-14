#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "util/exceptions.h"
#include "util/noncopyable.h"

#include <esp_http_server.h>

namespace esp32
{
class http_request : esp32::noncopyable
{
  public:
    http_request(httpd_req_t *req) : req_(req)
    {
        configASSERT(req_);
    }

    bool has_header(const char *header);
    std::optional<std::string> get_header(const char *header) const;
    std::vector<std::optional<std::string>> get_url_arguments(const std::vector<std::string> &names);
    std::vector<std::optional<std::string>> get_form_url_encoded_arguments(const std::vector<std::string> &names);

    http_method method() const;
    std::string url() const;
    std::string client_ip_address() const;
    size_t content_length() const
    {
        return this->req_->content_len;
    }

    esp_err_t read_body(const std::function<esp_err_t(const std::vector<uint8_t> &data)> &callback);

    static bool url_decode_in_place(std::string &url);

  protected:
    httpd_req_t *req_;

    friend class http_response;
    friend class array_response;
    friend class fs_card_file_response;
    friend class event_source_connection;

    static void extract_parameters(const std::vector<char> &query_str, const std::vector<std::string> &names,
                                   std::vector<std::optional<std::string>> &result);

    static bool url_decode_in_place(std::string &url, std::string::iterator begin, std::string::iterator end);
};
} // namespace esp32
