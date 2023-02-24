#pragma once

#include <esp_ota_ops.h>
#include <esp_partition.h>

#include <array>

#include "util/exceptions.h"

namespace esp32
{
class ota_updator
{
  public:
    ota_updator(const std::array<uint8_t, 32> &expected_sha256);
    void write(const uint8_t *data, size_t size);
    void end();
    void abort();
    bool is_running();

  private:
    esp_ota_handle_t handle_{0};
    const std::array<uint8_t, 32> expected_sha256_;
    const esp_partition_t *update_partition_{nullptr};
};

class ota_exception : public esp_exception
{
  public:
    using esp_exception::esp_exception;
};

} // namespace esp32
