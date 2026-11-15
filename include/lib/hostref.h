#ifndef LIB_HOSTREF_H_
#define LIB_HOSTREF_H_

#include <x86.h>
#include <cstdint>
#if UINTPTR_MAX > 0xffffffffu
#include <platform/log.h>
#include <atomic>
#include <mutex>
#include <vector>
#endif

// A host object named by one 32-bit word of guest memory: the pointer itself on a 32-bit host, a slot of one
// process-wide table on a 64-bit host. 0 names null.
namespace win32 { namespace hostref
{

#if UINTPTR_MAX == 0xffffffffu

inline x86::reg32 add(void* object)
{
    return x86::reg32(reinterpret_cast<uintptr_t>(object));
}

inline void* get(x86::reg32 ref)
{
    return reinterpret_cast<void*>(uintptr_t(ref));
}

inline void remove(x86::reg32 ref)
{
    RC_USE(ref);
}

#else

namespace detail
{

constexpr x86::reg32 ChunkBits = 12;
constexpr x86::reg32 ChunkSlots = x86::reg32(1) << ChunkBits;
constexpr x86::reg32 ChunkCount = 256;

struct Table
{
    std::atomic<std::atomic<void*>*>    chunks[ChunkCount] = {};
    std::mutex                          lock;
    std::vector<x86::reg32>             freeSlots;
    x86::reg32                          next = 1;   // slot 0 is null
};

inline Table& table()
{
    static Table s_table;
    return s_table;
}

inline std::atomic<void*>* slot(Table& t, x86::reg32 ref)
{
    if ((ref >> ChunkBits) >= ChunkCount)
        return nullptr;
    std::atomic<void*>* chunk = t.chunks[ref >> ChunkBits].load(std::memory_order_acquire);
    return chunk ? &chunk[ref & (ChunkSlots - 1)] : nullptr;
}

}

inline x86::reg32 add(void* object)
{
    if (!object)
        return 0;
    detail::Table& t = detail::table();
    std::lock_guard<std::mutex> guard(t.lock);
    x86::reg32 ref;
    if (!t.freeSlots.empty())
    {
        ref = t.freeSlots.back();
        t.freeSlots.pop_back();
    }
    else
    {
        ref = t.next++;
        if ((ref >> detail::ChunkBits) >= detail::ChunkCount)
            pf::fatal("hostref: more than %u host objects named from guest memory", ref - 1);
        if (!t.chunks[ref >> detail::ChunkBits].load(std::memory_order_relaxed))
            t.chunks[ref >> detail::ChunkBits].store(new std::atomic<void*>[detail::ChunkSlots](),
                                                     std::memory_order_release);
    }
    detail::slot(t, ref)->store(object, std::memory_order_release);
    return ref;
}

// Null for 0 and for a slot that names nothing.
inline void* get(x86::reg32 ref)
{
    std::atomic<void*>* s = ref ? detail::slot(detail::table(), ref) : nullptr;
    return s ? s->load(std::memory_order_acquire) : nullptr;
}

inline void remove(x86::reg32 ref)
{
    detail::Table& t = detail::table();
    std::atomic<void*>* s = ref ? detail::slot(t, ref) : nullptr;
    if (!s)
        return;
    std::lock_guard<std::mutex> guard(t.lock);
    if (s->exchange(nullptr, std::memory_order_acq_rel))
        t.freeSlots.push_back(ref);
}

#endif

template< typename T >
inline T* get(x86::reg32 ref)
{
    return static_cast<T*>(get(ref));
}

// A 4-byte field of a host-constructed object in guest memory that names a host object (a plain pointer on a 32-bit
// host); the slot goes with the field.
template< typename T >
class Ref
{
public:
    Ref() = default;
    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;
    T* operator->() const       { return get(); }
    operator T*() const         { return get(); }

#if UINTPTR_MAX == 0xffffffffu
    Ref& operator=(T* object)
    {
        m_object = object;
        return *this;
    }
    T* get() const              { return m_object; }

private:
    T*          m_object = nullptr;
#else
    ~Ref() { remove(m_ref); }

    Ref& operator=(T* object)
    {
        remove(m_ref);
        m_ref = add(object);
        return *this;
    }
    T* get() const              { return hostref::get<T>(m_ref); }

private:
    x86::reg32  m_ref = 0;
#endif
};

}}

#endif
