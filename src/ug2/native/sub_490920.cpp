// Native sub_490920: eRenderLightFlare, the flare quad of one light in a view (cdecl, 8 arguments). Vector helpers
// are inline; their stack arguments and junk fourth words are not written.

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_490920_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_401020(win32::WinApplication* app, x86::CPU& cpu);
void sub_401250(win32::WinApplication* app, x86::CPU& cpu);
void sub_42faf0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43cf10(win32::WinApplication* app, x86::CPU& cpu);
void sub_43d0c0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43d540(win32::WinApplication* app, x86::CPU& cpu);
void sub_43d610(win32::WinApplication* app, x86::CPU& cpu);
void sub_486ed0(win32::WinApplication* app, x86::CPU& cpu);
void sub_486ff0(win32::WinApplication* app, x86::CPU& cpu);
void sub_487040(win32::WinApplication* app, x86::CPU& cpu);
void sub_487080(win32::WinApplication* app, x86::CPU& cpu);
void sub_4870b0(win32::WinApplication* app, x86::CPU& cpu);
void sub_487120(win32::WinApplication* app, x86::CPU& cpu);
void sub_487160(win32::WinApplication* app, x86::CPU& cpu);
void sub_487190(win32::WinApplication* app, x86::CPU& cpu);
void sub_487210(win32::WinApplication* app, x86::CPU& cpu);
void sub_487270(win32::WinApplication* app, x86::CPU& cpu);
void sub_487320(win32::WinApplication* app, x86::CPU& cpu);
void sub_48b7b0(win32::WinApplication* app, x86::CPU& cpu);
void sub_48b7f0(win32::WinApplication* app, x86::CPU& cpu);
void sub_4901d0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ba8b0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bb1c0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc1f0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc300(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc450(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc4a0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5c7e80(win32::WinApplication* app, x86::CPU& cpu);
void sub_5e0b40(win32::WinApplication* app, x86::CPU& cpu);
void sub_5e0bf0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5e7e70(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ed980(win32::WinApplication* app, x86::CPU& cpu);
void sub_5f0e30(win32::WinApplication* app, x86::CPU& cpu);
void sub_75bed4(win32::WinApplication* app, x86::CPU& cpu);
void sub_75c9b0(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

typedef x86::reg32 reg32;
typedef void (*Callee)(win32::WinApplication*, x86::CPU&);
typedef uint32_t __attribute__((may_alias)) Word;

enum { B = 1, S = 2, D = 4, P = 8 };

const reg32 Zero = 0x7d7d3c;
const reg32 One = 0x784250;
const reg32 Max = 0x784544;
const reg32 Alpha = 0x82d528;
const reg32 Fade = 0x7f39a0;

signed char s_on;

#define CALL(...) do { __VA_ARGS__; if (cpu.terminate) [[unlikely]] return; } while (0)
#define LCALL(...) do { __VA_ARGS__; if (cpu.terminate) [[unlikely]] return false; } while (0)

// The guest frame and the callee-saved registers as the translated body last synced them.
struct Frame
{
    win32::WinApplication* app;
    x86::CPU& cpu;
    x86::reg8* h;
    reg32 f, sp, ebx, esi, edi, ebp;

    Real fl(int k) const { return Real(*reinterpret_cast< const fast::AlignedFloat* >(h + k)); }
    void fs(int k, float v) const { *reinterpret_cast< fast::AlignedFloat* >(h + k) = v; }
    reg32 w(int k) const { return *reinterpret_cast< const Word* >(h + k); }
    void sw(int k, reg32 v) const { *reinterpret_cast< Word* >(h + k) = v; }
    void copy3(int to, int from) const
    {
        sw(to, w(from));
        sw(to + 4, w(from + 4));
        sw(to + 8, w(from + 8));
    }
    void push(reg32 v)
    {
        sp -= 4;
        rc::st32(sp, v);
    }
    void sync(unsigned mask) const
    {
        if (mask & B)
            cpu.ebx = ebx;
        if (mask & S)
            cpu.esi = esi;
        if (mask & D)
            cpu.edi = edi;
        if (mask & P)
            cpu.ebp = ebp;
    }
    reg32 call(Callee fn, unsigned pops, unsigned mask = 0)
    {
        sync(mask);
        cpu.esp = sp - 4;
        fn(app, cpu);
        sp += 4 * pops;
        return cpu.eax;
    }
    reg32 call(Callee fn, unsigned pops, reg32 self, unsigned mask)
    {
        cpu.ecx = self;
        return call(fn, pops, mask);
    }
};

// Hides a value: keeps an addition's operand order (NaN choice).
inline Real late(Real v)
{
    __asm__ volatile("" : "+w"(v));
    return v;
}

// An aligned point through a 4x4 matrix (sub_487210).
[[gnu::always_inline]] inline void through(reg32 m, reg32 point, float& x, float& y, float& z)
{
    if (fast::aligned(m)) [[likely]]
        fast::transform< true >(m, point, x, y, z);
    else
        fast::transform< false >(m, point, x, y, z);
}

// _ftol2 of a value, low word.
inline reg32 ftol(Real v)
{
    return reg32(x86::truncInteger64(double(v)));
}

[[gnu::cold, gnu::noinline]] bool probe(Frame& fr, reg32 light, reg32 m, reg32 depth)
{
    x86::CPU& cpu = fr.cpu;
    const reg32 f = fr.f;
    const reg32 object = f + 0x2f8;
    LCALL(fr.call(sub_5e0b40, 0, object, B | S | D));
    const Real z = ld(light + 0x18);
    rc::st32(f + 0x2c, rc::ld32(light + 0x14));
    Real y = ld(f + 0x2c);
    rc::st32(f + 0x40, rc::ld32(light + 0x10));
    y *= ld(m + 0x10);
    Real zz = z;
    zz *= ld(m + 0x20);
    fr.push(f + 0x40);
    rc::st32(f + 0x61c, 0);
    y += zz;
    Real x = ld(f + 0x40);
    x *= ld(m);
    y += x;
    y += ld(m + 0x30);
    Real vx = ld(f + 0x40);
    vx *= ld(m + 4);
    Real vy = ld(f + 0x2c);
    vy *= ld(m + 0x14);
    vx += vy;
    Real vz = z;
    vz *= ld(m + 0x24);
    vx += vz;
    vx += ld(m + 0x34);
    Real wx = ld(f + 0x40);
    wx *= ld(m + 8);
    Real wy = ld(f + 0x2c);
    wy *= ld(m + 0x18);
    wx += wy;
    Real wz = z;
    wz *= ld(m + 0x28);
    wx += wz;
    wx += ld(m + 0x38);
    rc::stf32(f + 0xb8, float(wx));
    rc::st32(f + 0x48, rc::ld32(f + 0xb8));
    rc::stf32(f + 0x40, float(y));
    rc::stf32(f + 0x44, float(vx));
    reg32 seen = 0;
    LCALL(seen = fr.call(sub_486ed0, 1, object, P));
    if ((seen & 0xff) == 0)
        rc::st32(light + 0x1c, 0x461c3c00);
    else
    {
        fr.push(0);
        fr.push(0);
        fr.push(0);
        fr.push(f + 0x40);
        LCALL(fr.call(sub_5e7e70, 4, object, 0));
        const Real h = cpu.fpu.regs[(depth + 1) & 7];
        cpu.fpu.count = depth;
        rc::stf32(light + 0x1c, float(h));
        Real d = h;
        d -= ld(light + 0x18);
        d = cpu.fpu.abs(d);
        const double fa = d;
        const double fb = ld(0x7844f4);
        if (fa < fb)
            rc::st32(light + 0x1c, 0x461c3c00);
        else
        {
            Real zf = ld(f + 0x48);
            zf -= h;
            Real r = h;
            r -= zf;
            rc::stf32(light + 0x1c, float(r));
        }
    }
    rc::st32(f + 0x61c, 0xffffffffu);
    LCALL(fr.call(sub_5e0bf0, 0, object, 0));
    return true;
}

// Kinds 3 and 4: fades by facing, ended when away.
[[gnu::cold, gnu::noinline]] bool facing(Frame& fr, reg32 kind, reg32 scaleSlot, bool& ended)
{
    x86::CPU& cpu = fr.cpu;
    const reg32 f = fr.f;
    ended = false;
    fr.push(kind == 3 ? 0 : 1);
    reg32 r = 0;
    LCALL(r = fr.call(sub_5ed980, 1, 0x89e7a0, 0));
    fr.push(r);
    LCALL(r = fr.call(sub_5f0e30, 1, rc::ld32(0x890080), B | P));
    fr.edi = r;
    fr.push(r + 0x60);
    fr.push(f + 0x40);
    LCALL(fr.call(sub_401250, 2, fr.ebx, 0));
    fr.push(f + 0x40);
    fr.push(f + 0x40);
    LCALL(fr.call(sub_43cf10, 0, 0));
    Real t = ld(f + 0x44);
    t *= ld(fr.edi + 0x94);
    fr.sp += 8;
    Real u = ld(f + 0x48);
    u *= ld(fr.edi + 0x98);
    t += u;
    Real v = ld(f + 0x40);
    v *= ld(fr.edi + 0x90);
    t += v;
    const double fa = t;
    const double fb = ld(0x803a70);
    if (!(fa >= fb))
    {
        ended = true;
        return true;
    }
    t -= ld(0x803a70);
    Real k = ld(One);
    k -= ld(0x803a70);
    t /= k;
    rc::stf32(scaleSlot, float(t));
    return true;
}

// Kind 0: turns the quad axes by the view's angle.
[[gnu::noinline]] bool spin(Frame& fr, reg32 self)
{
    x86::CPU& cpu = fr.cpu;
    const reg32 f = fr.f;
    fr.push(f + 0x130);
    fr.push(f + 0x130);
    LCALL(fr.call(sub_43cf10, 0, 0));
    fr.sp += 8;
    fr.push(f + 0x140);
    fr.push(f + 0x240);
    LCALL(fr.call(sub_5bc4a0, 2, self, 0));
    reg32 count = 0;
    LCALL(count = fr.call(sub_5bc1f0, 0, self, 0));
    fr.sw(0xe4, count);
    Real angle = Real(x86::sreg32(count));
    Real num = fr.fl(0x240);
    num /= angle;
    num *= ld(0x7f3d50);
    rc::stf32(0x82db00, float(num));
    fr.push(rc::ld32(0x82db00));
    reg32 table = 0;
    LCALL(table = fr.call(sub_401020, 0, B | S));
    fr.push(table);
    fr.push(f + 0x130);
    fr.push(f + 0xf0);
    LCALL(fr.call(sub_5ba8b0, 0, 0));
    fr.sp += 0x10;
    const auto row = [&](int vector, int c) {
        Real acc = fr.fl(0x110 + c);
        acc *= fr.fl(vector + 8);
        Real mid = fr.fl(0x100 + c);
        mid *= fr.fl(vector + 4);
        acc += mid;
        Real low2 = fr.fl(0xf0 + c);
        low2 *= fr.fl(vector);
        acc += low2;
        acc += fr.fl(0x120 + c);
        return acc;
    };
    {
        const Real px = row(0x10, 0), py = row(0x10, 4);
        const Real pz = row(0x10, 8);
        fr.fs(0x268, float(pz));
        fr.sw(0x18, fr.w(0x268));
        fr.fs(0x10, float(px));
        fr.fs(0x14, float(py));
    }
    {
        const Real qx = row(0x50, 0), qy = row(0x50, 4);
        const Real qz = row(0x50, 8);
        fr.fs(0x238, float(qz));
        fr.sw(0x58, fr.w(0x238));
        fr.fs(0x50, float(qx));
        fr.fs(0x54, float(qy));
    }
    return true;
}

// The frame as host memory; it is 16-aligned, so floats load and store directly.
struct Slots
{
    x86::reg8* h;

    Real fl(int k) const { return Real(*reinterpret_cast< const fast::AlignedFloat* >(h + k)); }
    void fs(int k, float v) const { *reinterpret_cast< fast::AlignedFloat* >(h + k) = v; }
    reg32 w(int k) const { return *reinterpret_cast< const Word* >(h + k); }
    void sw(int k, reg32 v) const { *reinterpret_cast< Word* >(h + k) = v; }
};

enum { Ended = 0, Drawn = 1, Terminated = 2 };

#define RUN(...) do { __VA_ARGS__; if (cpu.terminate) [[unlikely]] return Terminated; } while (0)

// The quad of a visible light: distance fade, axes, two passes.
[[gnu::noinline]] int tail(win32::WinApplication* app, x86::CPU& cpu, const reg32 f, const reg32 entry, reg32 ebx, const reg32 bytes)
{
    const Slots s{rc::host(f)};
    const reg32 bp = entry - 4;
    const reg32 depth = cpu.fpu.count;
    const reg32 self = rc::ld32(entry + 4);
    const reg32 light = rc::ld32(entry + 8);
    const reg32 m = rc::ld32(entry + 12);
    const reg32 scaleSlot = entry + 16;
    const reg32 kind = rc::ld32(entry + 24);
    const reg32 view = rc::ld32(self + 0x40);
    const reg32 byteA = reg32(x86::sreg32(bytes) >> 8), byteB = reg32(x86::sreg8(bytes));
    reg32 esi = view, edi = self;
    const auto cold = [&]() __attribute__((always_inline)) { return Frame{app, cpu, s.h, f, f, ebx, esi, edi, bp}; };

    {
        s.sw(0x80, rc::ld32(view + 0x40));
        s.sw(0x84, rc::ld32(view + 0x44));
        s.sw(0x88, rc::ld32(view + 0x48));
        const float dx = float(s.fl(0x80) - s.fl(0x68));
        const float dy = float(s.fl(0x84) - s.fl(0x6c));
        const float dz = float(s.fl(0x88) - s.fl(0x70));
        s.fs(0x40, dx);
        s.fs(0x44, dy);
        s.fs(0x48, dz);
        Real dot = Real(dz) * Real(dz);
        dot += Real(dy) * Real(dy);
        dot += Real(dx) * Real(dx);
        s.fs(0x80, float(dot));
        s.fs(0x84, dy);
        s.fs(0x88, dz);
    }

    s.sw(0xc8, 0x3f800000);
    {
        Real low = ld(0x78d798), high = ld(0x78d79c);
        if (kind == 1)
        {
            low = ld(0x78d7a8);
            high = ld(0x78d7ac);
        }
        else if (kind == 2)
        {
            low = ld(0x78d7a0);
            high = ld(0x78d7a4);
        }
        const Real distance = s.fl(0x80);
        if (double(distance) > double(high))
            return Ended;
        if (double(distance) > double(low))
        {
            Real d = distance;
            d -= low;
            Real r = high;
            r -= low;
            d /= r;
            Real out = ld(One);
            out -= d;
            s.fs(0xc8, float(out));
        }
    }

    {
        const Real x = s.fl(0x40), y = s.fl(0x44), z = s.fl(0x48);
        Real sum = x * x;
        sum += y * y;
        sum += z * z;
        const Real inverse = ld(One) / cpu.fpu.sqrt(sum);
        s.fs(0x40, float(inverse * x));
        s.fs(0x44, float(inverse * y));
        s.fs(0x48, float(inverse * z));
    }
    {
        const Real x = s.fl(0x30), y = s.fl(0x34), z = s.fl(0x38), w = Real(0.0f);
        Real ox = ld(m + 0x30) * w;
        ox += ld(m + 0x20) * z;
        ox += ld(m + 0x10) * y;
        ox += ld(m) * x;
        Real oy = ld(m + 0x34) * w;
        oy += ld(m + 4) * x;
        oy += ld(m + 0x24) * z;
        oy += ld(m + 0x14) * y;
        Real oz = ld(m + 0x38) * w;
        oz += ld(m + 8) * x;
        oz += ld(m + 0x28) * z;
        oz += ld(m + 0x18) * y;
        const float vx = float(ox), vy = float(oy), vz = float(oz);
        Real dot = Real(vz) * s.fl(0x48);
        dot += Real(vy) * s.fl(0x44);
        dot += Real(vx) * s.fl(0x40);
        s.fs(0x80, float(dot));
        s.fs(0x84, vy);
        s.fs(0x88, vz);
    }

    if (byteB & 1)
    {
        s.fs(0x30, float(cpu.fpu.abs(s.fl(0x80))));
        s.sw(0x80, s.w(0x30));
    }
    else if (double(s.fl(0x80)) < double(ld(Zero)))
    {
        s.sw(0x30, 0);
        s.sw(0x80, 0);
    }

    s.sw(0x10, rc::ld32(view));
    s.sw(0x14, rc::ld32(view + 0x10));
    s.sw(0x18, rc::ld32(view + 0x20));
    s.sw(0x50, rc::ld32(view + 4));
    s.sw(0x54, rc::ld32(view + 0x14));
    s.sw(0x130, rc::ld32(view + 8));
    s.sw(0x134, rc::ld32(view + 0x18));
    s.sw(0x58, rc::ld32(view + 0x24));
    s.sw(0x138, rc::ld32(view + 0x28));
    cpu.edi = edi;
    {
        const Real radius = ld(self + 0xc);
        float x, y, z;
        through(view, f + 0x68, x, y, z);
        s.fs(0xb0, x);
        s.fs(0xb4, y);
        s.fs(0xb8, z);
        Real ratio = Real(z);
        ratio /= radius;
        s.sw(0x140, s.w(0x68));
        s.sw(0x144, s.w(0x6c));
        s.sw(0x148, s.w(0x70));
        s.sw(0x160, s.w(0x40));
        s.sw(0x164, s.w(0x44));
        s.sw(0x168, s.w(0x48));
        s.fs(0x2c, float(ratio));
    }
    reg32 cursor = byteA * 8 + 0x7f3cc8;

    for (int pass = 0; pass < 2; ++pass, cursor += 4)
    {
        if (pass == 1)
            app->allowPreemption(cpu);
        const reg32 rec2 = rc::ld32(cursor);
        esi = rec2;
        if (rec2 == 0)
            continue;
        if (kind == 2)
        {
            s.sw(-4, 0);
            s.sw(-8, 0);
            s.sw(-12, 0x3394fe62);
            cpu.ebx = ebx;
            cpu.esi = esi;
            cpu.esp = f - 16;
            RUN(sub_4901d0(app, cpu));
            ebx = cpu.eax;
        }
        else
            ebx = rc::ld32(rc::ld32(rec2 + 0x28) * 4 + 0x82cc10);
        if (ebx == 0)
            continue;
        rc::st32(Alpha, rc::ld32(scaleSlot));
        if (byteB & 4)
        {
            const Real farD = s.fl(0x80);
            const Real nearD = ld(rec2 + 0x18);
            cpu.fpu.regs[(depth + 1) & 7] = farD;
            cpu.fpu.regs[(depth + 2) & 7] = nearD;
            cpu.fpu.count = depth + 2;
            cpu.ebx = ebx;
            cpu.esi = esi;
            cpu.esp = f - 4;
            RUN(sub_75c9b0(app, cpu));
            Real v = cpu.fpu.regs[(depth + 1) & 7];
            cpu.fpu.count = depth;
            v *= ld(scaleSlot);
            rc::stf32(Alpha, float(v));
            if (double(v) > double(ld(One)))
                rc::st32(Alpha, 0x3f800000);
        }
        const Real alpha = ld(Alpha);
        Real span = ld(rec2 + 4);
        span -= ld(rec2);
        span *= alpha;
        span += ld(rec2);
        s.fs(0xa8, float(span));
        if (double(span) <= double(ld(Zero)))
            continue;
        {
            const Real push = ld(rec2 + 0x1c);
            const float y = float(push * s.fl(0x164) + s.fl(0x144));
            const float z = float(push * s.fl(0x168) + s.fl(0x148));
            const float x = float(push * s.fl(0x160) + s.fl(0x140));
            s.fs(0x30, x);
            s.fs(0x34, y);
            s.fs(0x38, z);
        }
        if (double(s.fl(0x2c)) < double(ld(rec2 + 0x20)))
            s.sw(0x2c, rc::ld32(rec2 + 0x20));
        if (double(s.fl(0x2c)) > double(ld(rec2 + 0x24)))
            s.sw(0x2c, rc::ld32(rec2 + 0x24));
        Real quad = s.fl(0xa8);
        quad *= s.fl(0x2c);
        const float gain = float(quad);
        s.fs(0xa8, gain);
        s.sw(0x1c0, 0);
        s.sw(0x1c4, 0);
        s.sw(0x1c8, 0x3f800000);
        s.sw(0x1cc, 0);
        s.sw(0x1d0, 0x3f800000);
        s.sw(0x1d4, 0x3f800000);
        s.sw(0x1d8, 0);
        s.sw(0x1dc, 0x3f800000);
        s.h[0x1f0] = 0;

        {
            s.fs(0x78, float(alpha * ld(rec2 + 8)));
            s.fs(0xec, float(alpha * ld(rec2 + 0xc)));
            s.fs(0xe8, float(alpha * ld(rec2 + 0x10)));
            Real c3 = alpha;
            c3 *= ld(rec2 + 0x14);
            c3 *= s.fl(0xc8);
            c3 = c3 + c3;
            s.fs(0xac, float(c3));
            if (double(c3) > double(ld(Max)))
                s.sw(0xac, 0x437f0000);
        }

        if (kind == 0)
        {
            Frame fr = cold();
            if (!spin(fr, self))
                return Terminated;
        }

        esi = bits(gain);
        {
            const Real g = Real(gain);
            s.fs(0x10, float(g * s.fl(0x10)));
            s.fs(0x14, float(g * s.fl(0x14)));
            s.fs(0x18, float(g * s.fl(0x18)));
            s.fs(0x50, float(g * s.fl(0x50)));
            s.fs(0x54, float(g * s.fl(0x54)));
            s.fs(0x58, float(g * s.fl(0x58)));
        }
        Real tint = s.fl(0xac);
        if (kind == 2)
        {
            Real d = ld(light + 0x18);
            d -= ld(light + 0x1c);
            Real g = ld(Fade);
            if (double(d) < double(ld(0x78d7b0)))
            {
                g *= ld(0x784260);
                g = Real(float(g));
            }
            s.fs(0x50, float(g * s.fl(0x50)));
            s.fs(0x54, float(g * s.fl(0x54)));
            s.fs(0x58, float(g * s.fl(0x58)));
            tint = ld(Max);
        }
        const reg32 fixed = rc::ld32(entry + 32);
        reg32 colour = 0;
        cpu.ebx = ebx;
        if (fixed != 0)
        {
            esi = ftol(tint) << 24;
            colour = fixed;
        }
        else
        {
            esi = ftol(tint) << 8;
            esi = (esi | ftol(s.fl(0xe8))) << 8;
            esi = (esi | ftol(s.fl(0xec))) << 8;
            colour = ftol(s.fl(0x78));
        }
        const reg32 packed = esi | colour;
        esi = packed;
        {
            // Loads stay where the guest has them: operand order decides which NaN an addition passes on.
            Real nx = s.fl(0x10);
            nx = -nx;
            s.sw(0x1e0, packed);
            s.fs(0x210, float(nx));
            s.sw(0x1e4, packed);
            Real ny = s.fl(0x14);
            s.sw(0x1e8, packed);
            ny = -ny;
            s.sw(0x1ec, packed);
            Real ny2 = ny;
            Real nz = s.fl(0x18);
            nz = -nz;
            s.fs(0x78, float(nz));
            Real a = s.fl(0x210);
            a -= s.fl(0x50);
            s.fs(0x40, float(a));
            ny2 -= s.fl(0x54);
            s.fs(0x44, float(ny2));
            nz -= s.fl(0x58);
            Real b = s.fl(0x40);
            b += s.fl(0x30);
            Real c = s.fl(0x44);
            c += s.fl(0x34);
            Real zc = nz;
            zc += s.fl(0x38);
            s.fs(0x258, float(zc));
            s.fs(0x180, float(b));
            s.sw(0x188, s.w(0x258));
            s.fs(0x184, float(c));

            Real d = s.fl(0x10);
            d -= s.fl(0x50);
            Real e = s.fl(0x14);
            e -= s.fl(0x54);
            s.fs(0x154, float(e));
            Real g = s.fl(0x18);
            g -= s.fl(0x58);
            s.fs(0x158, float(g));
            d += s.fl(0x30);
            Real e2 = s.fl(0x154);
            e2 += s.fl(0x34);
            Real g2 = s.fl(0x158);
            g2 += s.fl(0x38);
            s.fs(0x228, float(g2));
            s.fs(0x190, float(d));
            s.sw(0x198, s.w(0x228));
            s.fs(0x194, float(e2));

            Real h = s.fl(0x50);
            h += s.fl(0x10);
            Real i = s.fl(0x54);
            i += s.fl(0x14);
            s.fs(0xd4, float(i));
            Real j = s.fl(0x58);
            j += s.fl(0x18);
            s.fs(0xd8, float(j));
            h += s.fl(0x30);
            Real i2 = s.fl(0xd4);
            i2 += s.fl(0x34);
            Real j2 = s.fl(0xd8);
            j2 += s.fl(0x38);
            s.fs(0x208, float(j2));
            s.fs(0x1a0, float(h));
            s.fs(0x1a4, float(i2));
            s.fs(0x170, float(nx));
            Real k = s.fl(0x78);
            s.sw(0x1a8, s.w(0x208));
            Real mm = s.fl(0x170);
            mm += s.fl(0x50);
            s.fs(0xb0, float(mm));
            Real n = ny;
            n += s.fl(0x54);
            s.fs(0xb4, float(n));
            k = late(k);
            k += s.fl(0x58);
            Real p1 = s.fl(0xb0);
            p1 += s.fl(0x30);
            Real p2 = s.fl(0xb4);
            p2 += s.fl(0x34);
            k += s.fl(0x38);
            s.fs(0x98, float(k));
            s.fs(0x1b0, float(p1));
            s.sw(0x1b8, s.w(0x98));
            s.fs(0x1b4, float(p2));
            s.sw(-4, 0);
            s.sw(-8, 0);
            s.sw(-12, 0x872e90);
            s.sw(-16, ebx);
            s.sw(-20, f + 0x180);
            cpu.esi = esi;
            cpu.ecx = self;
            cpu.esp = f - 24;
            RUN(sub_5c7e80(app, cpu));
        }
    }
    return Drawn;
}

// After the prologue; Terminated when a callee ended the thread.
[[gnu::always_inline]] inline int run(win32::WinApplication* app, x86::CPU& cpu, const reg32 f, const reg32 entry)
{
    const Slots s{rc::host(f)};
    const reg32 bp = entry - 4;
    const reg32 self = rc::ld32(entry + 4);
    const reg32 light = rc::ld32(entry + 8);
    const reg32 m = rc::ld32(entry + 12);
    const reg32 scaleSlot = entry + 16;
    const reg32 kind = rc::ld32(entry + 24);
    reg32 ebx = cpu.ebx, esi = cpu.esi, edi = cpu.edi;
    const auto cold = [&]() __attribute__((always_inline)) { return Frame{app, cpu, s.h, f, f, ebx, esi, edi, bp}; };

    if (rc::ld32(0x7f3d40) == 0)
        return Ended;
    if (kind == 2)
    {
        const reg32 context = rc::ld32(self + 0x64);
        const Real w = ld(context + 0x509c);
        Real p = ld(0x7f39a8);
        p -= ld(0x7f39a4);
        p *= w;
        p += ld(0x7f39a4);
        rc::stf32(Fade, float(p));
    }
    ebx = light;
    esi = rc::ld32(self + 0x40);
    const reg32 view = esi;
    if (light == 0)
        return Ended;
    edi = m;
    if (m == 0 || view == 0)
        return Ended;

    const Real lightScale = ld(entry + 28);
    const reg32 byteA = reg32(x86::sreg8(rc::ld8(light + 0x2c)));
    const reg32 byteB = reg32(x86::sreg8(rc::ld8(light + 0x2d)));
    s.sw(0xcc, byteB);
    if (double(lightScale) == double(ld(Zero)))
    {
        if (rc::ld32(light + 0x1c) == 0x4479c000 || rc::ld32(entry + 20) == 1) [[unlikely]]
        {
            Frame fr = cold();
            if (!probe(fr, light, m, cpu.fpu.count))
                return Terminated;
        }
    }
    else
        rc::st32(light + 0x1c, rc::ld32(entry + 28));

    ebx = light + 0x10;
    s.sw(0x18, rc::ld32(light + 0x18));
    s.sw(0x10, rc::ld32(light + 0x10));
    s.sw(0x14, rc::ld32(light + 0x14));
    cpu.esi = esi;
    cpu.edi = edi;
    s.sw(0x68, 0);
    s.sw(0x6c, 0);
    s.sw(0x70, 0);
    s.sw(0x30, 0);
    s.sw(0x34, 0);
    s.sw(0x38, 0);

    const auto transformed = [&]() __attribute__((always_inline)) {
        cpu.ebp = bp;
        float x, y, z;
        through(m, f + 0x10, x, y, z);
        s.fs(0x68, x);
        s.fs(0x6c, y);
        s.fs(0x70, z);
    };
    const auto box = [&](Real radius) __attribute__((always_inline)) {
        const Real cx = s.fl(0x68), cy = s.fl(0x6c), cz = s.fl(0x70);
        const float minX = float(cx - radius), minY = float(cy - radius), minZ = float(cz - radius);
        const float maxX = float(radius + cx), maxY = float(radius + cy), maxZ = float(radius + cz);
        s.fs(0xd0, Real(minX) < cx ? minX : float(cx));
        s.fs(0xd4, Real(minY) < cy ? minY : float(cy));
        s.fs(0xd8, Real(minZ) < cz ? minZ : float(cz));
        s.fs(0xb0, Real(maxX) > cx ? maxX : float(cx));
        s.fs(0xb4, Real(maxY) > cy ? maxY : float(cy));
        s.fs(0xb8, Real(maxZ) > cz ? maxZ : float(cz));
    };
    const auto occlusion = [&]() __attribute__((always_inline)) {
        s.sw(-4, 0);
        s.sw(-8, f + 0xb0);
        s.sw(-12, f + 0xd0);
        cpu.ecx = self;
        cpu.esp = f - 16;
        sub_5bc300(app, cpu);
        return cpu.eax;
    };

    if (kind == 2)
    {
        ebx = light;
        if (rc::ld32(light + 0x1c) == 0x4479c000 || rc::ld32(light + 0x1c) == 0x461c3c00)
            return Ended;
        transformed();
        s.sw(0x70, rc::ld32(light + 0x1c));
        s.sw(0x78, f - 16);
        box(ld(Fade));
        edi = self;
        cpu.ebx = ebx;
        cpu.edi = edi;
        reg32 visible = 0;
        RUN(visible = occlusion());
        if (visible == 0)
            return Ended;
        ebx = light + 0x20;
        s.sw(0x30, rc::ld32(light + 0x20));
        s.sw(0x34, rc::ld32(light + 0x24));
        s.sw(0x178, rc::ld32(light + 0x28));
        s.fs(0x38, float(-s.fl(0x178)));
    }
    else if (kind == 1)
    {
        transformed();
        box(ld(0x7f3d4c));
        cpu.ebx = ebx;
        reg32 visible = 0;
        RUN(visible = occlusion());
        if (visible == 0)
            return Ended;
    }
    else
    {
        if ((kind == 3 || kind == 4) && byteA != 0xd && byteA != 0xe) [[unlikely]]
        {
            Frame fr = cold();
            bool ended = false;
            if (!facing(fr, kind, scaleSlot, ended))
                return Terminated;
            if (ended)
                return Ended;
        }
        transformed();
        s.sw(-4, 0);
        s.sw(-8, f + 0x68);
        cpu.ecx = self;
        cpu.esp = f - 12;
        RUN(sub_5bc450(app, cpu));
        if (cpu.eax == 0)
            return Ended;
        const reg32 source = rc::ld32(entry + 8) + 0x20;
        s.sw(0x30, rc::ld32(source));
        s.sw(0x34, rc::ld32(source + 4));
        s.sw(0x38, rc::ld32(source + 8));
    }
    return tail(app, cpu, f, entry, ebx, byteA << 8 | (byteB & 0xff));
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "light_flare_native")) [[unlikely]]
    {
        sub_490920_guest(app, cpu);
        return;
    }
    const reg32 entry = cpu.esp;
    const reg32 bp = entry - 4;
    const reg32 rec = (bp & ~15u) - 12;
    const reg32 f = rec - 0x614;
    rc::st32(bp, cpu.ebp);
    rc::st32(rec + 8, 0xffffffffu);
    rc::st32(rec + 4, 0x7727abu);
    rc::st32(rec, rc::ld32(cpu.efs));
    rc::st32(cpu.efs, rec);
    rc::st32(f + 8, cpu.ebx);
    rc::st32(f + 4, cpu.esi);
    rc::st32(f, cpu.edi);

    const int result = run(app, cpu, f, entry);
    if (result == Terminated) [[unlikely]]
        return;
    const reg32 prev = rc::ld32(rec);
    rc::st32(cpu.efs, prev);
    cpu.ecx = prev;
    cpu.eax = reg32(result);
    cpu.ebx = rc::ld32(f + 8);
    cpu.esi = rc::ld32(f + 4);
    cpu.edi = rc::ld32(f);
    cpu.ebp = rc::ld32(bp);
    cpu.esp = entry + 4;
}

// Lasting outputs and the frame minus helper temporaries.
void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    static const x86::reg16 ranges[][2] = {{0, 0x4c}, {0x50, 0x14}, {0x68, 0x14}, {0x80, 0xc}, {0xa0, 0x1c},
                                           {0xc0, 0xb0}, {0x178, 4}, {0x180, 0xf8}, {0x2f8, 0x338}};
    const x86::reg32 f = ((cpu.esp - 4) & ~15u) - 0x620;
    const x86::reg32 light = argument(cpu, 1);
    if (light)
        spans.push_back(shadow::Span{light + 0x1c, 4, false});
    spans.push_back(shadow::Span{cpu.esp + 16, 4, false});
    spans.push_back(shadow::Span{Alpha, 4, false});
    spans.push_back(shadow::Span{Fade, 4, false});
    spans.push_back(shadow::Span{0x82db00, 4, false});
    for (const auto& range : ranges)
        spans.push_back(shadow::Span{f + range[0], range[1], false});
}

const shadow::Spec spec = {"sub_490920", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, true};

}

void sub_490920(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_490920_guest, declare);
}

}}
