#include <mbedtls/md.h>

#include "util/noncopyable.h"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace esp32::hash
{
template <mbedtls_md_type_t hashType> class hash : esp32::noncopyable
{
  public:
    hash()
    {
        mbedtls_md_init(&ctx_);
        auto ret = mbedtls_md_setup(&ctx_, mbedtls_md_info_from_type(hashType), 0);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to set up hash algorithm");
        }
        ret = mbedtls_md_starts(&ctx_);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to start hash calculation");
        }
    }

    ~hash()
    {
        mbedtls_md_free(&ctx_);
    }

    void update(const std::string &input_string)
    {
        update(reinterpret_cast<const uint8_t *>(input_string.c_str()), input_string.length());
    }

    void update(const std::vector<uint8_t> &input_data)
    {
        update(input_data.data(), input_data.size());
    }

    void update(const std::span<uint8_t> &input_data)
    {
        update(input_data.data(), input_data.size());
    }

    void update(const uint8_t *input_data, size_t input_size)
    {
        int ret = mbedtls_md_update(&ctx_, input_data, input_size);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to update hash calculation");
        }
    }

    void update(const void *input_data, size_t input_size)
    {
        update(static_cast<const uint8_t *>(input_data), input_size);
    }

    std::vector<uint8_t> finish()
    {
        std::vector<uint8_t> hash_result(mbedtls_md_get_size(mbedtls_md_info_from_type(hashType)));
        int ret = mbedtls_md_finish(&ctx_, hash_result.data());
        if (ret != 0)
        {
            throw std::runtime_error("Failed to finalize hash calculation");
        }
        return hash_result;
    }

  private:
    mbedtls_md_context_t ctx_;
};

template <class... Args> auto sha256(Args... data)
{
    esp32::hash::hash<MBEDTLS_MD_SHA256> hasher;
    hasher.update(std::forward<Args>(data)...);
    return hasher.finish();
}

} // namespace esp32::hash
