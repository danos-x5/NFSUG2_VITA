// Native sub_5764d0: piecewise-linear lookup of a float table (thiscall, ret 4), exact to the guest.

#include <native/common.h>
#include <native/shadow.h>

namespace ug2
{

void sub_5764d0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// _ftol2 outside the int32 range, as native::sub_75bed4.
__attribute__((noinline, cold)) x86::reg64 wideTrunc(float value)
{
    asm volatile("" ::: "memory"); // makes the caller reload p, so p need not live in d8
    return x86::reg64(x86::truncInteger64(double(value)));
}

// Interpolates a table at the index of the scaled argument.
void body(win32::WinApplication*, x86::CPU& cpu)
{
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 self = cpu.ecx;
    const x86::reg32 fpuCount = cpu.fpu.count;
    const x86::reg32 count = rc::ld32(self);
    Real p = Real(rc::ldf32(entry + 4));
    p -= ld(self + 4);
    p *= ld(self + 0xc);
    rc::stf32(entry + 4, float(p));
    x86::reg32 idx, high;
    if (__builtin_expect((bits(p) & 0x7fffffffu) < 0x4f000000u, 1))
    {
        idx = x86::reg32(x86::sreg32(p));
        high = x86::reg32(x86::sreg32(idx) >> 31);
    }
    else
    {
        const x86::reg64 wide = wideTrunc(p);
        idx = x86::reg32(wide);
        high = x86::reg32(wide >> 32);
        p = Real(rc::ldf32(entry + 4));
    }
    const x86::reg32 slot = (fpuCount + 1) & 7;
    cpu.ecx = idx;
    cpu.esp = entry + 8;
    cpu.fpu.count = fpuCount + 1;
    const x86::reg32 table = rc::ld32(self + 0x10);
    if (x86::sreg32(idx) < 0 || p < ld(0x7d7d3c))
    {
        cpu.eax = table;
        cpu.edx = high;
        cpu.fpu.regs[slot] = ld(table);
        return;
    }
    const x86::reg32 last = count - 1;
    if (!(x86::sreg32(idx) < x86::sreg32(last)))
    {
        cpu.eax = last;
        cpu.edx = high;
        cpu.fpu.regs[slot] = ld(table + count * 4 - 4);
        return;
    }
    Real fi = Real(x86::sreg32(idx));
    if (fi > p)
    {
        fi -= ld(0x784250);
    }
    fi = p - fi;
    Real w = ld(0x784250);
    w -= fi;
    const x86::reg32 q = table + idx * 4;
    w *= ld(q);
    fi *= ld(q + 4);
    Real r = w;
    r += fi;
    cpu.eax = q;
    cpu.edx = table;
    cpu.fpu.regs[slot] = r;
}

// Argument slot is overwritten with the scaled value.
void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back({cpu.esp + 4, 4, false});
}

const shadow::Spec spec = {"sub_5764d0", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};

}

void sub_5764d0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5764d0_guest, declare);
}

}}
