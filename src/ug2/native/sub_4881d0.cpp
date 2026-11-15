// Native sub_4881d0: particle position, a cubic in time with matrix rows as coefficients (cdecl: out, matrix, time).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_4881d0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

template< bool A >
inline void cubic(x86::CPU& cpu, x86::reg32 out, x86::reg32 m)
{
    const Real t = fast::lf< A >(cpu.esp + 12);
    const Real t2 = t * t;
    const Real t3 = t * t2;

    Real x = t2 * fast::lf< A >(m + 0x10);
    x += t * fast::lf< A >(m + 0x20);
    x += t3 * fast::lf< A >(m);
    x += fast::lf< A >(m + 0x30);
    Real y = t2 * fast::lf< A >(m + 0x14);
    y += t3 * fast::lf< A >(m + 4);
    y += t * fast::lf< A >(m + 0x24);
    y += fast::lf< A >(m + 0x34);
    Real z = t2 * fast::lf< A >(m + 0x18);
    z += t3 * fast::lf< A >(m + 8);
    z += t * fast::lf< A >(m + 0x28);
    z += fast::lf< A >(m + 0x38);
    Real w = t2 * fast::lf< A >(m + 0x1c);
    w += t3 * fast::lf< A >(m + 0xc);
    w += t * fast::lf< A >(m + 0x2c);
    w += fast::lf< A >(m + 0x3c);

    const float last = float(w);
    fast::sf< A >(out + 0xc, last);
    fast::sf< A >(out, float(x));
    fast::sf< A >(out + 4, float(y));
    fast::sf< A >(out + 8, float(z));
    cpu.ecx = bits(last);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "particles_native")) [[unlikely]]
    {
        sub_4881d0_guest(app, cpu);
        return;
    }
    const x86::reg32 out = argument(cpu, 0);
    const x86::reg32 m = argument(cpu, 1);
    if (fast::aligned(out | m | cpu.esp)) [[likely]]
        cubic< true >(cpu, out, m);
    else
        cubic< false >(cpu, out, m);
    cpu.eax = out;
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 16, false});
}

// The translated body writes back eax and ecx only.
const shadow::Spec spec = {"sub_4881d0", shadow::Eax | shadow::Ecx, 0, false, false};

}

void sub_4881d0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_4881d0_guest, declare);
}

}}
