#pragma once

#include "util/noncopyable.h"
#include "util/timer/timer.h"
#include <atomic>
#include <string_view>

class operations final : esp32::noncopyable
{
  public:
    void reboot();

    void factory_reset();

    static operations instance;

    bool get_reset_pending() const
    {
        return reset_pending.load();
    }

    static void mark_running_parition_as_valid();
    static bool try_mark_running_parition_as_invalid();

  private:
    operations();
    void reboot_timer_no_exception(uint8_t seconds);
    void reboot_timer(uint8_t seconds);
    static void operations_shutdown_handler();
    static void erase_all_nvs_partitions();

    std::atomic_bool reset_pending{false};
    std::unique_ptr<esp32::timer::timer> reboot_timer_;
};
