#include "logging.h"
#include "logging_tags.h"
#include "util/helper.h"

#include <esp_log.h>
#include <string>
#include <vector>

logger logger::instance;

class Esp32Hook;
Esp32Hook *_esp32Hook = nullptr;

class Esp32Hook
{
  public:
    Esp32Hook()
    {
        _esp32Hook = this;
        // _prevLogger = esp_log_set_vprintf(esp32hook);
    }
    ~Esp32Hook()
    {
        // esp_log_set_vprintf(_prevLogger);
        _esp32Hook = nullptr;
    }

    void add_sink(serial_hook_sink *sink)
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex);
        sinks.push_back(sink);
    }

    void remove_sink(serial_hook_sink *sink)
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex);
        auto iter = std::find(sinks.begin(), sinks.end(), sink);
        if (iter != sinks.end())
        {
            sinks.erase(iter);
        }
    }

    auto sink_size()
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex);
        return sinks.size();
    }

  private:
    static int esp32hook(const char *str, va_list arg)
    {
        auto h = logger::instance.serial_hook_instance;
        if (h)
        {
            auto return_value = h->call_sinks(str, arg);

            if (h->_prevLogger)
            {
                return h->_prevLogger(str, arg);
            }
            return return_value;
        }
        return 0;
    }

    int call_sinks(const char *fmt, va_list args)
    {
        if (recursion_)
        {
            return 0;
        }

        recursion_++;
        std::string log_str;
        const size_t length = vsnprintf(nullptr, 0, fmt, args);

        log_str.resize(length);
        vsnprintf(&log_str[0], length + 1, fmt, args);

        std::lock_guard<esp32::semaphore> lock(sinks_mutex);
        for (auto &&sink : sinks)
        {
            if (sink)
            {
                sink->log(log_str);
            }
        }

        recursion_--;
        return length;
    }

    vprintf_like_t _prevLogger = nullptr;
    std::atomic_short recursion_ = 0;
    esp32::semaphore sinks_mutex;
    std::vector<serial_hook_sink *> sinks;
};

bool logger::enable_sd_logging()
{
    std::lock_guard<esp32::semaphore> lock(serial_hook_mutex);

    ESP_LOGI(LOGGING_TAG, "Enabling sd card logging");

    hook_uart_logger();

    if (sd_card_sink_instance)
    {
        serial_hook_instance->remove_sink(sd_card_sink_instance.get());
        sd_card_sink_instance.reset();
    }

    sd_card_sink_instance = std::make_unique<sd_card_sink>();
    serial_hook_instance->add_sink(sd_card_sink_instance.get());

    return true;
}

bool logger::enable_web_logging(const std::function<void(const std::string &)> &callbackP)
{
    std::lock_guard<esp32::semaphore> lock(serial_hook_mutex);

    ESP_LOGI(LOGGING_TAG, "Enabling web callback card logging");
    hook_uart_logger();

    if (web_callback_sink_instance)
    {
        serial_hook_instance->remove_sink(web_callback_sink_instance.get());
        web_callback_sink_instance.reset();
    }

    web_callback_sink_instance = std::make_unique<web_callback_sink>(callbackP);
    serial_hook_instance->add_sink(web_callback_sink_instance.get());

    return true;
}

template <class T> void logger::remove_sink(std::unique_ptr<T> &p)
{
    std::lock_guard<esp32::semaphore> lock(serial_hook_mutex);

    if (p)
    {
        serial_hook_instance->remove_sink(p.get());
        p.reset();
    }

    if (serial_hook_instance)
    {
        if (!serial_hook_instance->sink_size())
        {
            delete serial_hook_instance;
            p.reset();
        }
    }
}

void logger::disable_sd_logging()
{
    remove_sink(sd_card_sink_instance);
    ESP_LOGI(LOGGING_TAG, "Disabled sd card logging");
}

void logger::disable_web_logging()
{
    remove_sink(web_callback_sink_instance);
    ESP_LOGI(LOGGING_TAG, "Disabled web callback card logging");
}

void logger::hook_uart_logger()
{
    if (!serial_hook_instance)
    {
        serial_hook_instance = new Esp32Hook();
    }
}

void logger::set_logging_level(const char *tag, esp_log_level_t level)
{
    ESP_LOGI(LOGGING_TAG, "Setting log level for %s to %d", tag, level);
    esp_log_level_set(tag, level);
}
