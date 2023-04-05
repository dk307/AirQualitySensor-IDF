#pragma once

#include <cstdio>
#include <stdexcept>

namespace esp32::filesystem
{
class file : esp32::noncopyable
{
  public:
    file(const char *filename, const char *mode)
    {
        file_ = fopen(filename, mode);
        if (file_ == nullptr)
        {
            throw std::runtime_error("Failed to open file");
        }
    }

    ~file()
    {
        if (file_ != nullptr)
        {
            fclose(file_);
        }
    }

    size_t read(void *buffer, size_t size, size_t count)
    {
        return fread(buffer, size, count, file_);
    }

    size_t write(const void *buffer, size_t size, size_t count)
    {
        return fwrite(buffer, size, count, file_);
    }

    int seek(long offset, int origin)
    {
        return fseek(file_, offset, origin);
    }

    long tell()
    {
        return ftell(file_);
    }

    int flush()
    {
        return fflush(file_);
    }

    int eof()
    {
        return feof(file_);
    }

  private:
    FILE *file_{nullptr};
};
} // namespace esp32::filesystem
