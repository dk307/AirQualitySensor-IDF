#pragma once

#include <string>

class serial_hook_sink
{
  public:
    virtual void log(const std::string &log) = 0;
    virtual ~serial_hook_sink() = default;
};
