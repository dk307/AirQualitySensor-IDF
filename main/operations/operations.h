#pragma once

#include <string>

class operations
{
  public:
    void reboot();
    void begin();

    void factory_reset();

    // // update
    // bool start_update(size_t length, const std::string &md5, std::string &error);
    // bool write_update(const uint8_t *data, size_t length, std::string &error);
    // bool end_update(std::string &error);
    // void abort_update();
    // bool is_update_in_progress();

    static operations instance;

  private:
    operations()
    {
    }
    // void get_update_error(std::string &error);
    [[noreturn]] static void reset();
};
