#pragma once

#include "util/noncopyable.h"

#include <atomic>

class operations : esp32::noncopyable
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
    [[noreturn]] void reset();

    static void shutdown_restarting();
    std::atomic_bool reset_pending{false};
};
