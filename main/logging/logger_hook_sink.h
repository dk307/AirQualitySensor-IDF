#pragma once

#include <string>

class logger_hook_sink
{
  public:
    virtual void log(const std::string &log) = 0;
    virtual ~logger_hook_sink() = default;
};
