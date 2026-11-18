// Runtime native-versus-guest check: both bodies run from one entry state; the guest outcome is kept.

#include "check.h"

#include <platform/log.h>

#include <cstdio>
#include <string>
#include <vector>

namespace ug2 { namespace native { namespace check {

namespace {

const uint32_t DetailLimit = 5;
const uint32_t ReportEvery = 50000;

Stats* s_table[32];
std::atomic<uint32_t> s_count{0};
thread_local bool t_active = false;

typedef std::vector<std::vector<uint8_t>> Memory;

void capture(const shadow::Spans& spans, Memory& out)
{
    out.resize(spans.size());
    for (size_t i = 0; i < spans.size(); ++i)
        out[i].assign(::native::host(spans[i].address), ::native::host(spans[i].address) + spans[i].size);
}

void restore(const shadow::Spans& spans, const Memory& in)
{
    for (size_t i = 0; i < spans.size(); ++i)
        memcpy(::native::host(spans[i].address), in[i].data(), spans[i].size);
}

void enrol(Stats& stats)
{
    static std::atomic_flag lock = ATOMIC_FLAG_INIT;
    while (lock.test_and_set(std::memory_order_acquire)) {}
    const uint32_t n = s_count.load(std::memory_order_relaxed);
    bool known = false;
    for (uint32_t i = 0; i < n; ++i)
        known = known || s_table[i] == &stats;
    if (!known && n < 32)
    {
        s_table[n] = &stats;
        s_count.store(n + 1, std::memory_order_release);
    }
    lock.clear(std::memory_order_release);
}

uint32_t differences(x86::reg8 registers, const shadow::Spans& spans, const x86::CPU& a, const Memory& ma,
                     const x86::CPU& b, const Memory& mb, std::string& text)
{
    static const char* const names[8] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
    const x86::reg32 ra[8] = {a.eax, a.ecx, a.edx, a.ebx, a.esp, a.ebp, a.esi, a.edi};
    const x86::reg32 rb[8] = {b.eax, b.ecx, b.edx, b.ebx, b.esp, b.ebp, b.esi, b.edi};
    uint32_t count = 0;
    char line[160];
    for (int i = 0; i < 8; ++i)
        if ((registers >> i & 1) && ra[i] != rb[i])
        {
            snprintf(line, sizeof(line), "  %s native %08x guest %08x\n", names[i], ra[i], rb[i]);
            text += line;
            ++count;
        }
    if (a.fpu.count != b.fpu.count)
    {
        snprintf(line, sizeof(line), "  x87 depth native %u guest %u\n", unsigned(a.fpu.count), unsigned(b.fpu.count));
        text += line;
        ++count;
    }
    else if (a.fpu.count > 0)
    {
        const float x = float(a.fpu.regs[a.fpu.count & 7]), y = float(b.fpu.regs[b.fpu.count & 7]);
        if (memcmp(&x, &y, 4) != 0)
        {
            snprintf(line, sizeof(line), "  st0 native %.9g guest %.9g\n", double(x), double(y));
            text += line;
            ++count;
        }
    }
    for (size_t s = 0; s < spans.size(); ++s)
        if (memcmp(ma[s].data(), mb[s].data(), spans[s].size) != 0)
        {
            uint32_t first = 0;
            while (ma[s][first] == mb[s][first])
                ++first;
            snprintf(line, sizeof(line), "  span %u [%08x+%u] first difference at +%u: native %02x guest %02x\n",
                     unsigned(s), spans[s].address, unsigned(spans[s].size), unsigned(first), ma[s][first],
                     mb[s][first]);
            text += line;
            ++count;
        }
    return count;
}

}

void compare(Stats& stats, x86::reg8 registers, win32::WinApplication* app, x86::CPU& cpu, shadow::Body native,
             shadow::Body guest, shadow::Declare declare)
{
    if (t_active)
    {
        guest(app, cpu);
        return;
    }
    t_active = true;
    if (stats.compared.load(std::memory_order_relaxed) == 0)
        enrol(stats);
    shadow::Spans spans;
    declare(cpu, spans);
    const x86::CPU entry = cpu;
    Memory before, nativeMemory, guestMemory;
    capture(spans, before);
    native(app, cpu);
    const x86::CPU nativeCpu = cpu;
    capture(spans, nativeMemory);
    cpu = entry;
    restore(spans, before);
    guest(app, cpu);
    capture(spans, guestMemory);
    t_active = false;
    if (cpu.terminate)
        return;
    std::string text;
    const uint32_t count = differences(registers, spans, nativeCpu, nativeMemory, cpu, guestMemory, text);
    const uint32_t compared = stats.compared.fetch_add(1, std::memory_order_relaxed) + 1;
    if (count)
    {
        stats.mismatches.fetch_add(1, std::memory_order_relaxed);
        if (stats.details.fetch_add(1, std::memory_order_relaxed) < DetailLimit)
            pf::logInfo("check %s: mismatch at call %u (entry ecx %08x esp %08x)\n%s", stats.name, unsigned(compared),
                        entry.ecx, entry.esp, text.c_str());
    }
    if (compared % ReportEvery == 0)
        pf::logInfo("check %s: %u compared, %u mismatches", stats.name, unsigned(compared),
                    unsigned(stats.mismatches.load()));
}

void report()
{
    const uint32_t n = s_count.load(std::memory_order_acquire);
    for (uint32_t i = 0; i < n; ++i)
        pf::logInfo("check %s: total %u compared, %u mismatches", s_table[i]->name,
                    unsigned(s_table[i]->compared.load()), unsigned(s_table[i]->mismatches.load()));
}

}}}
