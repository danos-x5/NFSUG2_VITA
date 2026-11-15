// Native sub_5d3330: point against up to four quad edge cross products, running maximum (thiscall, ret 4).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5d3330_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// Stores cross product in argument slot; returns it.
Real cross(x86::reg32 quad, x86::reg32 point, x86::reg32 slot, x86::reg32 a, x86::reg32 b, x86::reg32 c, x86::reg32 d)
{
    Real t = ld(quad + a) - ld(quad + b);
    t *= ld(point) - ld(quad + c);
    Real u = ld(point + 4) - ld(quad + b);
    u *= ld(quad + d) - ld(quad + c);
    t -= u;
    rc::stf32(slot + 4, float(t));
    return ld(slot + 4);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("point_in_quad_native", true); }))
    {
        sub_5d3330_guest(app, cpu);
        return;
    }
    const x86::reg32 quad = cpu.ecx;
    const x86::reg32 slot = cpu.esp;
    const x86::reg32 point = rc::ld32(slot + 4);
    Real fp = ld(0x7842a0);

    // Only the last fcom's status word survives, so compare once on exit.
    const auto leave = [&](const Real& a, const Real& b) {
        cpu.fpu.compare(a, b);
        cpu.eax = (cpu.eax & 0xffff0000u) | x86::reg16(cpu.fpu.status.word);
        cpu.edx = point;
        const x86::reg32 count = cpu.fpu.count + 1;
        cpu.fpu.regs[count & 7] = fp;
        cpu.fpu.count = count;
    };

    Real v = cross(quad, point, slot, 0x44, 0x34, 0x30, 0x40);
    Real limit;
    if (!(fp > v))
    {
        fp = v;
        limit = ld(0x7d7d3c);
        if (fp > limit)
            return leave(fp, limit);
    }
    v = cross(quad, point, slot, 0x54, 0x44, 0x40, 0x50);
    if (!(fp > v))
        fp = v;
    limit = ld(0x7d7d3c);
    if (fp > limit)
        return leave(fp, limit);
    v = cross(quad, point, slot, 0x34, 0x64, 0x60, 0x30);
    if (!(fp > v))
        fp = v;
    limit = ld(0x7d7d3c);
    if (fp > limit || rc::ld8(quad + 9) != 4)
        return leave(fp, limit);
    v = cross(quad, point, slot, 0x64, 0x54, 0x50, 0x60);
    const Real last = fp;
    if (!(fp > v))
        fp = v;
    leave(last, v);
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back({cpu.esp + 4, 4, false});
}

const shadow::Spec spec = {"sub_5d3330", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, true, false};

}

void sub_5d3330(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5d3330_guest, declare);
}

}}
