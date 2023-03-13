#include "http_event_source.h"
#include "logging/logging_tags.h"
#include "util/helper.h"
#include <esp_log.h>
#include <mutex>
#include <string>

namespace esp32
{

#define CRLF_STR "\r\n"
#define CRLF_LEN (sizeof(CRLF_STR) - 1)

event_source_connection::event_source_connection(event_source *source, http_request *request) : source_(source)
{
    auto req = request->req_;
    hd_ = req->handle;
    fd_ = httpd_req_to_sockfd(req);

    CHECK_THROW_ESP(httpd_resp_set_status(req, HTTPD_200));
    CHECK_THROW_ESP(httpd_resp_set_type(req, "text/event-stream"));
    CHECK_THROW_ESP(httpd_resp_set_hdr(req, "Cache-Control", "no-cache"));
    CHECK_THROW_ESP(httpd_resp_set_hdr(req, "Connection", "keep-alive"));

    CHECK_THROW_ESP(httpd_resp_send_chunk(req, CRLF_STR, CRLF_LEN));
    req->sess_ctx = this;
    req->free_ctx = event_source_connection::destroy;
}

void event_source_connection::destroy(void *ptr)
{
    ESP_LOGI(WEBSERVER_TAG, "events disconnect");
    auto *connection = static_cast<event_source_connection *>(ptr);
    connection->source_->connections_.erase(connection);
    delete connection;
}

void event_source_connection::try_send(const std::string_view &message, const std::string_view &event, uint32_t id, uint32_t reconnect)
{
    if (fd_ == 0)
    {
        return;
    }

    std::string event_str;
    event_str.reserve(256);

    if (reconnect)
    {
        event_str.append("retry: ", sizeof("retry: ") - 1);
        event_str.append(esp32::string::to_string(reconnect));
        event_str.append(CRLF_STR, CRLF_LEN);
    }

    if (id)
    {
        event_str.append("id: ", sizeof("id: ") - 1);
        event_str.append(esp32::string::to_string(id));
        event_str.append(CRLF_STR, CRLF_LEN);
    }

    if (event.length())
    {
        event_str.append("event: ", sizeof("event: ") - 1);
        event_str.append(event);
        event_str.append(CRLF_STR, CRLF_LEN);
    }

    if (message.length())
    {
        event_str.append("data: ", sizeof("data: ") - 1);
        event_str.append(message);
        event_str.append(CRLF_STR, CRLF_LEN);
    }

    if (event_str.empty())
    {
        return;
    }

    event_str.append(CRLF_STR, CRLF_LEN);

    // Sending chunked content prelude
    const auto pre_event_str = esp32::string::snprintf("%x" CRLF_STR, 4 * sizeof(event_str.size()) + CRLF_LEN, event_str.size());
    httpd_socket_send(hd_, fd_, pre_event_str.c_str(), pre_event_str.size(), 0);

    // Sendiing content chunk
    httpd_socket_send(hd_, fd_, event_str.c_str(), event_str.size(), 0);

    // Indicate end of chunk
    httpd_socket_send(hd_, fd_, CRLF_STR, CRLF_LEN, 0);
}

event_source::~event_source()
{
    for (auto &&ses : connections_)
    {
        delete ses;
    }
}

void event_source::add_request(http_request *request)
{
    auto connection = new event_source_connection(this, request);
    std::lock_guard<esp32::semaphore> lock(connections_mutex_);
    connections_.insert(connection);
}

void event_source::try_send(const char *message, const char *event, uint32_t id, uint32_t reconnect)
{
    std::set<event_source_connection *> connections_copy;
    {
        std::lock_guard<esp32::semaphore> lock(connections_mutex_);
        connections_copy = connections_;
    }

    for (auto *ses : connections_copy)
    {
        ses->try_send(message, event, id, reconnect);
    }
}

size_t event_source::connection_count() const
{
    std::lock_guard<esp32::semaphore> lock(connections_mutex_);
    return connections_.size();
}

} // namespace esp32