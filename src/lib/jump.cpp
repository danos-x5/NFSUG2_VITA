#include <recomp.h>
#include <cpu.h>
#include <platform/log.h>
#include <platform/sync.h>
#include <atomic>
#include <unordered_map>

namespace rc
{

namespace
{

struct JumpTarget
{
    jmp_buf     buffer;
    x86::reg32  fpuCount;   // x87 depth at the setjmp call
};

// Guest jmp_buf address -> host target. Elements keep their address when the map grows.
std::unordered_map<x86::reg32, JumpTarget> s_targets;
pf::LwMutex s_targetsMutex("setjmp");

}

jmp_buf& jumpBuffer(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 guestBuffer)
{
    RC_USE(app);
    pf::LwLockGuard guard(s_targetsMutex);
    JumpTarget& target = s_targets[guestBuffer];
    target.fpuCount = cpu.fpu.count;
    return target.buffer;
}

void longJump(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 guestBuffer)
{
    RC_USE(app);
    JumpTarget* target = nullptr;
    {
        pf::LwLockGuard guard(s_targetsMutex);
        const auto it = s_targets.find(guestBuffer);
        if (it != s_targets.end())
            target = &it->second;
    }
    if (!target)
        x86::guestTrap(guestBuffer, "longjmp to a jmp_buf no setjmp call filled");
    cpu.fpu.count = target->fpuCount;
    longjmp(target->buffer, 1);
}

x86::reg32 g_promoteLo = 0;
x86::reg32 g_promoteSpan = 0xffffffffu;

void heapInFrame(x86::reg32 site, x86::reg32 address, x86::reg32 esp, x86::reg32 limit)
{
    static std::atomic<int> s_count{0};
    const int n = ++s_count;
    if (n <= 200 || (n & (n - 1)) == 0)
        pf::logError("alias check: 0x%08x accesses [0x%08x], inside its separate stack bytes [0x%08x, 0x%08x) (%d so far)",
                     site, address, esp, limit, n);
}

void slotMismatch(x86::reg32 site, x86::reg32 address, uint64_t memory, uint64_t local, unsigned bytes)
{
    static std::atomic<int> s_logged{0};
    static std::atomic<int> s_count{0};
    const int n = ++s_count;
    if (s_logged.fetch_add(1) < 200 || (n & (n - 1)) == 0)
        pf::logError("slot check: 0x%08x [0x%08x] %u bytes: memory 0x%llx, local 0x%llx (%d so far)", site, address,
                     bytes, (unsigned long long)memory, (unsigned long long)local, n);
}

}
