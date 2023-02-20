#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

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

        // void redirect(const std::string &url);
        // void send(AsyncResponse *response);
        // void send(int code, const char *content_type = nullptr, const char *content = nullptr);

        // AsyncEmptyResponse *beginResponse(int code, const char *content_type)
        // {
        //     auto *res = new AsyncEmptyResponse(this); // NOLINT(cppcoreguidelines-owning-memory)
        //     this->init_response_(res, 200, content_type);
        //     return res;
        // }

        // // NOLINTNEXTLINE(readability-identifier-naming)
        // AsyncWebServerResponse *beginResponse(int code, const char *content_type, const std::string &content)
        // {
        //     auto *res = new AsyncWebServerResponse(this, content); // NOLINT(cppcoreguidelines-owning-memory)
        //     this->init_response_(res, code, content_type);
        //     return res;
        // }

        // AsyncResponseStream *beginResponseStream(const char *content_type)
        // {
        //     auto *res = new AsyncResponseStream(this); // NOLINT(cppcoreguidelines-owning-memory)
        //     this->init_response_(res, 200, content_type);
        //     return res;
        // }

        // bool hasParam(const std::string &name) { return this->getParam(name) != nullptr; }
        // AsyncWebParameter *getParam(const std::string &name);

        // bool hasArg(const char *name) { return this->hasParam(name); }
        // std::string arg(const std::string &name)
        // {
        //     auto *param = this->getParam(name);
        //     if (param)
        //     {
        //         return param->value();
        //     }
        //     return {};
        // }

        // operator httpd_req_t *() const { return this->req_; }
        // std::optional<std::string> get_header(const char *name) const;

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

        // AsyncResponse *rsp_{};
        // std::map<std::string, AsyncWebParameter *> params_;
        // web_server_request(httpd_req_t *req) : req_(req) {}
        // void init_response_(AsyncResponse *rsp, int code, const char *content_type);
    };

#define __STR(x) #x
#define CHECK_HTTP_REQUEST(error_) CHECK_THROW(error_, __STR(__FUNC__ ":" __LINE__), esp32::http_request_exception)
}
