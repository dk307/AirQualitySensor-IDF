#include <cstdio>


namespace esp32::filesystem
{
    class file
    {
    public:
        file(const char *filename, const char *mode)
        {
            file_ = std::fopen(filename, mode);
            if (file_ == nullptr)
            {
                throw std::runtime_error("Failed to open file");
            }
        }

        ~file()
        {
            if (file_ != nullptr)
            {
                std::fclose(file_);
            }
        }

        size_t read(void *buffer, size_t size, size_t count)
        {
            return std::fread(buffer, size, count, file_);
        }

        size_t write(const void *buffer, size_t size, size_t count)
        {
            return std::fwrite(buffer, size, count, file_);
        }

        int seek(long offset, int origin)
        {
            return std::fseek(file_, offset, origin);
        }

        long tell()
        {
            return std::ftell(file_);
        }

        int flush()
        {
            return std::fflush(file_);
        }

        int eof()
        {
            return std::feof(file_);
        }

    private:
        FILE *file_;
    };
}
