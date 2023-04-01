#pragma once

#include "util/singleton.h"
#include <sdmmc_cmd.h>
#include <string>

class sd_card final : public esp32::singleton<sd_card>
{
  public:
    void begin();

    constexpr static char mount_point[] = "/sd";

    std::string get_info();

    bool is_mounted() const
    {
        return sd_card_ != nullptr;
    }

  private:
    sd_card() = default;
    ~sd_card() = default;

    friend class esp32::singleton<sd_card>;

    sdmmc_card_t *sd_card_{nullptr};
};