#include "logger.h"
#include "logging_tags.h"
#include "util/helper.h"

#include <esp_log.h>
#include <string>
#include <vector>

class Esp32Hook;
Esp32Hook *esp32_hook_ = nullptr;

class Esp32Hook
{
  public:
    Esp32Hook()
    {
        esp32_hook_ = this;
        prev_hooker_ = esp_log_set_vprintf(esp32hook);
    }
    ~Esp32Hook()
    {
        esp_log_set_vprintf(prev_hooker_);
        esp32_hook_ = nullptr;
    }

    void add_sink(logger_hook_sink *sink)
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex_);
        sinks_.push_back(sink);
    }

    void remove_sink(logger_hook_sink *sink)
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex_);
        auto iter = std::find(sinks_.begin(), sinks_.end(), sink);
        if (iter != sinks_.end())
        {
            sinks_.erase(iter);
        }
    }

    auto sink_size()
    {
        std::lock_guard<esp32::semaphore> lock(sinks_mutex_);
        return sinks_.size();
    }

  private:
    static int esp32hook(const char *str, va_list arg)
    {
        auto &&h = logger::get_instance().hook_instance_;
        if (h)
        {
            auto return_value = h->call_sinks(str, arg);

            if (h->prev_hooker_)
            {
                return h->prev_hooker_(str, arg);
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
        vsnprintf(log_str.data(), length + 1, fmt, args);

        std::lock_guard<esp32::semaphore> lock(sinks_mutex_);
        for (auto &&sink : sinks_)
        {
            if (sink)
            {
                sink->log(log_str);
            }
        }

        recursion_--;
        return length;
    }

    vprintf_like_t prev_hooker_ = nullptr;
    std::atomic_short recursion_ = 0;
    esp32::semaphore sinks_mutex_;
    std::vector<logger_hook_sink *> sinks_;
};

logger::logger()
{
    esp_register_shutdown_handler(logging_shutdown_handler);
}

void logger::logging_shutdown_handler()
{
    std::lock_guard<esp32::semaphore> lock(logger::get_instance().hook_mutex_);
    ESP_LOGI(LOGGING_TAG, "Flushing custom loggers");

    if (logger::get_instance().sd_card_sink_instance_)
    {
        logger::get_instance().sd_card_sink_instance_->flush();
    }
}

bool logger::enable_sd_logging()
{
    std::lock_guard<esp32::semaphore> lock(hook_mutex_);

    ESP_LOGI(LOGGING_TAG, "Enabling sd card logging");

    hook_logger();

    if (sd_card_sink_instance_)
    {
        hook_instance_->remove_sink(sd_card_sink_instance_.get());
        sd_card_sink_instance_.reset();
    }

    sd_card_sink_instance_ = std::make_unique<sd_card_sink>();
    hook_instance_->add_sink(sd_card_sink_instance_.get());

    return true;
}

bool logger::enable_web_logging(const std::function<void(std::unique_ptr<std::string>)> &callbackP)
{
    std::lock_guard<esp32::semaphore> lock(hook_mutex_);

    ESP_LOGI(LOGGING_TAG, "Enabling web callback card logging");
    hook_logger();

    if (web_callback_sink_instance_)
    {
        hook_instance_->remove_sink(web_callback_sink_instance_.get());
        web_callback_sink_instance_.reset();
    }

    web_callback_sink_instance_ = std::make_unique<web_callback_sink>(callbackP);
    hook_instance_->add_sink(web_callback_sink_instance_.get());

    return true;
}

template <class T> void logger::remove_sink(std::unique_ptr<T> &p)
{
    std::lock_guard<esp32::semaphore> lock(hook_mutex_);
    if (p)
    {
        hook_instance_->remove_sink(p.get());
        p.reset();
    }

    if (hook_instance_)
    {
        if (hook_instance_->sink_size() == 0)
        {
            hook_instance_.reset();
        }
    }
}

void logger::disable_sd_logging()
{
    remove_sink(sd_card_sink_instance_);
    ESP_LOGI(LOGGING_TAG, "Disabled sd card logging");
}

void logger::disable_web_logging()
{
    remove_sink(web_callback_sink_instance_);
    ESP_LOGI(LOGGING_TAG, "Disabled web callback card logging");
}

void logger::hook_logger()
{
    if (!hook_instance_)
    {
        hook_instance_ = std::make_unique<Esp32Hook>();
    }
}

void logger::set_logging_level(const char *tag, esp_log_level_t level)
{
    // there is no good time to remove this so it stays for ever
    logging_tags.push_back(tag);

    ESP_LOGI(LOGGING_TAG, "Setting log level for %s to %d", tag, level);
    esp_log_level_set(logging_tags[logging_tags.size() - 1].data(), level);
}

logger &logger::get_instance()
{
    static logger instance;
    return instance;
}
