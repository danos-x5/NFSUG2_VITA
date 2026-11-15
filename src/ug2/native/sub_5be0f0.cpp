// Native sub_5be0f0: stores two floats into the shadow outline buffer entry for the current index (cdecl).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_5be0f0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 Buffer = 0x870958;
const x86::reg32 Index = 0x870968;

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "shadow_points_native")) [[unlikely]]
    {
        sub_5be0f0_guest(app, cpu);
        return;
    }
    const x86::reg32 index = rc::ld32(Index);
    const x86::reg32 target = rc::ld32(Buffer) + index * 24 + 0x14;
    const x86::reg32 words = cpu.esp + 4;
    if (target - (Buffer - 7) >= 27 && target - words + 7 >= 15) [[likely]]
    {
        const float first = float(ld(words));
        const float second = float(ld(words + 4));
        rc::stf32(target, first);
        rc::stf32(target + 4, second);
        rc::st32(Index, index + 1);
    }
    else
    {
        const float first = float(ld(words));
        rc::stf32(target, first);
        const float second = float(ld(words + 4));
        rc::stf32(rc::ld32(Buffer) + rc::ld32(Index) * 24 + 0x18, second);
        rc::st32(Index, rc::ld32(Index) + 1);
    }
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    spans.push_back(shadow::Span{Index, 4, false});
    spans.push_back(shadow::Span{rc::ld32(Buffer) + rc::ld32(Index) * 24 + 0x14, 8, false});
}

const shadow::Spec spec = {"sub_5be0f0", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5be0f0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5be0f0_guest, declare);
}

}}
