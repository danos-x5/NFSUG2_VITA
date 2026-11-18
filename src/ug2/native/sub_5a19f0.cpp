// Native sub_5a19f0: bounding box of a collision shape's vertices, optionally offset (thiscall, ret 8: min, max).
// Inlines sub_5a1970, sub_43d540 and sub_43d570; exact to the guest.

#include <native/check.h>
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_5a19f0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

struct Point
{
    float x, y, z;
};

inline Point vertex(x86::reg32 records, x86::reg32 index, bool offset, Real ox, Real oy, Real oz)
{
    const x86::reg32 r = records + index * 0x30;
    Point p = {float(ld(r + 0x10)), float(ld(r + 0x14)), float(ld(r + 0x18))};
    if (offset)
    {
        p.x = float(Real(p.x) - ox);
        p.y = float(Real(p.y) - oy);
        p.z = float(Real(p.z) - oz);
    }
    return p;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("shape_bounds_native"))
    {
        sub_5a19f0_guest(app, cpu);
        return;
    }
    const x86::reg32 self = cpu.ecx;
    const x86::reg32 low = argument(cpu, 0), high = argument(cpu, 1);
    const x86::reg32 records = rc::ld32(self + 0x54);
    const bool offset = rc::ld8(self + 0x3e) != 0;
    const Real ox = offset ? ld(self + 0x40) : Real(0);
    const Real oy = offset ? ld(self + 0x44) : Real(0);
    const Real oz = offset ? ld(self + 0x48) : Real(0);
    Point p = vertex(records, 0, offset, ox, oy, oz);
    rc::stf32(low, float(Real(p.x)));
    rc::stf32(low + 4, float(Real(p.y)));
    rc::stf32(low + 8, float(Real(p.z)));
    rc::stf32(high, float(Real(p.x)));
    rc::stf32(high + 4, float(Real(p.y)));
    rc::stf32(high + 8, float(Real(p.z)));
    x86::reg32 ecx = ((cpu.esp - 4) & ~0xfu) - 0x20, edx = low;
    int32_t count = int32_t(rc::ld32(self + 0x14));
    for (int32_t i = 1; i < count; count = int32_t(rc::ld32(self + 0x14)), ++i)
    {
        p = vertex(records, x86::reg32(i), offset, ox, oy, oz);
        if (Real(p.x) < ld(low))
            rc::stf32(low, p.x);
        if (Real(p.y) < ld(low + 4))
            rc::stf32(low + 4, p.y);
        if (Real(p.z) < ld(low + 8))
            rc::stf32(low + 8, p.z);
        if (Real(p.x) > ld(high))
            rc::stf32(high, p.x);
        if (Real(p.y) > ld(high + 4))
            rc::stf32(high + 4, p.y);
        if (Real(p.z) > ld(high + 8))
            rc::stf32(high + 8, p.z);
        ecx = edx = bits(p.z);
    }
    cpu.eax = x86::reg32(count);
    cpu.ecx = ecx;
    cpu.edx = edx;
    cpu.esp += 12;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 12, false});
    spans.push_back(shadow::Span{argument(cpu, 1), 12, false});
}

const x86::reg8 compared = shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp |
                           shadow::Esi | shadow::Edi;
const shadow::Spec spec = {"sub_5a19f0", compared, 0, false, false};
check::Stats s_check = {"sub_5a19f0"};

}

void sub_5a19f0(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, compared, app, cpu, body, sub_5a19f0_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_5a19f0_guest, declare);
}

}}
