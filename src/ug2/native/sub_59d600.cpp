// Native sub_59d600: one wheel's tyre force, clamped to the load and turned into world space (thiscall, ret 4).
// Its helpers are inlined from native/physics.h; exact to the guest.

#include <native/check.h>
#include <native/common.h>
#include <native/physics.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_59d600_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;
using physics::Vec3;

inline Real one() { return ld(0x784250); }
inline Real zero() { return ld(0x7d7d3c); }

// Minus the dot product of r and n, as the guest orders it.
inline float planeOffset(const Vec3& r, const Vec3& n)
{
    Real d = Real(r.z) * Real(n.z);
    d += Real(r.y) * Real(n.y);
    d += Real(r.x) * Real(n.x);
    return float(-d);
}

void tyre(x86::CPU& cpu, reg32 w)
{
    bool scaled = false;
    if (ld(w + 0x108) > zero() && rc::ld32(w + 0x10c) == 0)
    {
        Real f = ld(0x7fc720);
        f *= ld(w + 0x124);
        rc::stf32(w + 0x124, float(f));
        scaled = true;
    }
    Real mag = ld(w + 0x128);
    mag *= ld(w + 0x128);
    Real t = ld(w + 0x124);
    t *= ld(w + 0x124);
    mag += t;
    const float magf = float(cpu.fpu.sqrt(mag));
    Real cap = ld(rc::ld32(w + 0x114) * 0x98u + 0x800f04);
    cap *= ld(rc::ld32(w + 0x18) + 0xc);
    cap *= ld(w + 0x188);
    cap *= ld(w + 0x184);
    cap *= ld(w + 0x180);
    cap *= ld(w + 0x164);
    cap *= ld(0x7fc718);
    float capf = float(cap);
    if (rc::ld32(rc::ld32(w + 0x20) + 0x424) == 1)
    {
        Real r = physics::traction(w);
        r *= Real(capf);
        capf = float(r);
    }
    if (Real(magf) > Real(capf) && Real(magf) > ld(0x78435c))
    {
        Real k = Real(capf);
        k /= Real(magf);
        Real a = k;
        a *= ld(w + 0x124);
        rc::stf32(w + 0x124, float(a));
        Real b = ld(w + 0x128);
        b *= k;
        rc::stf32(w + 0x128, float(b));
    }
    else if (scaled)
        rc::stf32(w + 0x124, float(ld(w + 0x124) / ld(0x7fc720)));

    bool full = true;
    if (Real(capf) > ld(0x78435c))
    {
        Real r = Real(magf);
        r /= Real(capf);
        rc::stf32(w + 0x170, float(r));
        full = r > one();
    }
    if (full)
        rc::st32(w + 0x170, 0x3f800000u);
    if (rc::ld8(w + 0x160) != 0 || rc::ld32(w + 0x10c) != 0)
        rc::st32(w + 0x170, 0x3f800000u);
    if (rc::ld8(w + 0x11c) == 0)
        rc::st32(w + 0x170, 0);

    const Vec3 v = physics::transformTransposed(w + 0x70, physics::load3(w + 0xf0));
    const float a = physics::slipAngle(cpu, v);
    Real c = physics::sine(cpu, a);
    c *= ld(w + 0x128);
    if (cpu.fpu.abs(Real(v.x)) > one())
    {
        c *= ld(0x7844f0);
        rc::stf32(w + 0x124, float(ld(w + 0x124) - c));
    }

    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const reg32 mode = rc::ld32(0x7fc7fc);
    const Vec3 r0 = physics::load3(w + 0x70), r1 = physics::load3(w + 0x80), n = physics::load3(w + 0x150);
    Vec3 row0 = {m[0], m[1], m[2]}, row1 = {m[4], m[5], m[6]}, row2 = {m[8], m[9], m[10]};
    if (mode == 1)
    {
        row0 = r0;
        row1 = r1;
        row2 = physics::load3(w + 0x90);
    }
    else if (mode == 2)
    {
        row0 = physics::addScaled(r0, n, planeOffset(r0, n));
        row1 = physics::addScaled(r1, n, planeOffset(r1, n));
        row2 = n;
    }
    else if (mode == 3)
    {
        row0 = physics::normalise(cpu, physics::addScaled(r0, n, planeOffset(r0, n)));
        row2 = n;
        row1 = physics::cross(row2, row0);
    }
    else if (mode == 4)
    {
        row0 = r0;
        row1 = r1;
        row2 = n;
    }
    m[0] = row0.x, m[1] = row0.y, m[2] = row0.z;
    m[4] = row1.x, m[5] = row1.y, m[6] = row1.z;
    m[8] = row2.x, m[9] = row2.y, m[10] = row2.z;
    const Vec3 f = physics::transform(m, Vec3{rc::ldf32(w + 0x124), rc::ldf32(w + 0x128), rc::ldf32(w + 0x164)});
    rc::stf32(w + 0x130, f.x);
    rc::stf32(w + 0x134, f.y);
    rc::stf32(w + 0x138, f.z);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("wheel_native"))
    {
        sub_59d600_guest(app, cpu);
        return;
    }
    const reg32 E = cpu.esp;
    tyre(cpu, cpu.ecx);
    cpu.eax = cpu.ecx + 0x130;
    cpu.edx = ((E - 4) & ~0xfu) - 0xc0 + 0x70;
    cpu.esp = E + 8;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const reg32 w = cpu.ecx;
    spans.push_back(shadow::Span{w + 0x124, 0x18, false});
    spans.push_back(shadow::Span{w + 0x170, 4, false});
}

const x86::reg8 kept = shadow::Eax | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi;
const shadow::Spec spec = {"sub_59d600", kept, 0, false, false};
check::Stats s_check = {"sub_59d600"};

}

void sub_59d600(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, kept, app, cpu, body, sub_59d600_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_59d600_guest, declare);
}

}}
