#ifndef PLATFORM_SMALLCOPY_H_
#define PLATFORM_SMALLCOPY_H_

// Copies and fills of a few hundred bytes with NEON (any alignment), for memfuncs.cpp: the system library's
// sceClibMemcpy takes about 190 ns before it copies anything.

#include <arm_neon.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pf
{

const size_t SmallCopyBytes = 1024;    // larger copies and fills go to the system library

// Forward copy; also right when the destination is below the source.
inline void copySmall(uint8_t* d, const uint8_t* s, size_t n)
{
    for (; n >= 32; n -= 32, s += 32, d += 32)
    {
        const uint8x16_t a = vld1q_u8(s), b = vld1q_u8(s + 16);
        vst1q_u8(d, a);
        vst1q_u8(d + 16, b);
    }
    if (n & 16)
    {
        vst1q_u8(d, vld1q_u8(s));
        s += 16;
        d += 16;
    }
    if (n & 8)
    {
        vst1_u8(d, vld1_u8(s));
        s += 8;
        d += 8;
    }
    if (n & 4)
    {
        uint32_t w;
        __builtin_memcpy(&w, s, 4);
        __builtin_memcpy(d, &w, 4);
        s += 4;
        d += 4;
    }
    if (n & 2)
    {
        uint16_t h;
        __builtin_memcpy(&h, s, 2);
        __builtin_memcpy(d, &h, 2);
        s += 2;
        d += 2;
    }
    if (n & 1)
        *d = *s;
}

// Fills n bytes with value.
inline void fillSmall(uint8_t* d, uint8_t value, size_t n)
{
    const uint8x16_t v = vdupq_n_u8(value);
    for (; n >= 32; n -= 32, d += 32)
    {
        vst1q_u8(d, v);
        vst1q_u8(d + 16, v);
    }
    if (n & 16)
    {
        vst1q_u8(d, v);
        d += 16;
    }
    if (n & 8)
    {
        vst1_u8(d, vget_low_u8(v));
        d += 8;
    }
    const uint32_t w = value * 0x01010101u;
    if (n & 4)
    {
        __builtin_memcpy(d, &w, 4);
        d += 4;
    }
    if (n & 2)
    {
        __builtin_memcpy(d, &w, 2);
        d += 2;
    }
    if (n & 1)
        *d = value;
}

// memmove; false when the destination starts inside the source.
inline bool moveSmall(uint8_t* d, const uint8_t* s, size_t n)
{
    if (size_t(d - s) < n)
        return false;
    copySmall(d, s, n);
    return true;
}

}

#endif
