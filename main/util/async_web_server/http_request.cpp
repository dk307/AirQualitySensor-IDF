#include "http_request.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/helper.h"

#include <esp_log.h>
#include <memory>
#include <sys/socket.h>

namespace esp32
{
bool http_request::has_header(const char *header)
{
    return httpd_req_get_hdr_value_len(req_, header) != 0;
}

std::optional<std::string> http_request::get_header(const char *header) const
{
    const size_t buf_len = httpd_req_get_hdr_value_len(req_, header);
    if (buf_len == 0)
    {
        return std::nullopt;
    }

    std::string buf;
    buf.resize(buf_len);

    CHECK_HTTP_REQUEST(httpd_req_get_hdr_value_str(req_, header, buf.data(), buf_len + 1));
    return buf;
}

std::vector<std::optional<std::string>> http_request::get_url_arguments(const std::vector<std::string> &names)
{
    std::vector<std::optional<std::string>> result;
    result.resize(names.size());

    auto query_len = httpd_req_get_url_query_len(req_);
    if (query_len == 0)
    {
        return result;
    }

    std::vector<char> query_str;
    query_str.resize(query_len + 1);

    CHECK_HTTP_REQUEST(httpd_req_get_url_query_str(req_, query_str.data(), query_str.size()));

    extract_parameters(query_str, names, result);
    return result;
}

void http_request::extract_parameters(const std::vector<char> &query_str, const std::vector<std::string> &names,
                                      std::vector<std::optional<std::string>> &result)
{
    auto query_val = std::make_unique<char[]>(query_str.size());

    for (auto i = 0; i < names.size(); i++)
    {
        auto res = httpd_query_key_value(query_str.data(), names[i].c_str(), query_val.get(), query_str.size());
        if (res == ESP_OK)
        {
            result[i] = std::string(query_val.get());
            url_decode_in_place(result[i].value());
        }
    }
}

std::vector<std::optional<std::string>> http_request::get_form_url_encoded_arguments(const std::vector<std::string> &names)
{
    std::vector<std::optional<std::string>> result;
    result.resize(names.size());

    ESP_LOGD(WEBSERVER_TAG, "Body size is %d", req_->content_len);

    if (req_->content_len > 16 * 1024) // 16k max size
    {
        CHECK_HTTP_REQUEST(ESP_ERR_NO_MEM);
    }

    std::vector<char> data;
    data.reserve(req_->content_len + 1);

    CHECK_HTTP_REQUEST(read_body([&data](const std::vector<uint8_t> &chunk) {
        data.insert(data.end(), chunk.cbegin(), chunk.cend());
        return ESP_OK;
    }));

    data.resize(req_->content_len + 1);
    extract_parameters(data, names, result);
    return result;
}

esp_err_t http_request::read_body(const std::function<esp_err_t(const std::vector<uint8_t> &)> &callback)
{
    const auto content_length = req_->content_len;
    if (content_length == 0)
    {
        return ESP_OK;
    }
    std::vector<uint8_t> data;
    data.reserve(std::min<size_t>(16 * 1024, content_length));

    size_t received = 0;
    while (received < content_length)
    {
        data.resize(data.capacity());
        auto len = httpd_req_recv(req_, reinterpret_cast<char *>(data.data()), data.size());
        if (len <= 0)
        {
            if (len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGE(WEBSERVER_TAG, "HTTP receive timeout");
            }
            return ESP_FAIL;
        }
        data.resize(len);
        const auto res = callback(data);
        if (res != ESP_OK)
        {
            return res;
        }
        received += len;
    }

    return ESP_OK;
}

http_method http_request::method() const
{
    return static_cast<http_method>(req_->method);
}

std::string http_request::url() const
{
    auto *str = strchr(req_->uri, '?');
    if (str == nullptr)
    {
        return req_->uri;
    }
    return std::string(req_->uri, str - req_->uri);
}

std::string http_request::client_ip_address() const
{
    auto socket = httpd_req_to_sockfd(req_); // This is the socket for the request

    struct sockaddr_in saddr
    {
    };
    socklen_t saddr_len = sizeof(saddr);

    if (getpeername(socket, (struct sockaddr *)&saddr, &saddr_len) == 0)
    {
        char buf[32]{};
        inet_ntoa_r(saddr.sin_addr, buf, 32);
        return buf;
    }

    CHECK_HTTP_REQUEST(ESP_FAIL);
}

bool http_request::url_decode_in_place(std::string &url)
{
    return url_decode_in_place(url, url.begin(), url.end());
}

bool http_request::url_decode_in_place(std::string &url, std::string::iterator begin, std::string::iterator end)
{
    bool res = true;
    std::array<char, 3> hex{0, 0, 0};

    // We know that the result string is going to be shorter or equal to the original,
    // so we'll do an in-place conversion to save on memory usage.

    auto write = begin;
    auto read = begin;

    for (; res && read != end;)
    {
        if (*read == '%')
        {
            if (std::distance(read, end) > 2)
            {
                hex[0] = *(++read);
                hex[1] = *(++read);
                ++read;

                auto a = std::isxdigit(static_cast<unsigned char>(hex[0]));
                auto b = std::isxdigit(static_cast<unsigned char>(hex[1]));

                res = a && b;

                if (res)
                {
                    try
                    {
                        *(write++) = static_cast<char>(std::stoul(hex.data(), nullptr, 16));
                    }
                    catch (...)
                    {
                        res = false;
                    }
                }
            }
            else
            {
                res = false;
            }
        }
        else
        {
            // Just keep going
            *write = *read;
            ++read;
            ++write;
        }
    }

    if (res)
    {
        *write = '\0';
        url.erase(write, end);
    }

    for (auto &c : url)
    {
        if (c == '+')
        {
            c = ' ';
        }
    }
    return res;
}
} // namespace esp32