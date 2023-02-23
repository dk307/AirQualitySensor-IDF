
#include <filesystem>

namespace esp32::filesystem
{
// filesystem equivalent do not work properly
bool create_directory(const std::filesystem::path &path);
bool remove_directory(const std::filesystem::path &path);
bool rename(const std::filesystem::path &old_path, const std::filesystem::path &new_path);
bool remove(const std::filesystem::path &path);
bool exists(const std::filesystem::path &path);
bool is_directory(const std::filesystem::path &path);
bool set_last_modified_time(const std::filesystem::path &path, time_t modtime);
} // namespace esp32::filesystem