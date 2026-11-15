// Native sub_441000: transforms a point by a 4x4 matrix (rows 0..3 as coefficients, translation in row 3) (cdecl: out, matrix, point).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_441000_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "vec_helpers_native")) [[unlikely]]
    {
        sub_441000_guest(app, cpu);
        return;
    }
    const x86::reg32 out = argument(cpu, 0);
    const x86::reg32 m = argument(cpu, 1);
    const x86::reg32 in = argument(cpu, 2);
    float x, y, z;
    if (fast::aligned(out | m | in)) [[likely]]
    {
        fast::transform< true >(m, in, x, y, z);
        fast::sf< true >(out, x);
        fast::sf< true >(out + 4, y);
        fast::sf< true >(out + 8, z);
    }
    else
    {
        fast::transform< false >(m, in, x, y, z);
        rc::stf32(out, x);
        rc::stf32(out + 4, y);
        rc::stf32(out + 8, z);
    }
    cpu.eax = out;
    cpu.ecx = in;
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 12, false});
}

const shadow::Spec spec = {"sub_441000", shadow::Eax | shadow::Ecx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_441000(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_441000_guest, declare);
}

}}
