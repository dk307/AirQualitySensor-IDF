#pragma once

#include <sdmmc_cmd.h>
class sd_card
{
public:
    void pre_begin();

    constexpr static char mount_point[] = "/sd";
private:
    sdmmc_card_t *sd_card_{nullptr};
};