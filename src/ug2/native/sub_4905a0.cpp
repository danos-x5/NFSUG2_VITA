// Native replacement of sub_4905a0: resolves unset entry handles through sub_4901d0, then links matching table rows (thiscall, no stack arguments).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_4905a0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_4901d0(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;

// Calls sub_4901d0 for one entry; false when terminating.
[[gnu::noinline]] bool resolve(win32::WinApplication* app, x86::CPU& cpu, reg32 e, reg32 left, reg32 total, reg32 obj,
                               reg32 entry)
{
    const reg32 savedEbx = cpu.ebx, savedEbp = cpu.ebp, savedEsi = cpu.esi, savedEdi = cpu.edi;
    const reg32 arg = rc::ld32(e + 4);
    cpu.ebx = left;
    cpu.esi = e + 8;
    cpu.edi = total;
    cpu.ebp = obj;
    cpu.ecx = arg;
    rc::st32(entry - 28, 0);
    rc::st32(entry - 32, 1);
    rc::st32(entry - 36, arg);
    cpu.esp = entry - 40;
    sub_4901d0(app, cpu);
    if (cpu.terminate)
        return false;
    rc::st32(e + 8, cpu.eax);
    cpu.ebx = savedEbx;
    cpu.ebp = savedEbp;
    cpu.esi = savedEsi;
    cpu.edi = savedEdi;
    return true;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("entry_resolve_native", true); }))
    {
        sub_4905a0_guest(app, cpu);
        return;
    }
    const reg32 self = cpu.ecx;
    const reg32 array = rc::ld32(self + 0x10);
    const reg32 total = rc::ld32(self + 0x14);
    if (array == 0 || int32_t(total) <= 0)
        return;
    const reg32 obj = rc::ld32(self + 0xc);
    const int32_t count = int32_t(int8_t(rc::ld8(obj + 0x19)));
    const reg32 entry = cpu.esp;
    for (reg32 i = 0; i < total; ++i)
    {
        const reg32 e = array + 16 * i;
        if (rc::ld32(e + 8) != 0xffffffffu)
            continue;
        if (!resolve(app, cpu, e, total - i, total, obj, entry))
            return;
    }
    for (int32_t s = 0; s < count; ++s)
    {
        const reg32 pair = rc::ld32(obj + 0x2c) + 8 * reg32(s);
        const reg32 key = rc::ld32(pair);
        for (reg32 e = array, j = total; j != 0; e += 16, --j)
        {
            if (rc::ld32(e + 4) != 0 && rc::ld32(e) == key)
            {
                const reg32 old = rc::ld32(pair + 4);
                rc::st32(pair + 4, rc::ld32(e + 8));
                rc::st32(e + 0xc, rc::ld32(obj + 0x2c) + 8 * reg32(s) + 4);
                rc::st32(e + 8, old);
                break;
            }
        }
        if (s + 1 < count)
            app->allowPreemption(cpu);
    }
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const reg32 obj = rc::ld32(cpu.ecx + 0xc);
    const reg32 array = rc::ld32(cpu.ecx + 0x10);
    const int32_t total = int32_t(rc::ld32(cpu.ecx + 0x14));
    const int32_t count = int32_t(int8_t(rc::ld8(obj + 0x19)));
    if (array == 0 || total <= 0)
        return;
    const int32_t entries = total < 4096 ? total : 4096;
    for (int32_t i = 0; i < entries; ++i)
        spans.push_back({array + 16 * reg32(i) + 8, 8, false});
    if (count <= 0)
        return;
    const reg32 table = rc::ld32(obj + 0x2c);
    for (int32_t s = 0; s < count; ++s)
        spans.push_back({table + 8 * reg32(s) + 4, 4, false});
}

const shadow::Spec spec = {"sub_4905a0", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi | shadow::Esp, 0, false,
                           false};

}

void sub_4905a0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_4905a0_guest, declare);
}

}}
