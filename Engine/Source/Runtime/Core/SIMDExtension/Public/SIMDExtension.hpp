#pragma once
#include <immintrin.h>
#include <string>

namespace Engine
{
    struct SIMDExtension
    {
        inline static bool check_avx()
        {
            // todo: less platform specific way
            constexpr size_t minimum_avx_requirements = 6; // == std::_Stl_isa_available_avx2
            static bool use_avx = std::__isa_available >= minimum_avx_requirements;
            return use_avx;
        }

        static void _mm256_memcpy(void* dst, const void* src, const size_t size)
        {
            if (check_avx())
            {
                _mm256_memcpy_Impl(dst, src, size);
            }
            else
            {
                std::memcpy(dst, src, size);
            }
        }
    private:
        static void _mm256_memcpy_Impl(void* dst, const void* src, const size_t size)
        {
            // 32 bytes size block copy
            const size_t count = size / sizeof(__m256);
            // remaining bytes if size is not multiple of 32
            const size_t remain = size % sizeof(__m256);

            const auto p_dst = static_cast<__m256i*>(dst);
            const auto p_src = static_cast<const __m256i*>(src);

            for (size_t i = 0; i < count; ++i)
            {
                _mm256_store_si256(p_dst + i, *(p_src + i));
            }

            // If remaining bytes exist, fallback to default memcpy
            if (remain)
            {
                std::memcpy(p_dst + count, p_src + count, remain);
            }
        }
    };
}