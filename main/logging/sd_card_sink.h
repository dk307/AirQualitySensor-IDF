#pragma once

#include "serial_hook_sink.h"

#include "hardware/sd_card.h"
#include "util/filesystem/file.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/helper.h"
#include "util/semaphore_lockable.h"
#include "util/static_queue.h"
#include "util/task_wrapper.h"
#include "logging/logging_tags.h"

#include <filesystem>
#include <mutex>
#include <esp_log.h>


class sd_card_sink final : public serial_hook_sink
{
  public:
    sd_card_sink() : background_log_task(std::bind(&sd_card_sink::flush_to_disk_task, this))
    {
        background_log_task.spawn_same("sd_card_sink", 4096, tskIDLE_PRIORITY);
    }

    ~sd_card_sink()
    {
        background_log_task.kill();
        flush_to_disk();
        sd_card_file.reset();
    }

    void log(const std::string &log) override
    {
        std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex);
        fs_buffer.insert(fs_buffer.end(), log.begin(), log.end());
    }

    void flush_to_disk_task()
    {
        try
        {
            esp32::filesystem::create_directory(sd_card_path.parent_path());
            do
            {
                vTaskDelay(pdMS_TO_TICKS(15 * 1000));
                flush_to_disk();
            } while (true);
        }
        catch (const std::exception &ex)
        {
            ESP_LOGE(LOGGING_TAG, "Failed to log to sd card with :%s", ex.what());
        }

        vTaskDelete(NULL);
    }

    void flush_to_disk()
    {
        {
            std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex);
            if (fs_buffer.empty())
            {
                return;
            }
        }

        if (sd_card_max_files > 1 && should_rotate())
        {
            for (auto i = sd_card_max_files - 2; i >= 0; i--)
            {
                auto a = sd_card_path;
                if (i)
                {
                    a.replace_filename(sd_card_path.filename().native() + esp32::string::to_string(i));
                }

                if (esp32::filesystem::exists(a))
                {
                    auto b = sd_card_path;
                    b.replace_filename(sd_card_path.filename().native() + esp32::string::to_string(i));
                    esp32::filesystem::remove(b);
                    esp32::filesystem::rename(a, b);
                }
            }
            sd_card_file.reset();
        }

        if (!sd_card_file)
        {
            sd_card_file = std::make_unique<esp32::filesystem::file>(sd_card_path.c_str(), "a");
        }

        {
            std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex);
            sd_card_file->write(reinterpret_cast<uint8_t *>(fs_buffer.data()), 1, fs_buffer.size()); // ignore error
            fs_buffer.clear();
        }
        sd_card_file->flush();
    }

    bool should_rotate()
    {
        esp32::filesystem::file_info file_info{sd_card_path};
        return (file_info.exists() && (file_info.size() > (8 * 1024)));
    }

  private:
    std::unique_ptr<esp32::filesystem::file> sd_card_file;
    const std::filesystem::path sd_card_path{std::filesystem::path(sd_card::mount_point) / "logs/log.txt"};
    const uint8_t sd_card_max_files = 5;
    esp32::semaphore fs_buffer_mutex;
    std::vector<char> fs_buffer;
    esp32::task background_log_task;
};
