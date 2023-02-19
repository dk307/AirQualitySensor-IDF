#pragma once

#include <string>
#include <esp_http_server.h>

namespace esp32
{
    class http_request
    {
    public:
        http_request(httpd_req_t *req) : req_(req)
        {
        }

        http_method method() const;
        std::string url() const;
        std::string host() const;
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

    protected:
        httpd_req_t *req_;

        friend class http_response;
        friend class fs_card_file_response;

        // AsyncResponse *rsp_{};
        // std::map<std::string, AsyncWebParameter *> params_;
        // web_server_request(httpd_req_t *req) : req_(req) {}
        // void init_response_(AsyncResponse *rsp, int code, const char *content_type);
    };
}
