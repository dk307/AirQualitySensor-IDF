#pragma once

#include "hardware/sd_card.h"
#include "logger_hook_sink.h"
#include "logging/logging_tags.h"
#include "util/cores.h"
#include "util/filesystem/file.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/helper.h"
#include "util/psram_allocator.h"
#include "util/semaphore_lockable.h"
#include "util/static_queue.h"
#include "util/task_wrapper.h"
#include <esp_log.h>
#include <filesystem>
#include <mutex>

class sd_card_sink final : public logger_hook_sink
{
  public:
    sd_card_sink() : background_log_task_([this] { flush_to_disk_task(); })
    {
        background_log_task_.spawn_pinned("sd_card_sink", 4 * 1024, tskIDLE_PRIORITY, esp32::main_task_core);
    }

    ~sd_card_sink()
    {
        background_log_task_.kill();
        flush_to_disk();
        sd_card_file_.reset();
    }

    void log(const std::string &log) override
    {
        std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex_);
        fs_buffer_.insert(fs_buffer_.end(), log.begin(), log.end());
    }

    void flush()
    {
        flush_to_disk();
    }

  private:
    void flush_to_disk_task()
    {
        try
        {
            esp32::filesystem::create_directory(sd_card_path_.parent_path());
            do
            {
                vTaskDelay(pdMS_TO_TICKS(60 * 1000));
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
            std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex_);
            if (fs_buffer_.empty())
            {
                return;
            }
        }

        std::lock_guard<esp32::semaphore> lock(flush_to_disk_mutex_);
        if (sd_card_max_files_ > 1 && should_rotate())
        {
            for (auto i = sd_card_max_files_ - 2; i >= 0; i--)
            {
                auto a = sd_card_path_;
                if (i)
                {
                    a.replace_filename(sd_card_path_.stem().native() + esp32::string::to_string(i) + sd_card_path_.extension().native());
                }

                if (esp32::filesystem::exists(a))
                {
                    auto b = sd_card_path_;
                    b.replace_filename(sd_card_path_.stem().native() + esp32::string::to_string(i) + sd_card_path_.extension().native());
                    esp32::filesystem::remove(b);
                    esp32::filesystem::rename(a, b);
                }
            }
            sd_card_file_.reset();
        }

        if (!sd_card_file_)
        {
            sd_card_file_ = std::make_unique<esp32::filesystem::file>(sd_card_path_.c_str(), "a");
        }

        {
            std::lock_guard<esp32::semaphore> lock(fs_buffer_mutex_);
            sd_card_file_->write(reinterpret_cast<uint8_t *>(fs_buffer_.data()), 1, fs_buffer_.size()); // ignore error
            fs_buffer_.clear();
        }
        sd_card_file_->flush();
    }

    bool should_rotate()
    {
        esp32::filesystem::file_info file_info{sd_card_path_};
        return (file_info.exists() && (file_info.size() > (16 * 1024)));
    }

  private:
    std::unique_ptr<esp32::filesystem::file> sd_card_file_;
    const std::filesystem::path sd_card_path_{std::filesystem::path(sd_card::mount_point) / "logs/log.txt"};
    const uint8_t sd_card_max_files_ = 5;
    esp32::semaphore flush_to_disk_mutex_;
    esp32::semaphore fs_buffer_mutex_;
    std::vector<char, esp32::psram::allocator<char>> fs_buffer_;
    esp32::task background_log_task_;
};
