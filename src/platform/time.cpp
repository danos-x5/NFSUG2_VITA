#include <platform/time.h>
#include <platform/perf.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <atomic>

namespace pf
{

namespace
{

std::atomic<uint64_t> s_base{0};    // the last wide read; ticksUs adds the cheaper low word to it

}

uint64_t ticksUs()
{
    perf::count(perf::CountClock);
    const uint32_t low = sceKernelGetProcessTimeLow();
    const uint64_t base = s_base.load(std::memory_order_relaxed);
    const uint32_t since = low - uint32_t(base);
    if (base != 0 && since < 0x80000000u)
        return base + since;
    const uint64_t now = sceKernelGetProcessTimeWide();
    s_base.store(now, std::memory_order_relaxed);
    return now;
}

uint32_t ticksUsLow()
{
    perf::count(perf::CountClock);
    return sceKernelGetProcessTimeLow();
}

uint32_t ticksMs()
{
    return uint32_t(ticksUs() / 1000);
}

void sleepMs(uint32_t milliseconds)
{
    // Sleep(0) must still let other guest threads take the execution lock.
    sleepUs(milliseconds ? milliseconds * 1000 : 50);
}

void sleepUs(uint32_t microseconds)
{
    sceKernelDelayThread(microseconds);
}

}
