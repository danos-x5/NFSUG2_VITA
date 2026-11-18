// Native drivetrain of the car physics, exact to the guest: engine torque from curve tables (0x594300), wheel load
// ratio (0x594840), torque split (0x594a30) and their combination (0x594da0).

#include <native/check.h>
#include <native/common.h>
#include <native/physics.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_594300_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_594840_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_594a30_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_594da0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;

using physics::Registers;
using physics::lookup;

inline float speedArgument(reg32 w)
{
    Real a = ld(w + 0x14);
    a *= ld(0x7a1d28);
    return float(a);
}

inline Real throttle(reg32 w, bool flag)
{
    if (flag)
        return ld(0x7d7d3c);
    Real f = ld(w + 0x80);
    f *= ld(w + 0x78);
    return f;
}

// sub_594300 (thiscall on the engine): torque at the current speed.
Real torque(x86::CPU& cpu, reg32 w, Registers& out)
{
    Real l1 = lookup(w + 0x24, speedArgument(w), out);
    l1 *= ld(w + 0x90);
    if (rc::ld32(w + 0x74) != 0)
        l1 *= ld(0x7fbff4);
    const bool flag = rc::ld8(w + 0x88) != 0;
    Real acc = l1;
    acc *= throttle(w, flag);
    float total = float(acc);
    const reg32 n = rc::ld32(w + 0x84);
    if (x86::sreg32(n) > 0)
    {
        const float gear = rc::ldf32(n * 4 + 0x7a1798);
        const reg32 e = rc::ld32(rc::ld32(w + 0x1c));
        Real l2 = lookup(w + 0x60, speedArgument(w), out);
        l2 *= ld(e + 0x2ac);
        l2 *= Real(gear);
        Real f = throttle(w, flag);
        f *= l2;
        f += Real(total);
        total = float(f);
    }
    const Real c = ld(w + 0x8c);
    if (c > ld(0x7fbff0))
    {
        Real d = ld(0x784250);
        d -= ld(0x7fbff0);
        Real x = ld(w + 0x8c);
        if (ld(0x784250) < x)
            x = ld(0x784250);
        x -= ld(0x7fbff0);
        x /= d;
        x *= ld(0x7a1914);
        x = ld(0x784250) - x;
        x *= Real(total);
        total = float(x);
    }
    const float idle = flag ? 0.0f : float(throttle(w, false));
    Real l3 = lookup(w + 0x38, speedArgument(w), out);
    Real m = ld(0x784250);
    m -= Real(idle);
    l3 *= m;
    l3 = -l3;
    if (flag)
        l3 *= ld(0x7a18d8);
    l3 += Real(total);
    out.ecx = (out.ecx & 0xffffff00u) | rc::ld8(w + 0x88);
    cpu.fpu.compare(l3, ld(0x7d7d3c));
    out.eax = (out.eax & 0xffff0000u) | cpu.fpu.status.word;
    if (l3 < ld(0x7d7d3c) && !flag)
    {
        out.edx = rc::ld32(w + 0x20);
        l3 *= ld(rc::ld32(out.edx + 0x3c) == 1 ? 0x7a1918 : 0x7a191c);
    }
    return l3;
}

inline Real wheelTerm(reg32 wheel)
{
    Real f = ld(rc::ld32(wheel + 0x14) + 0x10);
    f *= ld(wheel + 0x124);
    f = ld(wheel + 0x108) - f;
    f /= ld(wheel + 0x2c);
    return f;
}

// sub_594840 (thiscall): load ratio of the driven wheels.
Real loadRatio(reg32 self)
{
    Real r = ld(0x7d7d3c);
    const reg32 s = rc::ld32(self + 0x18);
    if (ld(s + 0x10) >= ld(0x78435c))
    {
        Real a = wheelTerm(rc::ld32(self + 0x30));
        a += wheelTerm(rc::ld32(self + 0x2c));
        a *= ld(0x784260);
        a *= ld(s + 0xc);
        r = a;
    }
    if (ld(s + 0x10) <= ld(0x79b158))
    {
        Real a = wheelTerm(rc::ld32(self + 0x28));
        a += wheelTerm(rc::ld32(self + 0x24));
        a *= ld(0x784260);
        a *= ld(s + 8);
        r += a;
        if (ld(s + 0x10) >= ld(0x78435c))
            r *= ld(0x784260);
    }
    r *= ld(s + rc::ld32(self + 0x3c) * 4 + 0x20);
    return r;
}

// One axle's torque share; writes the split to slot.
inline Real axleShare(reg32 self, reg32 left, reg32 right, reg32 pair, bool firstRight, reg32 slot)
{
    const bool locked = rc::ld8(left + 0x11c) != 0 && rc::ld8(right + 0x11c) != 0;
    const reg32 split = locked ? rc::ld32(0x7fbffc) : 0x3f000000u;
    rc::st32(slot, split);
    const float s = rc::ldf32(slot);
    const reg32 mode = rc::ld32(rc::ld32(self + 0x34) + 0x42c);
    if (mode == 1)
    {
        Real f = ld(0x784250);
        f -= Real(s);
        return f;
    }
    if (mode == 2)
        return Real(s);
    Real sum = firstRight ? ld(left + 0x28) : ld(right + 0x28);
    sum += firstRight ? ld(right + 0x28) : ld(left + 0x28);
    Real a = sum;
    a = Real(__builtin_fabsf(float(a)));
    if (!(a > ld(0x7845a0)))
        return ld(0x784260);
    Real f = ld(pair + 0x28) / sum;
    if (f < Real(s))
    {
        Real g = ld(0x7fbffc);
        g *= ld(0x784260);
        return ld(0x784250) - g;
    }
    Real t = ld(0x784250);
    t -= Real(s);
    if (f > t)
    {
        Real g = ld(0x7fbffc);
        g *= ld(0x784260);
        return g;
    }
    return f;
}

// sub_594a30 (thiscall, ret 4): splits torque over four wheels.
void split(reg32 self, reg32 E, reg32& eax, reg32& edx)
{
    if (rc::ld32(self + 0x3c) == 1)
        return;
    const reg32 slot = E + 4, temp = E - 4;
    const float T = rc::ldf32(slot);
    {
        Real a = Real(T);
        a += ld(self + 0x70);
        rc::stf32(self + 0x70, float(a));
    }
    Real lim = Real(x86::sreg32(rc::ld32(rc::ld32(self + 0x34) + 0x3e4)));
    lim *= ld(0x784d28);
    if (ld(0x784d28) > lim)
        lim = ld(0x784d28);
    lim *= Real(T);
    {
        const reg32 e = rc::ld32(self + 0x20);
        rc::stf32(e + 0x7c, float(ld(e + 0x7c) - lim));
    }
    const reg32 s = rc::ld32(self + 0x18);
    Real tg = Real(T);
    tg *= ld(s + rc::ld32(self + 0x3c) * 4 + 0x20);
    Real a = ld(0x784250);
    a -= ld(self + 0xec);
    a *= ld(s + 8);
    a *= tg;
    rc::stf32(temp, float(a));
    Real b = ld(s + 0xc);
    b *= ld(self + 0xec);
    tg *= b;
    const reg32 w24 = rc::ld32(self + 0x24), w28 = rc::ld32(self + 0x28);
    Real f = axleShare(self, w24, w28, w28, true, slot);
    if (rc::ld32(w24 + 0x10c) == 0)
    {
        Real v = ld(0x784250);
        v -= f;
        v *= ld(temp);
        v += ld(w24 + 0x108);
        rc::stf32(w24 + 0x108, float(v));
    }
    if (rc::ld32(w28 + 0x10c) == 0)
    {
        Real v = f;
        v *= ld(temp);
        v += ld(w28 + 0x108);
        rc::stf32(w28 + 0x108, float(v));
    }
    const reg32 w2c = rc::ld32(self + 0x2c), w30 = rc::ld32(self + 0x30);
    f = axleShare(self, w2c, w30, w2c, false, slot);
    if (rc::ld32(w2c + 0x10c) == 0)
    {
        Real v = f;
        v *= tg;
        v += ld(w2c + 0x108);
        rc::stf32(w2c + 0x108, float(v));
    }
    eax = rc::ld32(w30 + 0x10c);
    if (eax == 0)
    {
        Real v = ld(0x784250);
        v -= f;
        v *= tg;
        v += ld(w30 + 0x108);
        rc::stf32(w30 + 0x108, float(v));
    }
    edx = w2c;
}

// sub_594da0 (thiscall): gearbox torque to the wheels.
void drive(x86::CPU& cpu, reg32 self, reg32 E)
{
    if (rc::ld32(self + 0x44) != 3)
        return;
    const reg32 w = rc::ld32(self + 0x20);
    Registers inner;
    Real t = torque(cpu, w, inner);
    t += ld(w + 0x7c);
    Real d = ld(rc::ld32(w + 0x18) + 0xc);
    const reg32 m = rc::ld32(rc::ld32(w + 0x20) + 0x44);
    if (m == 2 || m == 0)
        d *= ld(0x7a1920);
    t /= d;
    const float q = float(t);
    rc::stf32(E - 4, q);
    Real v = Real(q) - loadRatio(self);
    Real inv = ld(0x784250);
    inv /= ld(rc::ld32(rc::ld32(self + 0x20) + 0x18) + 0xc);
    const reg32 s = rc::ld32(self + 0x18);
    reg32 eax = rc::ld32(rc::ld32(self + 0x20) + 0x18), edx = rc::ld32(self + 0x20);
    if (ld(s + 0x10) <= ld(0x79b158))
    {
        edx = rc::ld32(self + 0x3c);
        eax = rc::ld32(self + 0x24);
        Real g = ld(s + rc::ld32(self + 0x3c) * 4 + 0x20);
        g *= ld(s + 8);
        Real k = ld(0x784250);
        k -= ld(self + 0xec);
        k /= ld(rc::ld32(self + 0x24) + 0x2c);
        k *= g;
        k *= g;
        k *= ld(0x784260);
        inv += k;
    }
    cpu.fpu.compare(ld(s + 0x10), ld(0x78435c));
    eax = (eax & 0xffff0000u) | cpu.fpu.status.word;
    if (ld(s + 0x10) >= ld(0x78435c))
    {
        edx = rc::ld32(self + 0x3c);
        eax = rc::ld32(self + 0x2c);
        Real g = ld(s + rc::ld32(self + 0x3c) * 4 + 0x20);
        g *= ld(s + 0xc);
        Real k = ld(self + 0xec);
        k /= ld(rc::ld32(self + 0x2c) + 0x2c);
        k *= g;
        k *= g;
        k *= ld(0x784260);
        inv += k;
    }
    v /= inv;
    const reg32 frame = E - 16;
    rc::stf32(frame + 4, float(v));
    split(self, frame, eax, edx);
    cpu.eax = eax;
    cpu.edx = edx;
}

// Wrappers: registers as the guest leaves them for its callers.
void torqueBody(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("drivetrain_native"))
    {
        sub_594300_guest(app, cpu);
        return;
    }
    Registers out = {cpu.eax, cpu.ecx, cpu.edx};
    const Real r = torque(cpu, cpu.ecx, out);
    cpu.eax = out.eax;
    cpu.ecx = out.ecx;
    cpu.edx = out.edx;
    cpu.fpu.count += 1;
    cpu.fpu.regs[cpu.fpu.count & 7] = r;
    cpu.esp += 4;
}

void ratioBody(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("drivetrain_native"))
    {
        sub_594840_guest(app, cpu);
        return;
    }
    const Real r = loadRatio(cpu.ecx);
    cpu.fpu.count += 1;
    cpu.fpu.regs[cpu.fpu.count & 7] = r;
    cpu.esp += 4;
}

void splitBody(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("drivetrain_native"))
    {
        sub_594a30_guest(app, cpu);
        return;
    }
    reg32 eax = cpu.eax, edx = cpu.edx;
    split(cpu.ecx, cpu.esp, eax, edx);
    cpu.eax = eax;
    cpu.edx = edx;
    cpu.esp += 8;
}

void driveBody(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("drivetrain_native"))
    {
        sub_594da0_guest(app, cpu);
        return;
    }
    drive(cpu, cpu.ecx, cpu.esp);
    cpu.esp += 4;
}

void declareNone(x86::CPU&, shadow::Spans&) {}

// The wheels' and engine's accumulators that sub_594a30 updates.
void declareSplit(reg32 self, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{self + 0x70, 4, false});
    spans.push_back(shadow::Span{rc::ld32(self + 0x20) + 0x7c, 4, false});
    for (reg32 o = 0x24; o <= 0x30; o += 4)
        spans.push_back(shadow::Span{rc::ld32(self + o) + 0x108, 4, false});
}

void declareA30(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{cpu.esp + 4, 4, false});
    declareSplit(cpu.ecx, spans);
}

void declareDa0(x86::CPU& cpu, shadow::Spans& spans)
{
    declareSplit(cpu.ecx, spans);
}

const x86::reg8 kept = shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi;
const x86::reg8 withAll = kept | shadow::Eax | shadow::Ecx | shadow::Edx;
const x86::reg8 withAd = kept | shadow::Eax | shadow::Edx;
const shadow::Spec spec300 = {"sub_594300", withAll, 0, true, false};
const shadow::Spec spec840 = {"sub_594840", kept, 0, true, false};
const shadow::Spec specA30 = {"sub_594a30", withAd, 0, false, false};
const shadow::Spec specDa0 = {"sub_594da0", withAd, 0, false, false};
check::Stats s300 = {"sub_594300"}, s840 = {"sub_594840"}, sA30 = {"sub_594a30"}, sDa0 = {"sub_594da0"};

}

void sub_594300(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s300, withAll, app, cpu, torqueBody, sub_594300_guest, declareNone);
    else
        shadow::call(spec300, app, cpu, torqueBody, sub_594300_guest, declareNone);
}

void sub_594840(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s840, kept, app, cpu, ratioBody, sub_594840_guest, declareNone);
    else
        shadow::call(spec840, app, cpu, ratioBody, sub_594840_guest, declareNone);
}

void sub_594a30(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(sA30, withAd, app, cpu, splitBody, sub_594a30_guest, declareA30);
    else
        shadow::call(specA30, app, cpu, splitBody, sub_594a30_guest, declareA30);
}

void sub_594da0(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(sDa0, withAd, app, cpu, driveBody, sub_594da0_guest, declareDa0);
    else
        shadow::call(specDa0, app, cpu, driveBody, sub_594da0_guest, declareDa0);
}

}}
