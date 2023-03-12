#pragma once

#include "logger_hook_sink.h"
#include "util/semaphore_lockable.h"
#include "util/static_queue.h"
#include "util/task_wrapper.h"
#include <functional>
#include <mutex>

class web_callback_sink final : public logger_hook_sink
{
  public:
    web_callback_sink(const std::function<void(std::unique_ptr<std::string>)> &callback)
        : background_log_task_([this] { flush_callback(); }), callback_(callback)
    {
        configASSERT(callback_);
        background_log_task_.spawn_same("web_log_sink", 2 * 1024, tskIDLE_PRIORITY);
    }

    ~web_callback_sink()
    {
        background_log_task_.kill();
    }

    void log(const std::string &log) override
    {
        auto str_cpy = new std::string(log);
        queue_.enqueue(str_cpy, portMAX_DELAY);
    }

  private:
    void flush_callback()
    {
        while (true)
        {
            std::string *log;
            if (queue_.dequeue(log, portMAX_DELAY))
            {
                // transfer ownership
                auto log_uni_ptr = std::unique_ptr<std::string>(log);
                callback_(std::move(log_uni_ptr));
            }
        }

        vTaskDelete(NULL);
    }

  private:
    esp32::static_queue<std::string *, 512> queue_;
    esp32::task background_log_task_;
    const std::function<void(std::unique_ptr<std::string>)> callback_;
};
