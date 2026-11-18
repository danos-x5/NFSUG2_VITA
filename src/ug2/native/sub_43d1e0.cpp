// Native sub_43d1e0: cross product of two 3-vectors (cdecl: out, a, b), exact to the guest; out may alias a or b.

#include <native/check.h>
#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_43d1e0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

template< bool A >
inline void cross(x86::reg32 out, x86::reg32 a, x86::reg32 b)
{
    Real y = fast::lf< A >(b) * fast::lf< A >(a + 8);
    y -= fast::lf< A >(b + 8) * fast::lf< A >(a);
    Real z = fast::lf< A >(b + 4) * fast::lf< A >(a);
    z -= fast::lf< A >(a + 4) * fast::lf< A >(b);
    Real x = fast::lf< A >(b + 8) * fast::lf< A >(a + 4);
    x -= fast::lf< A >(b + 4) * fast::lf< A >(a + 8);
    fast::sf< A >(out, float(x));
    fast::sf< A >(out + 4, float(y));
    fast::sf< A >(out + 8, float(z));
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("phys_vector_native"))
    {
        sub_43d1e0_guest(app, cpu);
        return;
    }
    const x86::reg32 out = argument(cpu, 0), a = argument(cpu, 1), b = argument(cpu, 2);
    if (fast::aligned(out | a | b)) [[likely]]
        cross< true >(out, a, b);
    else
        cross< false >(out, a, b);
    cpu.eax = out;
    cpu.ecx = b;
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 12, false});
}

const x86::reg8 compared = shadow::Eax | shadow::Ecx | shadow::Ebx | shadow::Ebp | shadow::Esi |
                           shadow::Edi;
const shadow::Spec spec = {"sub_43d1e0", compared, 0, false, false};
check::Stats s_check = {"sub_43d1e0"};

}

void sub_43d1e0(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, compared, app, cpu, body, sub_43d1e0_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_43d1e0_guest, declare);
}

}}
