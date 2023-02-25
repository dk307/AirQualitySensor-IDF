#pragma once

#include <sdmmc_cmd.h>

#include <string>

class sd_card
{
  public:
    void pre_begin();

    constexpr static char mount_point[] = "/sd";

    std::string get_info();

    static sd_card instance;

  private:
    sd_card() = default;
    sdmmc_card_t *sd_card_{nullptr};
};