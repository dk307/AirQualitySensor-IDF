#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

#include <esp_http_server.h>

#include "util/exceptions.h"

namespace esp32
{
    class http_request
    {
    public:
        http_request(httpd_req_t *req) : req_(req)
        {
        }

        bool has_header(const char *header);
        std::optional<std::string> get_header(const char *header);
        std::vector<std::optional<std::string>> get_url_arguments(const std::vector<std::string> &names);
        std::vector<std::optional<std::string>> get_form_url_encoded_arguments(const std::vector<std::string> &names);

        http_method method() const;
        std::string url() const;
        std::string client_ip_address() const;
        size_t contentLength() const { return this->req_->content_len; }

        esp_err_t read_body(const std::function<esp_err_t(const std::vector<uint8_t> &data)> &callback);

        static bool url_decode_in_place(std::string &url);

    protected:
        httpd_req_t *req_;

        friend class http_response;
        friend class array_response;
        friend class fs_card_file_response;

        static void extract_parameters(const std::vector<char> &query_str,
                                       const std::vector<std::string> &names,
                                       std::vector<std::optional<std::string>> &result);

        static bool url_decode_in_place(std::string &url, std::string::iterator begin, std::string::iterator end);
    };

#define __STR(x) #x
#define CHECK_HTTP_REQUEST(error_) CHECK_THROW(error_, __STR(__LINE__), esp32::http_request_exception)
}
