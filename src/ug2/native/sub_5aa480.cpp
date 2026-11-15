// Native sub_5aa480: mixer accumulate over the intrusive node list (thiscall, ret 4: scale).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5aa480_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5a0d20(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// Link pointers address node+4; link 0 maps to node 0.
inline x86::reg32 nodeOf(x86::reg32 link) { return link ? link - 4 : 0; }

// One element: dst = scale * src + dst, with scale re-read from memory.
void accumulate(x86::reg32 scale, x86::reg32 dst, x86::reg32 src)
{
    Real v = ld(scale);
    v *= ld(src);
    v += ld(dst);
    rc::stf32(dst, float(v));
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("mixer_accumulate_native", true); }))
    {
        sub_5aa480_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 savedEbx = cpu.ebx, savedEbp = cpu.ebp, savedEsi = cpu.esi, savedEdi = cpu.edi;
    const x86::reg32 frame = entry - 0x248;
    const x86::reg32 idxSlot = entry - 0x238;
    const x86::reg32 thisSlot = entry - 0x234;
    const x86::reg32 buffer = entry - 0x230;
    const x86::reg32 scale = entry + 4;

    const x86::reg32 self = cpu.ecx;
    rc::st32(entry - 0x23c, savedEbx); rc::st32(entry - 0x240, savedEbp); rc::st32(entry - 0x244, savedEsi); rc::st32(entry - 0x248, savedEdi);
    rc::st32(thisSlot, self);
    rc::st32(entry - 0x24c, buffer);
    cpu.esi = self;
    cpu.esp = entry - 0x250;
    sub_5a0d20(app, cpu);
    if (cpu.terminate)
        return;

    x86::reg32 ebp = cpu.ebp;
    x86::reg32 esi = cpu.esi;
    x86::reg32 edi = cpu.edi;
    x86::reg32 ebx = rc::ld32(esi + 0xc);
    rc::st32(idxSlot, 0);

    for (;;)
    {
        ebx = nodeOf(ebx);
        if (ebx == nodeOf(esi + 0xc))
            break;

        cpu.ecx = ebx;
        cpu.ebx = ebx;
        cpu.esi = esi;
        cpu.edi = edi;
        cpu.ebp = ebp;
        cpu.esp = frame;
        ::native::callGuest(app, cpu, rc::ld32(rc::ld32(ebx) + 4));
        if (cpu.terminate)
            return;
        ebx = cpu.ebx;
        cpu.ebp = cpu.eax;
        cpu.ecx = ebx;
        cpu.esp = frame;
        ::native::callGuest(app, cpu, rc::ld32(rc::ld32(ebx) + 0xc));
        if (cpu.terminate)
            return;
        ebx = cpu.ebx;
        cpu.esi = cpu.eax;
        cpu.edi = buffer + rc::ld32(idxSlot) * 4;
        cpu.ecx = ebx;
        cpu.esp = frame;
        ::native::callGuest(app, cpu, rc::ld32(rc::ld32(ebx) + 0x20));
        if (cpu.terminate)
            return;
        ebx = cpu.ebx;
        esi = cpu.esi;
        edi = cpu.edi;
        ebp = cpu.ebp;
        cpu.esp = frame;

        const int32_t count = int32_t(ebp);
        if (count > 0)
        {
            const x86::reg32 blocks = x86::reg32(count) >> 2;
            const x86::reg32 tail = x86::reg32(count) & 3;
            const x86::reg32 bytes = x86::reg32(count) * 4;
            float* d = reinterpret_cast<float*>(::native::host(esi));
            const float* s = reinterpret_cast<const float*>(::native::host(edi));
            const bool fast = x86::reg32(count) < 0x10000000u && scale - esi + 3 >= bytes + 3 &&
                              esi <= 0u - bytes && edi <= 0u - bytes &&
                              ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(s)) & 3) == 0;
            if (fast)
            {
                // No store reaches the scale slot; reload it only after a preemption point.
                Real k = ld(scale);
                const auto step = [&] {
                    Real v = k;
                    v *= Real(*s++);
                    v += Real(*d);
                    *d++ = float(v);
                };
                for (x86::reg32 b = blocks; b != 0;)
                {
                    step(); step(); step(); step();
                    if (--b != 0)
                    {
                        app->allowPreemption(cpu);
                        k = ld(scale);
                    }
                }
                for (x86::reg32 t = tail; t != 0;)
                {
                    step();
                    if (--t != 0)
                    {
                        app->allowPreemption(cpu);
                        k = ld(scale);
                    }
                }
            }
            else
            {
                x86::reg32 dst = esi, src = edi;
                for (x86::reg32 b = blocks; b != 0;)
                {
                    for (x86::reg32 e = 0; e < 16; e += 4)
                        accumulate(scale, dst + e, src + e);
                    dst += 16;
                    src += 16;
                    if (--b != 0)
                        app->allowPreemption(cpu);
                }
                for (x86::reg32 t = tail; t != 0;)
                {
                    accumulate(scale, dst, src);
                    dst += 4;
                    src += 4;
                    if (--t != 0)
                        app->allowPreemption(cpu);
                }
            }
            edi += bytes;
        }

        ebx = rc::ld32(ebx + 4);
        esi = rc::ld32(thisSlot);
        rc::st32(idxSlot, rc::ld32(idxSlot) + ebp);
        app->allowPreemption(cpu);
    }

    cpu.ecx = esi;
    cpu.ebx = ebx;
    cpu.esi = esi;
    cpu.edi = edi;
    cpu.ebp = ebp;
    cpu.esp = frame;
    ::native::callGuest(app, cpu, rc::ld32(rc::ld32(esi) + 4));
    if (cpu.terminate)
        return;

    cpu.edi = rc::ld32(entry - 0x248); cpu.esi = rc::ld32(entry - 0x244); cpu.ebp = rc::ld32(entry - 0x240); cpu.ebx = rc::ld32(entry - 0x23c);
    cpu.esp = entry + 8;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    (void)spans;
}

const shadow::Spec spec = {"sub_5aa480", shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};

}

void sub_5aa480(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5aa480_guest, declare);
}

}}
