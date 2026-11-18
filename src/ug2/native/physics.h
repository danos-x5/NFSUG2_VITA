#ifndef UG2_NATIVE_PHYSICS_H_
#define UG2_NATIVE_PHYSICS_H_

// Helpers shared by the car physics natives, each exact to the guest routine it inlines: _ftol2, the curve lookup
// sub_5764d0 and the angle sub_43c570.

#include <native/common.h>

namespace ug2 { namespace native { namespace physics {

using x86::reg32;

// eax, ecx and edx as the inlined routine leaves them.
struct Registers
{
    reg32 eax, ecx, edx;
};

// _ftol2 outside the int32 range, as native::sub_75bed4.
__attribute__((noinline, cold)) inline x86::reg64 wideTrunc(float value)
{
    asm volatile("" ::: "memory");
    return x86::reg64(x86::truncInteger64(double(value)));
}

// _ftol2: truncation to edx:eax.
inline x86::reg64 ftol(Real p)
{
    if (__builtin_expect((bits(float(p)) & 0x7fffffffu) < 0x4f000000u, 1))
        return x86::reg64(int64_t(x86::sreg32(p)));
    return wideTrunc(float(p));
}

// sub_5764d0: piecewise-linear lookup of a curve at the scaled argument.
inline Real lookup(reg32 curve, float argument, Registers& out)
{
    const reg32 count = rc::ld32(curve);
    Real p = Real(argument);
    p -= ld(curve + 4);
    p *= ld(curve + 0xc);
    const float stored = float(p);
    reg32 idx, high;
    if (__builtin_expect((bits(stored) & 0x7fffffffu) < 0x4f000000u, 1))
    {
        idx = reg32(x86::sreg32(p));
        high = reg32(x86::sreg32(idx) >> 31);
    }
    else
    {
        const x86::reg64 wide = wideTrunc(stored);
        idx = reg32(wide);
        high = reg32(wide >> 32);
        p = Real(stored);
    }
    out.ecx = idx;
    const reg32 table = rc::ld32(curve + 0x10);
    if (x86::sreg32(idx) < 0 || p < ld(0x7d7d3c))
    {
        out.eax = table;
        out.edx = high;
        return ld(table);
    }
    if (!(x86::sreg32(idx) < x86::sreg32(count - 1)))
    {
        out.eax = count - 1;
        out.edx = high;
        return ld(table + count * 4 - 4);
    }
    Real fi = Real(x86::sreg32(idx));
    if (fi > p)
        fi -= ld(0x784250);
    fi = p - fi;
    Real w = ld(0x784250);
    w -= fi;
    const reg32 q = table + idx * 4;
    out.eax = q;
    out.edx = table;
    w *= ld(q);
    fi *= ld(q + 4);
    Real r = w;
    r += fi;
    return r;
}

inline Real lookup(reg32 curve, float argument)
{
    Registers unused;
    return lookup(curve, argument, unused);
}

constexpr reg32 AngleTable = 0x7eacb0;

// sub_43c570: 16-bit angle of (x, y); out takes eax on entry.
inline void angle(x86::CPU& cpu, Real x, Real y, Registers& out)
{
    const Real zero = ld(0x7d7d3c);
    reg32 esi = 0;
    cpu.fpu.compare(x, zero);
    if (x < zero)
    {
        x = Real(float(-x));
        esi = 1;
    }
    if (y < zero)
    {
        esi ^= 3;
        y = -y;
    }
    reg32 eax = out.eax, ecx, edx = out.edx;
    cpu.fpu.compare(x, y);
    if (x > y)
    {
        Real r = y;
        r /= x;
        r *= ld(0x786db8);
        const x86::reg64 t = ftol(r);
        eax = reg32(t);
        edx = reg32(t >> 32);
        const reg32 at = AngleTable - eax;
        ecx = ((at & 0xffff0000u) | rc::ld8(at)) << 5;
    }
    else if (cpu.fpu.compare(y, x), y > x)
    {
        Real r = x;
        r /= y;
        r *= ld(0x786db8);
        const reg32 idx = reg32(ftol(r));
        edx = AngleTable - idx;
        eax = ((idx & 0xffff0000u) | rc::ld8(edx)) << 5;
        ecx = 0x4000u - eax;
    }
    else
        ecx = y == zero ? 0 : 0x2000u;
    if (esi == 0)
        eax = (eax & 0xffff0000u) | (ecx & 0xffffu);
    else if (esi == 3)
        eax = 0u - ecx;
    else if (esi == 1)
        eax = 0x8000u - ecx;
    else
        eax = ecx + 0x8000u;
    out.eax = eax;
    out.ecx = ecx;
    out.edx = edx;
}

}}}

#endif /* !UG2_NATIVE_PHYSICS_H_ */
