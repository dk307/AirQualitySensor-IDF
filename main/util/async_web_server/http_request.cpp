#include "http_request.h"

namespace esp32
{

  http_method http_request::method() const
  {
    return static_cast<http_method>(this->req_->method);
  }

  std::string http_request::url() const
  {
    auto *str = strchr(this->req_->uri, '?');
    if (str == nullptr)
    {
      return this->req_->uri;
    }
    return std::string(this->req_->uri, str - this->req_->uri);
  }
}
