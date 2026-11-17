// Native sub_4198c0: picks a list entry by distance and height, then walks neighbours by threshold. Keeps the guest frame; _ftol2 is inlined, other callees run as guest code.

#include <lib/guest.h>
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

#include <cstring>

namespace ug2
{

void sub_4198c0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_4122c0(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;

reg32 entryParam(reg32 el)
{
    const uint16_t f5c = rc::ld16(el + 0x5c);
    if (f5c == 0xffff)
        return 0;
    const uint16_t f5e = rc::ld16(el + 0x5e);
    if (f5e == 0xffff)
        return 0;
    const int32_t a = int16_t(f5c);
    const reg32 base = a >= 0 ? rc::ld32(0x88a334u + reg32(a) * 4) : 0;
    return reg32(int32_t(int16_t(f5e)) * 0x38) + base + 0x8c;
}

// native::sub_75bed4 (_ftol2) outside the int32 range.
__attribute__((noinline, cold)) reg32 wideTrunc(Real v)
{
    return reg32(x86::truncInteger64(double(v)));
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("entry_pick_native"))
    {
        sub_4198c0_guest(app, cpu);
        return;
    }
    const reg32 E = cpu.esp;
    const reg32 L = cpu.ecx;
    const reg32 A = rc::ld32(E + 4);
    const reg32 arg2 = rc::ld32(E + 12);
    const reg32 start = rc::ld32(E + 16);
    const reg32 savedEbx = cpu.ebx, savedEbp = cpu.ebp, savedEsi = cpu.esi, savedEdi = cpu.edi;
    const reg32 fc = cpu.fpu.count;
    const reg32 S = ((E - 4) & 0xfffffff0u) - 0x24 - 12;
    const reg32 frameEbp = E - 4;

    reg32 ebx = 0xffffffffu;
    reg32 i = int32_t(start) > 0 ? start : 0;
    x86::Float M = 0;
    Real lastV = cpu.fpu.regs[(fc + 1) & 7];
    const Real hScale = ld(0x784278), kScore = ld(0x783cc4), kStep = ld(0x7852ac);
    const float C1 = float(ld(0x7d7d3c)), C2 = float(ld(0x784360));
    while (int32_t(i) < int32_t(rc::ld32(L + 4)))
    {
        const reg32 el = rc::ld32(L + 8 + i * 4);
        rc::st32(S - 4, arg2);
        rc::st32(S - 8, 0);
        rc::stf32(S - 12, float(ld(el + 0x54)));
        rc::st32(S - 16, S + 0x20);
        cpu.ecx = el;
        cpu.ebx = ebx;
        cpu.esi = el;
        cpu.edi = i;
        cpu.ebp = frameEbp;
        cpu.edx = L;
        cpu.esp = S - 20;
        cpu.fpu.count = fc;
        sub_4122c0(app, cpu);
        if (cpu.terminate)
            return;
        const Real out1 = ld(S + 0x24);
        Real dx = ld(A);
        dx -= ld(S + 0x20);
        Real dy = ld(A + 4);
        dy -= out1;
        Real sx = dx;
        sx *= dx;
        Real sy = dy;
        sy *= dy;
        sx += sy;
        const float D = float(cpu.fpu.sqrt(sx));
        const reg32 P = entryParam(el);
        const int32_t h = int16_t(rc::ld16(P + 0x12));
        const int32_t iv = int32_t(reg32(h) << 13);
        Real hh = Real(iv);
        hh *= hScale;
        hh -= ld(A + 8);
        hh = cpu.fpu.abs(hh);
        const float H = float(hh);
        Real X;
        if (!(D > C1))
            X = C1;
        else
        {
            X = D;
            if (C2 < D)
                X = C2;
        }
        Real f2 = kScore;
        f2 *= Real(H);
        f2 += X;
        if (f2 < Real(M) || ebx == 0xffffffffu)
        {
            M = f2;
            ebx = i;
        }
        Real v = X;
        v *= kStep;
        const reg32 r = __builtin_expect((bits(v) & 0x7fffffffu) < 0x4f000000u, 1) ? reg32(int32_t(v)) : wideTrunc(v);
        lastV = v;
        const int32_t t = int32_t(reg32(-2) - r);
        const reg32 step = t <= 0 ? 0 : reg32(t);
        i = i + step + 1;
        if (int32_t(i) < int32_t(rc::ld32(L + 4)))
            app->allowPreemption(cpu);
    }
    cpu.fpu.regs[(fc + 1) & 7] = lastV; // what native _ftol2 leaves; phase 2 reads this slot
    reg32 result;
    if (ebx == 0xffffffffu)
        result = ebx;
    else
    {
        bool done = false;
        reg32 slot = L + ebx * 4 + 8;
        for (;;)
        {
            if (done)
            {
                result = ebx;
                break;
            }
            const reg32 el = rc::ld32(slot);
            Real Z = C1;
            const reg32 mraw = rc::ld32(el + 0x54);
            const reg32 P = entryParam(el);
            if (P != 0)
            {
                const reg32 vt = rc::ld32(el);
                rc::st32(S - 4, A);
                rc::st32(S - 8, P);
                cpu.ecx = el;
                cpu.edx = vt;
                cpu.eax = P;
                cpu.ebx = ebx;
                cpu.esi = el;
                cpu.edi = slot;
                cpu.ebp = frameEbp;
                cpu.esp = S - 8;
                cpu.fpu.count = fc;
                ::native::callGuest(app, cpu, rc::ld32(vt + 0x2c));
                if (cpu.terminate)
                    return;
                Real R = Real(cpu.fpu.regs[(fc + 1) & 7]);
                Real d = ld(el + 0x54);
                d -= ld(el + 0x58);
                R += d;
                Z = R;
            }
            const uint8_t fl = rc::ld8(E + 8);
            float mf;
            std::memcpy(&mf, &mraw, sizeof mf);
            const Real Mf = Real(mf);
            const Real T = ld(E + 0x14);
            bool stepped = false;
            if (fl & 1)
            {
                Real tt = Mf;
                tt -= Z;
                if (tt < T)
                {
                    ++ebx;
                    slot += 4;
                    stepped = true;
                }
            }
            if (!stepped)
            {
                if (fl & 2)
                {
                    Real tt = Mf;
                    tt -= Z;
                    if (!(tt > T))
                        done = true;
                    else
                    {
                        --ebx;
                        slot -= 4;
                    }
                }
                else
                    done = true;
            }
            const reg32 cnt1 = rc::ld32(L + 4) - 1;
            const reg32 c = int32_t(ebx) <= 0 ? 0 : ebx;
            const reg32 lim = int32_t(cnt1) < int32_t(c) ? cnt1 : c;
            if (ebx != lim)
            {
                result = 0xffffffffu;
                break;
            }
            if (ebx == 0xffffffffu)
            {
                result = ebx;
                break;
            }
            app->allowPreemption(cpu);
        }
    }
    cpu.eax = result;
    cpu.ecx = cpu.edx = L;
    cpu.ebx = savedEbx;
    cpu.ebp = savedEbp;
    cpu.esi = savedEsi;
    cpu.edi = savedEdi;
    cpu.fpu.count = fc;
    cpu.esp = E + 24;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    (void)spans;
}

const shadow::Spec spec = {"sub_4198c0", shadow::Eax | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_4198c0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_4198c0_guest, declare);
}

}}
