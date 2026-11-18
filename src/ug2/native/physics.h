#ifndef UG2_NATIVE_PHYSICS_H_
#define UG2_NATIVE_PHYSICS_H_

// Helpers shared by the car physics natives, each exact to the guest routine it inlines: _ftol2, curve lookup,
// angle, sine, vector and matrix helpers and the traction factor.

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

// sub_43c2b0 then sub_43c350: sine polynomial of a u16 angle.
inline Real sine16(x86::CPU& cpu, reg32 angle)
{
    Real x = Real(x86::sreg32(angle & 0xffffu)) * ld(0x784240);
    Real b;
    const Real t0 = ld(0x786da0);
    if (x >= t0)
    {
        cpu.fpu.compare(x, t0);
        b = ld(0x784250);
        x = x - ld(0x786d9c);
    }
    else
    {
        const Real t1 = ld(0x784254);
        cpu.fpu.compare(x, t1);
        if (x >= t1)
        {
            x = x - ld(0x786d98);
            b = ld(0x7842a0);
        }
        else
            b = ld(0x784250);
    }
    const Real p2 = x * x;
    const Real p3 = p2 * x;
    const Real p5 = p3 * p2;
    const Real p7 = p5 * p2;
    const Real p9 = p7 * p2;
    const Real t3 = p3 * ld(0x786d94);
    const Real t5 = p5 * ld(0x786d90);
    const Real t7 = p7 * ld(0x786d8c);
    const Real t9 = p9 * ld(0x786d88);
    Real a = x - t3;
    a = a + t5;
    a = a - t7;
    a = a + t9;
    return a * b;
}

// sub_43c350: sine of an angle in the game's float units.
inline Real sine(x86::CPU& cpu, float angle)
{
    Real p = Real(angle);
    p *= ld(0x78423c);
    return sine16(cpu, reg32(ftol(p)));
}

struct Vec3
{
    float x, y, z;
};

inline Vec3 load3(reg32 a)
{
    return Vec3{float(ld(a)), float(ld(a + 4)), float(ld(a + 8))};
}

// sub_441000 on a row-major 4x4 matrix m: p times m.
inline Vec3 transform(const float* m, const Vec3& p)
{
    Real x = Real(m[8]) * Real(p.z);
    x += Real(m[4]) * Real(p.y);
    x += Real(m[0]) * Real(p.x);
    x += Real(m[12]);
    Real y = Real(m[9]) * Real(p.z);
    y += Real(m[1]) * Real(p.x);
    y += Real(m[5]) * Real(p.y);
    y += Real(m[13]);
    Real z = Real(m[10]) * Real(p.z);
    z += Real(m[2]) * Real(p.x);
    z += Real(m[6]) * Real(p.y);
    z += Real(m[14]);
    return Vec3{float(x), float(y), float(z)};
}

// sub_43c9d0 then sub_441000: p times the transpose of the guest matrix at g.
inline Vec3 transformTransposed(reg32 g, const Vec3& p)
{
    float m[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m[4 * r + c] = rc::ldf32(g + 16 * reg32(c) + 4 * reg32(r));
    return transform(m, p);
}

// sub_43d0c0: a + s * b.
inline Vec3 addScaled(const Vec3& a, const Vec3& b, float s)
{
    Real y = Real(s) * Real(b.y);
    y += Real(a.y);
    Real z = Real(s) * Real(b.z);
    z += Real(a.z);
    Real x = Real(s) * Real(b.x);
    x += Real(a.x);
    return Vec3{float(x), float(y), float(z)};
}

// sub_43d1e0: cross product as the guest orders it.
inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    Real y = Real(b.x) * Real(a.z);
    y -= Real(b.z) * Real(a.x);
    Real z = Real(b.y) * Real(a.x);
    z -= Real(a.y) * Real(b.x);
    Real x = Real(b.z) * Real(a.y);
    x -= Real(b.y) * Real(a.z);
    return Vec3{float(x), float(y), float(z)};
}

// sub_43cf10: unit vector, or (1, 0, 0) for a zero one.
inline Vec3 normalise(x86::CPU& cpu, const Vec3& v)
{
    Real s = Real(v.x);
    s *= Real(v.x);
    Real t = Real(v.y);
    t *= Real(v.y);
    s += t;
    t = Real(v.z);
    t *= Real(v.z);
    s += t;
    const Real len = cpu.fpu.sqrt(s);
    if (len == ld(0x7d7d3c))
        return Vec3{1.0f, 0.0f, 0.0f};
    const Real inv = ld(0x784250) / len;
    Real a = inv;
    a *= Real(v.x);
    Real b = Real(v.y);
    b *= inv;
    Real c = Real(v.z);
    c *= inv;
    return Vec3{float(a), float(b), float(c)};
}

// sub_59d490: traction factor of a wheel.
inline Real traction(reg32 w)
{
    const Real one = ld(0x784250), zero = ld(0x7d7d3c);
    if (rc::ld32(w + 0x10c) != 0 || ld(w + 0x28) < zero || x86::sreg32(rc::ld32(rc::ld32(w + 0x14) + 0x18)) < 2)
        return one;
    Real x = ld(rc::ld32(w + 0x20) + 0x3ec);
    x -= ld(0x7a1eb0);
    x *= ld(0x7850fc);
    if (x > one)
        x = one;
    else if (x < zero)
        x = zero;
    Real r = one;
    r -= ld(0x7fc71c);
    x *= r;
    x += ld(0x7fc71c);
    return x;
}

// Wheel slip angle from the contact velocity, as sub_5a8280 and sub_59d600 compute it.
inline float slipAngle(x86::CPU& cpu, const Vec3& v)
{
    Registers regs = {0, 0, 0};
    angle(cpu, Real(v.x), Real(v.y), regs);
    const Real k = ld(0x7a1788);
    Real a = Real(x86::sreg32(regs.eax & 0xffffu));
    a *= ld(0x784240);
    float af = float(a);
    if (a > k)
        af = float(Real(af) - (k + k));
    if (Real(v.x) < ld(0x7d7d3c))
    {
        Real t = k;
        t += Real(af);
        af = float(t);
        if (t > k)
            af = float(Real(af) - (k + k));
    }
    return af;
}

}}}

#endif /* !UG2_NATIVE_PHYSICS_H_ */
