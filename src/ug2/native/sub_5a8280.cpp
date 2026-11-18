// Native sub_5a8280: one wheel's contact forces (thiscall, ret 8), with its helpers sub_43c9d0, sub_441000, sub_43c570,
// sub_59d490, sub_59d400 and sub_59d510 inlined (native/physics.h). Exact to the guest.

#include <native/check.h>
#include <native/common.h>
#include <native/physics.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_5a8280_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;
using physics::lookup;

inline Real one() { return ld(0x784250); }
inline Real zero() { return ld(0x7d7d3c); }

// sub_59d400: grip test; sets the wheel's slipping state.
void gripTest(x86::CPU& cpu, reg32 w, float q)
{
    const reg32 e = rc::ld32(w + 0x1c);
    Real h = ld(e + 8);
    h *= ld(w + 0x17c);
    h *= ld(0x7fc70c);
    Real g = ld(e + 4);
    g *= ld(w + 0x178);
    g *= ld(0x7fc708);
    h += g;
    h *= ld(0x7fc710);
    Real u = cpu.fpu.abs(ld(w + 0x28));
    u *= ld(0x7fc714);
    Real v = ld(rc::ld32(w + 0x14) + 0x10);
    v *= Real(q);
    u += v;
    if (!(h > u))
    {
        rc::st32(w + 0x10c, 0);
        return;
    }
    rc::st32(w + 0x10c, h > one() ? 1u : 0u);
    rc::st32(w + 0x28, 0);
}

// sub_59d510: sliding force from the friction curves.
Real slideForce(reg32 w, float a0, float a1)
{
    if (rc::ld32(0x7fc7f0) == 0)
    {
        Real r = lookup(0x7fc954, a1);
        r *= ld(w + 0x164);
        r *= ld(0x7fc718);
        r *= ld(0x7fc740);
        return r;
    }
    const reg32 d = rc::ld32(w + 0x18);
    Real x = Real(a1);
    x *= ld(0x78424c);
    x /= ld(d);
    x *= ld(0x78821c);
    x *= ld(0x784260);
    const float xf = float(x);
    const reg32 idx = reg32(physics::ftol(x));
    Real t = ld(0x7fc7f8);
    t *= Real(a0);
    t /= ld(d + 4);
    const float tf = float(t);
    if (!(x86::sreg32(idx) < 6))
    {
        Real r = lookup(rc::ld32(0x7fc7ec), tf);
        r *= ld(rc::ld32(w + 0x18) + 8);
        r *= ld(0x7fc7f4);
        return r;
    }
    const float l1 = float(lookup(rc::ld32(idx * 4 + 0x7fc7d4), tf));
    Real d2 = lookup(rc::ld32(idx * 4 + 0x7fc7d8), tf);
    Real frac = Real(xf) - Real(x86::sreg32(idx));
    d2 -= Real(l1);
    frac *= d2;
    frac += Real(l1);
    frac *= ld(rc::ld32(w + 0x18) + 8);
    frac *= ld(0x7fc7f4);
    return frac;
}

void wheel(x86::CPU& cpu, reg32 w, reg32 step)
{
    const reg32 e = rc::ld32(w + 0x1c);
    Real lon = ld(e + 4);
    lon *= ld(w + 0x178);
    lon *= ld(0x7fc708);
    Real lat = ld(e + 8);
    lat *= ld(w + 0x17c);
    lat *= ld(0x7fc70c);
    const float latf = float(lat);
    if (ld(w + 0x28) > zero())
        lon = -lon;
    const reg32 locked = rc::ld32(w + 0x10c);
    if (locked == 0)
    {
        lon += ld(w + 0x108);
        rc::stf32(w + 0x108, float(lon));
    }
    Real lat2 = Real(latf);
    if (ld(w + 0x28) > zero())
        lat2 = -lat2;
    if (locked == 0)
    {
        lat2 += ld(w + 0x108);
        rc::stf32(w + 0x108, float(lat2));
    }

    const physics::Vec3 v = physics::transformTransposed(w + 0x70, physics::load3(w + 0xf0));
    const float vx = v.x, vy = v.y;
    const float af = physics::slipAngle(cpu, v);

    Real slip = ld(rc::ld32(w + 0x14) + 0x10);
    slip *= ld(w + 0x28);
    slip -= Real(vx);
    rc::stf32(w + 0x118, float(slip));
    rc::stf32(w + 0x16c, float(slip));
    if (-ld(0x7fc704) > ld(w + 0x118) && ld(w + 0x108) < zero() && Real(latf) == zero() && step == 0)
    {
        Real f = -ld(w + 0x118);
        f = ld(0x7fc704) / f;
        f *= ld(w + 0x108);
        rc::stf32(w + 0x108, float(f));
    }

    const float vy2 = float(Real(vy) * Real(vy));
    const float slipf = rc::ldf32(w + 0x118);
    Real mag = Real(slipf);
    mag *= Real(slipf);
    mag += Real(vy2);
    mag = cpu.fpu.sqrt(mag);
    const float magf = float(mag);
    bool rolling = mag == zero() || (Real(vy) == zero() && Real(vx) == zero());
    if (!rolling)
    {
        Real load = ld(rc::ld32(w + 0x114) * 0x98u + 0x800f08);
        load *= ld(w + 0x184);
        load *= ld(w + 0x180);
        load *= ld(rc::ld32(w + 0x18) + 0x10);
        float loadf = float(load);
        if (rc::ld32(rc::ld32(w + 0x20) + 0x424) == 1)
        {
            Real r = physics::traction(w);
            r *= Real(loadf);
            loadf = float(r);
        }
        Real q = ld(w + 0x164);
        q /= Real(magf);
        q *= ld(0x7fc718);
        q *= Real(loadf);
        const float qf = float(q);
        Real sp = Real(vx);
        sp *= Real(vx);
        sp += Real(vy2);
        sp = cpu.fpu.sqrt(sp);
        Real g = ld(w + 0x164) / sp;
        g *= ld(0x7fc718);
        g *= Real(loadf);
        g *= cpu.fpu.abs(Real(vx));
        gripTest(cpu, w, float(g));
        if (cpu.fpu.abs(Real(slipf)) > ld(0x7fc704) || cpu.fpu.abs(Real(af)) > ld(rc::ld32(w + 0x18))
            || rc::ld32(w + 0x10c) != 0)
        {
            Real r1 = Real(slipf);
            r1 *= ld(w + 0x190);
            rc::st8(w + 0x160, 1);
            r1 *= Real(qf);
            const float r1f = float(r1);
            Real r2 = Real(qf);
            r2 *= ld(w + 0x194);
            rc::stf32(w + 0x124, r1f);
            r2 *= Real(vy);
            r2 = -r2;
            const float r2f = float(r2);
            rc::stf32(w + 0x128, r2f);
            Real s = ld(rc::ld32(w + 0x20) + 0x3ec);
            s *= ld(0x78426c);
            if (s < one() && Real(loadf) > ld(0x784358))
            {
                const Real inv = one() / Real(loadf);
                rc::stf32(w + 0x124, float(Real(r1f) * inv));
                rc::stf32(w + 0x128, float(inv * Real(r2f)));
            }
            rc::stf32(w + 0x100, slipf);
            rc::stf32(w + 0x104, vy);
            return;
        }
    }
    rc::st8(w + 0x160, 0);
    Real f = ld(w + 0x190) / ld(rc::ld32(w + 0x14) + 0x10);
    f *= ld(w + 0x108);
    rc::stf32(w + 0x124, float(f));
    const Real s18 = Real(vx) > zero() ? Real(1.0f) : Real(-1.0f);
    const Real s1c = Real(af) > zero() ? Real(-1.0f) : Real(1.0f);
    Real a0 = ld(0x7fc718);
    a0 *= ld(w + 0x164);
    Real r = slideForce(w, float(a0), float(cpu.fpu.abs(Real(af))));
    r *= ld(w + 0x194);
    rc::st32(w + 0x100, 0);
    rc::stf32(w + 0x104, vy);
    r *= s1c;
    r *= s18;
    rc::stf32(w + 0x128, float(r));
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("wheel_native"))
    {
        sub_5a8280_guest(app, cpu);
        return;
    }
    wheel(cpu, cpu.ecx, rc::ld32(cpu.esp + 8));
    cpu.esp += 12;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const reg32 w = cpu.ecx;
    spans.push_back(shadow::Span{w + 0x28, 4, false});
    spans.push_back(shadow::Span{w + 0x100, 0x30, false});
    spans.push_back(shadow::Span{w + 0x160, 1, false});
    spans.push_back(shadow::Span{w + 0x16c, 4, false});
}

const x86::reg8 kept = shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi;
const shadow::Spec spec = {"sub_5a8280", kept, 0, false, false};
check::Stats s_check = {"sub_5a8280"};

}

void sub_5a8280(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, kept, app, cpu, body, sub_5a8280_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_5a8280_guest, declare);
}

}}
