#include "util/filesystem/filesystem.h"
#include "util/filesystem/file_info.h"

#include <sys/stat.h>
#include <sys/utime.h>

namespace esp32::filesystem
{
bool create_directory(const std::filesystem::path &path)
{
    const file_info info{path};
    if (!info.exists())
    {
        // https://en.cppreference.com/w/cpp/filesystem/perms
        return mkdir(path.c_str(), S_IRWXU) == 0;
    }

    return false;
}

bool rename(const std::filesystem::path &old_path, const std::filesystem::path &new_path)
{
    return ::rename(old_path.c_str(), new_path.c_str()) == 0;
}

bool remove_directory(const std::filesystem::path &path)
{
    const file_info info{path};
    if (info.exists())
    {
        return rmdir(path.c_str()) == 0;
    }

    return false;
}

bool remove(const std::filesystem::path &path)
{
    const file_info info{path};
    if (info.exists())
    {
        return unlink(path.c_str()) == 0;
    }

    return false;
}

bool exists(const std::filesystem::path &path)
{
    const file_info fi{path};
    return fi.exists();
}

bool is_directory(const std::filesystem::path &path)
{
    const file_info fi{path};
    return fi.exists() && fi.is_directory();
}

bool set_last_modified_time(const std::filesystem::path &path, time_t modtime)
{
    struct utimbuf new_times;
    new_times.actime = modtime;
    new_times.modtime = new_times.actime;
    return utime(path.c_str(), &new_times) == 0;
}
} // namespace esp32::filesystem
