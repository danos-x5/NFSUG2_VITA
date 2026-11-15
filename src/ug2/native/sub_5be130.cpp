// Native sub_5be130: appends a point (3 floats at the argument) to the shadow outline buffer (cdecl).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_5be130_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 Buffer = 0x870958;
const x86::reg32 Count = 0x870964;

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "shadow_points_native")) [[unlikely]]
    {
        sub_5be130_guest(app, cpu);
        return;
    }
    const x86::reg32 point = argument(cpu, 0);
    const x86::reg32 count = rc::ld32(Count);
    const x86::reg32 target = rc::ld32(Buffer) + count * 24 + 4;
    if (target - (Buffer - 11) >= 27 && target - point + 11 >= 23) [[likely]]
    {
        const x86::reg32 x = rc::ld32(point), y = rc::ld32(point + 4), z = rc::ld32(point + 8);
        rc::st32(target, x);
        rc::st32(target + 4, y);
        rc::st32(target + 8, z);
        rc::st32(Count, count + 1);
    }
    else
    {
        rc::st32(target, rc::ld32(point));
        rc::st32(rc::ld32(Buffer) + rc::ld32(Count) * 24 + 8, rc::ld32(point + 4));
        rc::st32(rc::ld32(Buffer) + rc::ld32(Count) * 24 + 12, rc::ld32(point + 8));
        rc::st32(Count, rc::ld32(Count) + 1);
    }
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    spans.push_back(shadow::Span{Count, 4, false});
    spans.push_back(shadow::Span{rc::ld32(Buffer) + rc::ld32(Count) * 24 + 4, 12, false});
}

const shadow::Spec spec = {"sub_5be130", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5be130(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5be130_guest, declare);
}

}}
