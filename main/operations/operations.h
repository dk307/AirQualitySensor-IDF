#pragma once

#include <atomic>

class operations
{
  public:
    void reboot();

    void factory_reset();

    static operations instance;

    bool get_reset_pending() const
    {
        return reset_pending.load();
    }

  private:
    operations(); 
    [[noreturn]] void reset();

    static void shutdown_restarting();
    std::atomic_bool reset_pending{false};
};
