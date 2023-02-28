#pragma once

#include "serial_hook_sink.h"

#include "util/semaphore_lockable.h"
#include "util/static_queue.h"
#include "util/task_wrapper.h"

#include <functional>
#include <mutex>

class web_callback_sink final : public serial_hook_sink
{
  public:
    web_callback_sink(const std::function<void(const std::string &)> &callbackP)
        : background_log_task(std::bind(&web_callback_sink::flush_callback, this)), callback(callbackP)
    {
        background_log_task.spawn_same("web_callback_sink", 4096, tskIDLE_PRIORITY);
    }

    ~web_callback_sink()
    {
        background_log_task.kill();
    }

    void log(const std::string &log) override
    {
        auto str_cpy = new std::string(log);
        queue.enqueue(str_cpy, portMAX_DELAY);
    }

  private:
    void flush_callback()
    {
        while (true)
        {
            std::string *c;
            if (queue.dequeue(c, portMAX_DELAY))
            {
                callback(*c);
                delete c;
            }
        }

        vTaskDelete(NULL);
    }

  private:
    esp32::static_queue<std::string *, 1024> queue;
    esp32::task background_log_task;
    const std::function<void(const std::string &)> callback;
};
