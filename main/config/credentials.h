#pragma once

#include <string>

class credentials
{
  public:
    credentials() = default;
    credentials(const std::string &user_name, const std::string &password) : user_name_(user_name), password_(password)
    {
    }

    const std::string &get_password() const
    {
        return password_;
    }

    const std::string &get_user_name() const
    {
        return user_name_;
    }

    bool empty() const
    {
        return user_name_.empty();
    }

  private:
    std::string user_name_;
    std::string password_;
};
