// Native sub_5bc450: point against the six view planes of a view object, 2 when inside all of them else 0 (thiscall, ret 8).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_5bc450_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

template< bool A >
inline void test(x86::CPU& cpu, x86::reg32 planes, x86::reg32 point)
{
    const Real px = fast::lf< A >(point), py = fast::lf< A >(point + 4), pz = fast::lf< A >(point + 8);
    const Real zero = fast::lf< true >(0x7d7d3c);
    cpu.eax = 2;
    cpu.edx = 6;
    for (x86::reg32 i = 0; i < 6; ++i)
    {
        const x86::reg32 plane = planes + 16 * i;
        Real d = fast::lf< A >(plane + 8) * pz;
        d += fast::lf< A >(plane) * px;
        d += fast::lf< A >(plane + 4) * py;
        d += fast::lf< A >(plane + 12);
        if (!(d > zero))
        {
            cpu.eax = 0;
            cpu.edx = i;
            break;
        }
    }
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "view_visible_native")) [[unlikely]]
    {
        sub_5bc450_guest(app, cpu);
        return;
    }
    const x86::reg32 planes = rc::ld32(cpu.ecx) + 0xc0;
    const x86::reg32 point = argument(cpu, 0);
    if (fast::aligned(planes | point)) [[likely]]
        test< true >(cpu, planes, point);
    else
        test< false >(cpu, planes, point);
    cpu.esp += 12;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    (void)spans;
}

const shadow::Spec spec = {"sub_5bc450", shadow::Eax | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5bc450(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bc450_guest, declare);
}

}}
