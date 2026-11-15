// Native sub_4122c0: table entry lookup, two vtable calls and an inlined sub_401c40 offset for an object's UI position (thiscall, ret 16).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>
#include <winapi/d3d9_native.h>

namespace ug2
{

void sub_4122c0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

namespace br = win32::d3d9::bridge;
using x86::reg32;

float fromBits(reg32 value)
{
    float r;
    memcpy(&r, &value, 4);
    return r;
}

// Fixed operand order add, so NaN payloads match.
Real add(Real a, Real b)
{
    Real r;
#if defined(__aarch64__)
    asm("fadd %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
#elif defined(__arm__)
    asm("vadd.f32 %0, %1, %2" : "=t"(r) : "t"(a), "t"(b));
#else
    r = a + b;
#endif
    return r;
}

// Guest frame layout; every path restores the entry registers.
void body(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 E = cpu.esp;
    const reg32 fpuCount = cpu.fpu.count;
    const reg32 self = cpu.ecx;
    const reg32 entryEdx = cpu.edx;
    const reg32 entryEbx = cpu.ebx, entryEsi = cpu.esi, entryEdi = cpu.edi;
    const reg32 out = rc::ld32(E + 4);

    const auto fail = [&](reg32 ecx) {
        rc::st32(out, 0);
        rc::st32(out + 4, 0);
        rc::st32(out + 8, 0);
        cpu.eax = out;
        cpu.ecx = ecx;
        cpu.edx = entryEdx;
        cpu.ebx = entryEbx;
        cpu.esi = entryEsi;
        cpu.edi = entryEdi;
        cpu.esp = E + 0x14;
    };

    const uint16_t ax = rc::ld16(self + 0x5c);
    if (ax == 0xffff)
    {
        fail(self);
        return;
    }
    const uint16_t cx = rc::ld16(self + 0x5e);
    if (cx == 0xffff)
    {
        fail((self & 0xffff0000u) | cx);
        return;
    }
    const int32_t sax = int16_t(ax);
    const reg32 table = sax < 0 ? 0 : rc::ld32(0x88a334 + reg32(sax) * 4);
    const reg32 offset = reg32(int32_t(int16_t(cx)) * 0x38);
    const reg32 ebx = offset + table + 0x8c;
    if (ebx == 0)
    {
        fail(offset);
        return;
    }

    Real x = ld(E + 8);
    const reg32 arg2 = rc::ld32(E + 0xc);
    x += ld(self + 0x58);
    x -= ld(self + 0x54);
    const int32_t index = int32_t(rc::ld32(E + 0x10));
    rc::stf32(E - 4, float(x));
    rc::st32(E - 8, arg2);
    rc::st32(E - 12, entryEbx);
    rc::st32(E - 16, entryEsi);
    rc::st32(E - 20, entryEdi);
    cpu.edi = reg32(index);

    reg32 L0 = arg2;
    reg32 T = bits(float(x));
    if (index != -1)
    {
        const reg32 vtable = rc::ld32(self);
        rc::st32(E - 24, reg32(index));
        cpu.eax = vtable;
        cpu.edx = arg2;
        cpu.ebx = ebx;
        cpu.esi = self;
        cpu.esp = E - 28;
        br::callGuest(app, cpu, rc::ld32(vtable + 8));
        if (cpu.terminate)
            return;
        const reg32 result = cpu.eax;

        reg32 slot;
        if (index == 0)
            slot = rc::ld32(self + 4);
        else if (index >= 2 && index < 6)
            slot = bits(float(Real(rc::ldf32(self + reg32(index) * 4))));
        else
            slot = 0;
        rc::st32(E + 8, slot);
        rc::st32(E - 24, result);

        // Inlined sub_401c40; its stack writes mirror the guest's.
        const reg32 S = E - 32;
        rc::st32(S, ebx);
        const reg32 hi = reg32(rc::ld16(ebx + 0x16)) << 8;
        const reg32 lo = reg32(rc::ld16(ebx + 0x14)) << 8;
        rc::st32(S, hi);
        Real f1 = Real(x86::sreg32(hi));
        rc::st32(S, lo);
        f1 *= Real(Real(rc::ldf32(0x784278)));
        Real f2 = Real(x86::sreg32(lo));
        f2 *= Real(Real(rc::ldf32(0x784278)));
        rc::stf32(S, float(f2));
        if (f2 > f1)
            f1 = f2;
        const reg32 scaled = reg32(x86::sreg32(x86::sreg8(rc::ld8(result + ebx + 0x22)))) << 9;
        rc::st32(E - 24, scaled);
        Real f3 = Real(x86::sreg32(scaled));
        f3 *= Real(Real(rc::ldf32(0x784278)));
        f1 *= f3;

        L0 = bits(float(add(add(f1, Real(fromBits(slot))), ld(E + 0xc))));
        rc::st32(E - 8, L0);
        T = rc::ld32(E - 4);
    }

    const reg32 vtable = rc::ld32(self);
    rc::st32(E - 24, L0);
    rc::st32(E - 28, T);
    rc::st32(E - 32, out);
    rc::st32(E - 36, ebx);
    cpu.eax = out;
    cpu.ecx = self;
    cpu.edx = vtable;
    cpu.ebx = ebx;
    cpu.esi = self;
    cpu.esp = E - 40;
    cpu.fpu.count = fpuCount;
    br::callGuest(app, cpu, rc::ld32(vtable + 0x38));
    if (cpu.terminate)
        return;
    cpu.ebx = entryEbx;
    cpu.esi = entryEsi;
    cpu.edi = entryEdi;
    cpu.esp = E + 0x14;
    cpu.fpu.count = fpuCount;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back({rc::ld32(cpu.esp + 4), 12, false});
}

const shadow::Spec spec = {"sub_4122c0", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, true};

}

void sub_4122c0(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("ui_pos_native", true); }))
    {
        sub_4122c0_guest(app, cpu);
        return;
    }
    shadow::call(spec, app, cpu, body, sub_4122c0_guest, declare);
}

}}
