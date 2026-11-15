#ifndef PLATFORM_CACHED_H_
#define PLATFORM_CACHED_H_

// A function-local static without the guard barrier on every read: the first call makes it (thread-safe), later
// calls load a pointer whose address dependency orders the reads.

#include <atomic>

namespace pf
{

// make() once per lambda type; a reference to the result.
template< typename Make >
inline const auto& cachedStatic(Make make)
{
    using T = decltype(make());
    static std::atomic<const T*> s_value{nullptr};
    if (const T* value = s_value.load(std::memory_order_relaxed)) [[likely]]
        return *value;
    static const T s = make();
    s_value.store(&s, std::memory_order_release);
    return s;
}

}

#endif
