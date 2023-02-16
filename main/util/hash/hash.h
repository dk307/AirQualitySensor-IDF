#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>

namespace esp32
{
    namespace hash
    {
        template <mbedtls_md_type_t typeT, size_t hashLenT>
        auto hash(const uint8_t *data, std::size_t len)
        {
            std::array<uint8_t, hashLenT> result;
            mbedtls_md_context_t ctx;

            mbedtls_md_init(&ctx);
            mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(typeT), 0);
            mbedtls_md_starts(&ctx);
            mbedtls_md_update(&ctx, data, len);
            mbedtls_md_finish(&ctx, result.data());
            mbedtls_md_free(&ctx);

            return result;
        }

        auto md5(const uint8_t *data, std::size_t len)
        {
            return hash<MBEDTLS_MD_MD5, 16>(data, len);
        }

        auto md5(const std::string& data)
        {
            return md5(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        }
    }
}
