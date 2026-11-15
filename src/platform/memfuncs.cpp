#include <platform/smallcopy.h>

#include <psp2/kernel/clib.h>
#include <cstddef>

// The compiler's copy helpers and memcpy, memmove and memset (linked with --wrap): NEON up to pf::SmallCopyBytes
// (smallcopy.h), the system library above.
extern "C"
{

#ifdef NFSU_COPY_PROFILE
void pf_noteCopy(void* caller, size_t bytes);   // sampler.cpp
#define NOTE(count) pf_noteCopy(__builtin_return_address(0), count)
#else
#define NOTE(count)
#endif

namespace
{

inline void copy(void* destination, const void* source, size_t count)
{
    if (count <= pf::SmallCopyBytes)
        pf::copySmall(static_cast<uint8_t*>(destination), static_cast<const uint8_t*>(source), count);
    else
        sceClibMemcpy(destination, source, count);
}

inline void move(void* destination, const void* source, size_t count)
{
    if (count > pf::SmallCopyBytes
        || !pf::moveSmall(static_cast<uint8_t*>(destination), static_cast<const uint8_t*>(source), count))
        sceClibMemmove(destination, source, count);
}

inline void fill(void* destination, int value, size_t count)
{
    if (count <= pf::SmallCopyBytes)
        pf::fillSmall(static_cast<uint8_t*>(destination), uint8_t(value), count);
    else
        sceClibMemset(destination, value, count);
}

}

void __aeabi_memcpy(void* destination, const void* source, size_t count)
{
    NOTE(count);
    copy(destination, source, count);
}

void __aeabi_memcpy4(void* destination, const void* source, size_t count)
{
    NOTE(count);
    copy(destination, source, count);
}

void __aeabi_memcpy8(void* destination, const void* source, size_t count)
{
    NOTE(count);
    copy(destination, source, count);
}

void __aeabi_memmove(void* destination, const void* source, size_t count)
{
    NOTE(count);
    move(destination, source, count);
}

void __aeabi_memmove4(void* destination, const void* source, size_t count)
{
    NOTE(count);
    move(destination, source, count);
}

void __aeabi_memmove8(void* destination, const void* source, size_t count)
{
    NOTE(count);
    move(destination, source, count);
}

// The helpers take the count before the fill value.
void __aeabi_memset(void* destination, size_t count, int value)
{
    NOTE(count);
    fill(destination, value, count);
}

void __aeabi_memset4(void* destination, size_t count, int value)
{
    NOTE(count);
    fill(destination, value, count);
}

void __aeabi_memset8(void* destination, size_t count, int value)
{
    NOTE(count);
    fill(destination, value, count);
}

void __aeabi_memclr(void* destination, size_t count)
{
    NOTE(count);
    fill(destination, 0, count);
}

void __aeabi_memclr4(void* destination, size_t count)
{
    NOTE(count);
    fill(destination, 0, count);
}

void __aeabi_memclr8(void* destination, size_t count)
{
    NOTE(count);
    fill(destination, 0, count);
}

void* __wrap_memcpy(void* destination, const void* source, size_t count)
{
    NOTE(count);
    copy(destination, source, count);
    return destination;
}

void* __wrap_memmove(void* destination, const void* source, size_t count)
{
    NOTE(count);
    move(destination, source, count);
    return destination;
}

void* __wrap_memset(void* destination, int value, size_t count)
{
    NOTE(count);
    fill(destination, value, count);
    return destination;
}

}
