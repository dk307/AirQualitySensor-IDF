#pragma once

#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"

#include "sd_card_sink.h"
#include "web_callback_sink.h"

#include <esp_log.h>

class Esp32Hook;

class logger : esp32::noncopyable
{
  public:
    bool enable_sd_logging();
    void disable_sd_logging();
    bool enable_web_logging(const std::function<void(const std::string &c)> &callbackP);
    void disable_web_logging();

    auto get_general_logging_level()
    {
        return esp_log_level_get("*");
    }

    void set_logging_level(const char *tag, esp_log_level_t level);

    void set_general_logging_level(esp_log_level_t level)
    {
        set_logging_level("*", level);
    }

    static logger instance;

  private:
    esp32::semaphore serial_hook_mutex;
    std::unique_ptr<sd_card_sink> sd_card_sink_instance;
    std::unique_ptr<web_callback_sink> web_callback_sink_instance;

    Esp32Hook *serial_hook_instance{nullptr};

    void hook_uart_logger();

    template <class T> void remove_sink(std::unique_ptr<T> &p);
    friend class Esp32Hook;
};