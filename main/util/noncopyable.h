#pragma once

namespace esp32
{
class noncopyable
{
  protected:
    noncopyable() = default;
    ~noncopyable() = default;

  private:
    explicit noncopyable(const noncopyable &) = delete;
    const noncopyable &operator=(const noncopyable &) = delete;
};
} // namespace esp32