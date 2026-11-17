#ifndef LIB_MEMLOG_H
#define LIB_MEMLOG_H

#include <atomic>
#include <cstdint>

// Memory usage log (memory_log=on): one key=value line per memory_log_seconds and at start and world loads.

namespace win32 { namespace memlog {

extern std::atomic<bool> g_enabled;

// After the guest memory exists, with the game's frame counter (may be null). Logs the start line.
void configure(uint32_t (*frameCounter)());
// Any thread: logs one line now when enabled.
void event(const char* why);
void tick();

// Game thread, once per frame: one flag test when off.
inline void frame()
{
    if (__builtin_expect(g_enabled.load(std::memory_order_relaxed), 0))
        tick();
}

}}

#endif
