// Native sub_5abdf0 (thiscall): five refinement passes over the four view objects' +0x124 values; helpers run through entry points.
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{
void sub_5abdf0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_43c9c0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43c9d0(win32::WinApplication* app, x86::CPU& cpu);
void sub_594cb0(win32::WinApplication* app, x86::CPU& cpu);
void sub_594da0(win32::WinApplication* app, x86::CPU& cpu);
void sub_59d600(win32::WinApplication* app, x86::CPU& cpu);
void sub_5a8280(win32::WinApplication* app, x86::CPU& cpu);
void sub_5aa830(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{
namespace
{
using x86::reg32;
using x86::reg8;

// vldr/vstr fault on unaligned addresses: A selects them for aligned objects.
template <bool A>
inline float lf(const reg8* p)
{
    float v;
    memcpy(&v, A ? __builtin_assume_aligned(p, 4) : p, 4);
    return v;
}

template <bool A>
inline void sf(reg8* p, float v)
{
    memcpy(A ? __builtin_assume_aligned(p, 4) : p, &v, 4);
}

// Sum of the two column products of a view's matrix terms.
template <bool A>
inline void columns(const reg8* v, Real& x, Real& y)
{
    const Real a = Real(lf<A>(v + 0x124)), b = Real(lf<A>(v + 0x128));
    x = Real(lf<A>(v + 0xc0));
    x *= b;
    Real x2 = Real(lf<A>(v + 0xb0));
    x2 *= a;
    x += x2;
    y = Real(lf<A>(v + 0xc4));
    y *= b;
    Real y2 = Real(lf<A>(v + 0xb4));
    y2 *= a;
    y += y2;
}

template <bool A>
inline void correct(reg8* g, reg8* v, const reg8* w, float t14, float t18, float t1c)
{
    Real m1 = Real(t14);
    m1 *= Real(lf<A>(w + 0x94));
    m1 = Real(t18) - m1;
    Real m2 = Real(t14);
    m2 *= Real(lf<A>(w + 0x90));
    m2 += Real(t1c);
    reg32 aux;
    memcpy(&aux, v + 0x14, 4);
    const Real s = Real(lf<false>(g + aux + 0x10));
    const Real c = Real(lf<A>(v + 0x124)), d = Real(lf<A>(v + 0x2c));
    Real r = s;
    r *= c;
    r = Real(lf<A>(v + 0x108)) - r;
    r /= d;
    r *= s;
    m2 *= Real(lf<A>(v + 0xc0));
    m1 *= Real(lf<A>(v + 0xb0));
    m2 += m1;
    r -= m2;
    r /= s;
    r *= d;
    r += c;
    sf<A>(v + 0x124, float(r));
}

// One refinement pass over the four views; returns K+0x158.
template <bool A>
inline reg32 pass(reg8* g, reg32 K2, const reg32 (&vp)[4], const reg32 (&wp)[4], float t20, float t24)
{
    reg8* const v0 = g + vp[0];
    const reg8* const w0 = g + wp[0];
    Real r;
    {
        Real x, y;
        columns<A>(v0, x, y);
        const float t30 = float(x), t34 = float(y);
        Real p = Real(t20);
        p += Real(t30);
        Real q = Real(t24);
        q += Real(t34);
        t20 = float(p);
        t24 = float(q);
        r = Real(t34);
        r *= Real(lf<A>(w0 + 0x90));
        Real s = Real(t30);
        s *= Real(lf<A>(w0 + 0x94));
        r -= s;
    }
    for (int i = 1; i < 4; ++i)
    {
        Real x, y;
        columns<A>(g + vp[i], x, y);
        Real p = Real(t20);
        p += x;
        Real q = Real(t24);
        q += y;
        t20 = float(p);
        t24 = float(q);
        y *= Real(lf<A>(g + wp[i] + 0x90));
        r += y;
        x *= Real(lf<A>(g + wp[i] + 0x94));
        r -= x;
    }
    const reg8* const k = g + K2;
    const Real one = Real(lf<true>(g + 0x784250));
    Real inv = one / Real(lf<A>(k + 0x164));
    Real sx = Real(t20);
    sx *= inv;
    const float t18 = float(sx);
    inv *= Real(t24);
    const float t1c = float(inv);
    const reg32 hasB = rc::ld32(K2 + 0x158);
    Real z = one;
    z /= Real(lf<A>(k + (hasB != 0 ? 0x1d8 : 0x198)));
    const float t14 = float(r / z);

    for (int i = 0; i < 4; ++i)
    {
        reg8* const v = g + vp[i];
        if (v[0x160] == 0 && rc::ld32(vp[i] + 0x10c) == 0)
            correct<A>(g, v, g + wp[i], t14, t18, t1c);
    }
    return hasB;
}

[[gnu::noinline, gnu::cold]] reg32 passUnaligned(reg8* g, reg32 self, reg32 L, reg32& w0);

// Loads the pass inputs; the frame L is 16-aligned.
template <bool A>
inline reg32 passFrom(reg8* g, reg32 self, reg32 L, reg32& w0)
{
    const reg32 vp[4] = {rc::ld32(self + 0x38), rc::ld32(self + 0x3c), rc::ld32(self + 0x40), rc::ld32(self + 0x44)};
    const reg32 wp[4] = {rc::ld32(self + 0x28), rc::ld32(self + 0x2c), rc::ld32(self + 0x30), rc::ld32(self + 0x34)};
    const reg32 K2 = rc::ld32(self + 0x20);
    if (A && ((vp[0] | vp[1] | vp[2] | vp[3] | wp[0] | wp[1] | wp[2] | wp[3] | K2) & 3) != 0) [[unlikely]]
        return passUnaligned(g, self, L, w0);
    w0 = wp[0];
    return pass<A>(g, K2, vp, wp, lf<true>(g + L + 0x40), lf<true>(g + L + 0x44));
}

reg32 passUnaligned(reg8* g, reg32 self, reg32 L, reg32& w0)
{
    return passFrom<false>(g, self, L, w0);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("view_adjust_native"))
    {
        sub_5abdf0_guest(app, cpu);
        return;
    }
    const reg32 E = cpu.esp;
    const reg32 oebx = cpu.ebx, oebp = cpu.ebp, oesi = cpu.esi, oedi = cpu.edi;
    const reg32 self = cpu.ecx;
    reg8* const g = win32::g_guestBase;
    const reg32 L = ((E - 4) & 0xfffffff0u) - 0x84 - 12;

    cpu.ecx = rc::ld32(self + 0x4c);
    cpu.esp = L - 4;
    cpu.ebp = E - 4;
    cpu.esi = self;
    sub_594cb0(app, cpu);
    if (cpu.terminate) return;

    const reg32 stepArg = reg32(int32_t(int8_t(rc::ld8(rc::ld32(rc::ld32(self + 0x5c) + 0x3c) + 0x18))));
    reg32 edi = self + 0x38;
    for (reg32 ebx = 4; ebx != 0; --ebx)
    {
        rc::st32(L - 4, stepArg);
        rc::st32(L - 8, 0);
        cpu.ecx = rc::ld32(edi);
        cpu.ebx = ebx;
        cpu.esp = L - 12;
        cpu.edi = edi;
        sub_5a8280(app, cpu);
        if (cpu.terminate) return;
        edi += 4;
        if (ebx != 1)
            app->allowPreemption(cpu);
    }

    rc::st32(L - 4, rc::ld32(self + 0x20) + 0x30);
    rc::st32(L - 8, L + 0x50);
    cpu.esp = L - 12;
    sub_43c9d0(app, cpu);
    if (cpu.terminate) return;
    const reg32 K110 = rc::ld32(self + 0x20) + 0x110;
    rc::st32(L - 12, K110);
    rc::st32(L - 16, L + 0x50);
    rc::st32(L - 20, L + 0x40);
    cpu.eax = K110;
    cpu.ecx = L + 0x50;
    cpu.esp = L - 24;
    sub_43c9c0(app, cpu);
    if (cpu.terminate) return;

    for (edi = 5; edi != 0; --edi)
    {
        reg32 w0;
        const reg32 hasB = passFrom<true>(g, self, L, w0);
        cpu.eax = rc::ld32(self + 0x44);
        cpu.ecx = rc::ld32(self + 0x4c);
        cpu.edx = w0;
        cpu.ebx = hasB;
        cpu.esp = L - 4;
        cpu.edi = edi;
        sub_594da0(app, cpu);
        if (cpu.terminate) return;
        if (edi != 1)
            app->allowPreemption(cpu);
    }

    reg32 ebx = self + 0x38;
    for (edi = 0; edi < 4; ++edi)
    {
        rc::st32(L - 4, edi);
        cpu.ecx = rc::ld32(ebx);
        cpu.ebx = ebx;
        cpu.esp = L - 8;
        cpu.edi = edi;
        sub_59d600(app, cpu);
        if (cpu.terminate) return;
        ebx = cpu.ebx + 4;
        edi = cpu.edi;
        if (edi != 3)
            app->allowPreemption(cpu);
    }
    cpu.ecx = rc::ld32(self + 0x4c);
    cpu.ebx = ebx;
    cpu.esp = L - 4;
    cpu.edi = edi;
    sub_5aa830(app, cpu);
    if (cpu.terminate) return;
    cpu.ebx = oebx;
    cpu.esi = oesi;
    cpu.edi = oedi;
    cpu.ebp = oebp;
    cpu.esp = E + 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    for (int i = 0; i < 4; ++i)
        spans.push_back({rc::ld32(cpu.ecx + 0x38 + 4 * i) + 0x124, 4, false});
}
const shadow::Spec spec = {"sub_5abdf0", shadow::Ebx | shadow::Ebp | shadow::Edi, 4, false, false};
}

void sub_5abdf0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5abdf0_guest, declare);
}
}}
