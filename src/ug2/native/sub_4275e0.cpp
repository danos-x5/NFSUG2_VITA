// Native replacement of sub_4275e0: walks two corner lists in lockstep, writes the travelled distance to arg4 and returns success.

#include <native/common.h>
#include <native/shadow.h>

namespace ug2
{

void sub_4275e0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_403fc0(win32::WinApplication* app, x86::CPU& cpu);
void sub_4040a0(win32::WinApplication* app, x86::CPU& cpu);
void sub_404ef0(win32::WinApplication* app, x86::CPU& cpu);
void sub_425910(win32::WinApplication* app, x86::CPU& cpu);
void sub_425980(win32::WinApplication* app, x86::CPU& cpu);
void sub_4259c0(win32::WinApplication* app, x86::CPU& cpu);
void sub_425a40(win32::WinApplication* app, x86::CPU& cpu);
void sub_43e6d0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ec0c0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ec250(win32::WinApplication* app, x86::CPU& cpu);
void sub_60be30(win32::WinApplication* app, x86::CPU& cpu);
void sub_614840(win32::WinApplication* app, x86::CPU& cpu);
void sub_75c401(win32::WinApplication* app, x86::CPU& cpu);
void sub_75c461(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;
typedef void (*Fn)(win32::WinApplication*, x86::CPU&);

struct Ctx
{
    win32::WinApplication* app;
    x86::CPU& cpu;
    reg32 ebx, esi, edi, ebp;
    reg32 sp;

    void push(reg32 v) { sp -= 4; rc::st32(sp, v); }

    bool call(Fn fn, reg32 ecx)
    {
        cpu.ecx = ecx;
        cpu.esp = sp - 4;
        cpu.ebx = ebx;
        cpu.esi = esi;
        cpu.edi = edi;
        cpu.ebp = ebp;
        fn(app, cpu);
        return !cpu.terminate;
    }

    Real result()
    {
        const Real v = cpu.fpu.st(0);
        cpu.fpu.pop();
        return v;
    }
};

inline bool inSet(reg32 v) { return v == 9 || v == 0x16 || v == 0xd; }
inline reg32 f2u(Real v) { return bits(float(v)); }
inline Real u2f(reg32 v) { float f; memcpy(&f, &v, 4); return Real(f); }
inline void stf(reg32 address, Real v) { rc::stf32(address, float(v)); }
inline int32_t s8(reg32 address) { return int32_t(int8_t(rc::ld8(address))); }
inline Real absBits(Real v) { return u2f(f2u(v) & 0x7fffffffu); }

struct Consts
{
    Real zero, one, tiny, big;
};

// sub_4043f0: entry/exit times of one projected interval.
inline void slab(Real a0, Real a1, Real r, const Consts& k, Real& lo, Real& hi)
{
    if (absBits(a0) < k.tiny)
    {
        lo = absBits(a1) > r ? u2f(0x461c3c00) : u2f(0xc61c3c00);
        hi = u2f(0x461c3c00);
        return;
    }
    const Real inv = k.one / a0;
    const Real b = (r - a1) * inv;
    const Real a = -((a1 + r) * inv);
    if (a0 < k.zero)
    {
        lo = b;
        hi = a;
    }
    else
    {
        lo = a;
        hi = b;
    }
}

// sub_404240: intersects [lo, hi] with [c, d] by bubble sort.
inline bool clip(Real& lo, Real& hi, Real c, Real d)
{
    Real s[4] = {lo, hi, c, d};
    for (int n = 3; n > 0; --n)
        for (int i = 0; i < n; ++i)
            if (s[i + 1] < s[i])
            {
                const Real t = s[i];
                s[i] = s[i + 1];
                s[i + 1] = t;
            }
    if (!(s[1] >= lo) || !(s[1] <= hi) || !(s[1] >= c) || !(s[1] <= d))
        return false;
    lo = s[1];
    hi = s[2];
    return true;
}

// sub_4044c0: first contact time of two moving boxes.
Real contact(reg32 P, reg32 Q, Real v0, Real v1, Real w0, Real w1, const Consts& k)
{
    const Real p10 = ld(P + 0x10), p14 = ld(P + 0x14), p20 = ld(P + 0x20), p24 = ld(P + 0x24);
    const Real p40 = ld(P + 0x40), p44 = ld(P + 0x44);
    const Real q10 = ld(Q + 0x10), q14 = ld(Q + 0x14), q20 = ld(Q + 0x20), q24 = ld(Q + 0x24);
    const Real q40 = ld(Q + 0x40), q44 = ld(Q + 0x44);
    const Real c11 = p10 * q10 + q14 * p14;
    const Real c12 = q24 * p14 + p10 * q20;
    const Real c21 = q14 * p24 + p20 * q10;
    const Real c22 = q24 * p24 + q20 * p20;

    Real lo, hi, c, d;
    slab(w1 * p24 + w0 * p20, v1 * p24 + v0 * p20, (absBits(c22 * q44) + absBits(c21 * q40)) + absBits(p44), k, lo, hi);
    slab(w1 * p14 + p10 * w0, v1 * p14 + p10 * v0, (absBits(c12 * q44) + absBits(c11 * q40)) + absBits(p40), k, c, d);
    if (!clip(lo, hi, c, d))
        return k.big;
    slab(q24 * w1 + q20 * w0, q24 * v1 + q20 * v0, (absBits(c22 * p44) + absBits(c12 * p40)) + absBits(q44), k, c, d);
    if (!clip(lo, hi, c, d))
        return k.big;
    slab(w1 * q14 + w0 * q10, v1 * q14 + v0 * q10, (absBits(c21 * p44) + absBits(c11 * p40)) + absBits(q40), k, c, d);
    if (!clip(lo, hi, c, d))
        return k.big;
    if (lo > k.zero)
        return lo;
    if (lo < k.zero && hi > k.zero)
        return k.zero;
    return k.big;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("corner_walk_native", true); }))
    {
        sub_4275e0_guest(app, cpu);
        return;
    }
    const reg32 entry = cpu.esp;
    const reg32 self = cpu.ecx;
    const reg32 arg0 = rc::ld32(entry + 4), O = rc::ld32(entry + 8), pD = rc::ld32(entry + 0x10), pOut = rc::ld32(entry + 0x14);
    const Real arg2 = ld(entry + 0xc);
    const reg32 savedEbp = cpu.ebp, savedEbx = cpu.ebx, savedEsi = cpu.esi, savedEdi = cpu.edi;
    const reg32 S = ((entry - 4) & ~reg32(15)) - 0x1100;
    const reg32 A = S + 0xd0, B = S + 0x2a0, arrD = S + 0x470, arrC = S + 0xab0;

    Ctx c{app, cpu, self, savedEsi, rc::ld32(pD), entry - 4, S};
    const reg32 D = c.edi;
    const Real zero = ld(0x7d7d3c);

    c.push(arg0);
    if (!c.call(ug2::sub_403fc0, A)) return;
    c.sp = S;
    c.esi = O;

    const reg32 X = rc::ld32(O + 0x1c);
    reg32 P = 0;
    if (X)
        if (const reg32 y = rc::ld32(X + 0x30))
            P = y + 0x10;
    c.push(P);
    if (!c.call(ug2::sub_403fc0, B)) return;
    c.sp = S;

    reg32 owner = 0;
    if (X)
    {
        const reg32 kind = rc::ld32(X + 0x24);
        if (kind == 3 || kind == 2 || kind == 4)
            owner = rc::ld32(X + 0x2c);
    }
    const auto kindIs = [&](reg32 n) { return X && rc::ld32(rc::ld32(X + 0x14) + 4) == n; };

    if (rc::ld32(rc::ld32(D + 0x14) + 4) == 3)
    {
        if (!c.call(ug2::sub_5ec0c0, D)) return;
        if (c.result() > ld(0x785b6c))
        {
            c.push(D);
            if (!c.call(ug2::sub_4259c0, A)) return;
            c.sp = S;
        }
    }
    if (kindIs(3))
    {
        if (!c.call(ug2::sub_5ec0c0, X)) return;
        if (c.result() > ld(0x785b6c))
        {
            c.push(O);
            if (!c.call(ug2::sub_425a40, B)) return;
            c.sp = S;
        }
    }

    Real s28 = ld(0x783798);
    Real s1c = s28;
    if (kindIs(1))
        s1c = ld(0x7837a0);
    if (rc::ld32(D + 0x24) == 4)
        s28 = Real(1.0f);
    else if (kindIs(3))
        s1c = ld(0x7837a8);
    Real s24 = ld(D + 0x430);

    const reg32 W = rc::ld32(O + 0x10);
    const Real wx = ld(W), wy = ld(W + 4), wz = ld(W + 8);
    Real s20 = cpu.fpu.sqrt((wx * wx + wy * wy) + wz * wz);

    const Real dot1 = (ld(D + 0x88) * ld(D + 0x98) + ld(D + 0x84) * ld(D + 0x94)) + ld(D + 0x80) * ld(D + 0x90);
    if (!c.call(ug2::sub_60be30, O)) return;
    const Real b0 = c.result();

    if (ld(D + 0x42c) < zero)
    {
        const reg32 q6 = rc::ld32(rc::ld32(D + 0x2c) + 0x6f8);
        const reg32 code = q6 ? rc::ld32(q6 + 0x204) : 0x16;
        if (inSet(code) || rc::ld8(A) == 0)
        {
            c.push(D);
            c.push(A);
            if (!c.call(ug2::sub_425910, A)) return;
            c.sp = S;
            s24 = cpu.fpu.abs(s24);
            s28 = ld(0x78379c);
        }
    }

    const reg32 U = rc::ld32(O + 0x18);
    const Real dot2 = (ld(W + 8) * ld(U + 8) + ld(W + 4) * ld(U + 4)) + ld(W) * ld(U);
    if (dot2 < zero && (kindIs(2) || kindIs(3)) && owner)
    {
        const reg32 r = rc::ld32(owner + 0x6f8);
        const reg32 code = r ? rc::ld32(r + 0x204) : 0x16;
        if (inSet(code) || rc::ld8(B) == 0)
        {
            c.push(O);
            c.push(B);
            if (!c.call(ug2::sub_425980, B)) return;
            c.sp = S;
            s20 = cpu.fpu.abs(s20);
            s1c = ld(0x78379c);
        }
    }

    const Real eps = ld(0x78435c);
    if (!(s24 > eps))
        s24 = u2f(0x3a83126f);
    if (!(s20 > eps))
        s20 = u2f(0x3a83126f);
    if (s24 < ld(0x7844c0))
        s28 = ld(0x7837a4);
    if (s20 < ld(0x7844c0))
        s1c = ld(0x7837a4);

    const auto vector = [&](reg32 base) {
        c.sp = S;
        c.push(0x405330);
        c.push(0x405320);
        c.push(0x14);
        c.push(0x50);
        c.push(base);
        return c.call(ug2::sub_75c461, 0);
    };
    if (!vector(arrC)) return;
    c.sp = S;
    if (!vector(arrD)) return;
    c.sp = S;

    c.push(S + 0x80);
    c.push(S + 0x90);
    if (!c.call(ug2::sub_5ec250, D)) return;
    c.sp = S;

    c.push(f2u(s28));
    c.push(rc::ld16(D + 0x40c));
    c.push(S + 0x80);
    c.push(S + 0x90);
    c.push(arrC);
    c.push(A);
    if (!c.call(ug2::sub_4040a0, self)) return;
    c.sp = S;

    c.push(S + 0x80);
    c.push(S + 0x90);
    if (!c.call(ug2::sub_614840, O)) return;
    c.sp = S;

    const reg32 R = rc::ld32(O + 0xc);
    c.push(f2u(s1c));
    c.push(rc::ld32(U + 4));
    c.push(rc::ld32(U));
    if (!c.call(ug2::sub_43e6d0, 0)) return;
    const reg32 angle = cpu.eax;
    c.sp = S - 4;
    c.push(angle);
    c.push(S + 0x80);
    c.push(S + 0x90);
    c.push(arrD);
    c.push(B);
    if (!c.call(ug2::sub_4040a0, self)) return;
    c.sp = S;

    const reg32 T = rc::ld32(D + 0x42c);
    c.ebx = D + 0x60;
    c.edi = D + 0x70;
    rc::st32(S + 0x28, 0);
    rc::st32(S + 0x30, T);
    c.push(c.edi);
    c.push(c.ebx);
    c.push(T);
    if (!c.call(ug2::sub_404ef0, A)) return;
    c.sp = S;
    c.edi = cpu.eax;
    const int32_t idxC = int32_t(c.edi);
    s28 = Real(0.0f);

    c.push(W);
    c.push(R);
    c.push(f2u(dot2));
    if (!c.call(ug2::sub_404ef0, B)) return;
    c.sp = S;
    int32_t idxD = int32_t(cpu.eax);

    Real p0x = ld(D + 0x60), p0y = ld(D + 0x64);
    Real q0x = ld(R), q0y = ld(R + 4);
    reg32 pc = arrC + reg32(idxC) * 0x50;
    reg32 pd = arrD + reg32(idxD) * 0x50;
    Real cx = ld(pc), cy = ld(pc + 4);
    Real dx = ld(pd), dy = ld(pd + 4);
    stf(S + 0x68, cx);
    stf(S + 0x6c, cy);
    stf(S + 0x78, dx);
    stf(S + 0x7c, dy);
    int32_t i2 = idxC;
    Real s34 = u2f(0x47c34f80);
    const Consts k{zero, ld(0x784250), ld(0x7843ec), ld(0x784360)};
    const Real lenEps = ld(0x7845a0);
    const int32_t nA = s8(A), nB = s8(B);

    while (i2 < nA && idxD < nB && s28 <= arg2)
    {
        const Real v1 = cx - p0x, v2 = cy - p0y, v3 = dx - q0x, v4 = dy - q0y;
        const Real len1 = cpu.fpu.sqrt(v2 * v2 + v1 * v1);
        const Real len2 = cpu.fpu.sqrt(v4 * v4 + v3 * v3);
        const Real r1 = len1 / s24;
        const Real r2 = len2 / s20;
        s1c = r1 < r2 ? r1 : r2;
        const Real vx = q0x - p0x, vy = q0y - p0y;
        stf(S + 0x80, vx);
        stf(S + 0x84, vy);
        rc::st32(S + 0x88, 0);
        rc::st32(S + 0x8c, 0);
        Real n1x = u2f(0x3f800000), n1y = u2f(0), n2x = n1x, n2y = n1y;
        if (!(r1 == zero))
        {
            const Real k1 = k.one / r1;
            n1x = k1 * v1;
            n1y = k1 * v2;
        }
        if (!(r2 == zero))
        {
            const Real k2 = k.one / r2;
            n2x = k2 * v3;
            n2y = k2 * v4;
        }
        stf(S + 0x60, n1x);
        stf(S + 0x64, n1y);
        stf(S + 0x70, n2x);
        stf(S + 0x74, n2y);
        const Real wx = n2x - n1x, wy = n2y - n1y;
        stf(S + 0x90, wx);
        stf(S + 0x94, wy);
        rc::st32(S + 0x98, 0);
        rc::st32(S + 0x9c, 0);
        const Real res = contact(pc, pd, vx, vy, wx, wy, k);
        if (res <= s1c && !(res < zero))
        {
            s34 = res + s28;
            break;
        }
        Real m1x = zero, m1y = zero;
        if (cpu.fpu.abs(len1) > lenEps)
        {
            const Real k1 = (s1c * s24) / len1;
            m1x = v1 * k1;
            m1y = v2 * k1;
        }
        p0x = m1x + p0x;
        p0y = m1y + p0y;
        s24 = (s1c * dot1) + s24;
        if (!(s24 > eps))
            s24 = u2f(0x3a83126f);
        Real m2x = zero, m2y = zero;
        if (cpu.fpu.abs(len2) > lenEps)
        {
            const Real k2 = (s1c * s20) / len2;
            m2x = v3 * k2;
            m2y = v4 * k2;
        }
        q0x = m2x + q0x;
        q0y = m2y + q0y;
        s20 = (s1c * b0) + s20;
        if (!(s20 > eps))
            s20 = u2f(0x3a83126f);
        if (!(cpu.fpu.abs(r1 - s1c) <= eps))
        {
            pd += 0x50;
            ++idxD;
            dx = ld(pd);
            dy = ld(pd + 4);
            stf(S + 0x78, dx);
            stf(S + 0x7c, dy);
        }
        else
        {
            pc += 0x50;
            ++i2;
            cx = ld(pc);
            cy = ld(pc + 4);
            stf(S + 0x68, cx);
            stf(S + 0x6c, cy);
        }
        s28 = s1c + s28;
        if (i2 < nA)
        {
            cpu.esp = S;
            app->allowPreemption(cpu);
        }
    }
    c.ebx = pc;
    c.esi = pd;
    c.edi = reg32(i2);
    stf(pOut, s34);
    const bool ok = s34 < arg2 && !(cpu.fpu.abs(s34 - ld(0x784360)) <= ld(0x78435c));

    const auto destroy = [&](reg32 base) {
        c.sp = S;
        c.push(0x405330);
        c.push(0x14);
        c.push(0x50);
        c.push(base);
        return c.call(ug2::sub_75c401, 0);
    };
    if (!destroy(arrD)) return;
    if (!destroy(arrC)) return;
    cpu.eax = (cpu.eax & 0xffffff00u) | (ok ? 1u : 0u);
    cpu.ebx = savedEbx;
    cpu.esi = savedEsi;
    cpu.edi = savedEdi;
    cpu.ebp = savedEbp;
    cpu.esp = entry + 0x18;
}

void declare(x86::CPU& entry, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{rc::ld32(entry.esp + 0x14), 4, false});
}

const shadow::Spec spec = {"sub_4275e0", shadow::Eax | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_4275e0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_4275e0_guest, declare);
}

}}
