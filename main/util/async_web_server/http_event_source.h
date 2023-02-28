#pragma once

#include "http_request.h"
#include "http_response.h"
#include "util/noncopyable.h"

#include <set>

namespace esp32
{
class event_source;

class event_source_connection : esp32::noncopyable
{
  public:
    event_source_connection(event_source *source, esp32::http_request *request);

    static void destroy(void *ptr);
    void send(const char *message, const char *event, uint32_t id, uint32_t reconnect);

  protected:
    httpd_handle_t hd_{};
    event_source *source_;
    int fd_{};
};

class event_source : esp32::noncopyable
{
  public:
    ~event_source();
    void add_request(http_request *request);
    void send(const char *message, const char *event, uint32_t id, uint32_t reconnect);

    size_t connection_count() const; 

  protected:
    friend class event_source_connection;

    std::set<event_source_connection *> connections_;
};
} // namespace esp32