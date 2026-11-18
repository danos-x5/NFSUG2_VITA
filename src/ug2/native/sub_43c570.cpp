// Native sub_43c570: 16-bit angle of (x, y) from the arctangent byte table 0x7EACB0 (cdecl; result in ax). Registers,
// status word and argument slot end as in the guest.

#include <native/check.h>
#include <native/common.h>
#include <native/physics.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_43c570_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("phys_vector_native"))
    {
        sub_43c570_guest(app, cpu);
        return;
    }
    const x86::reg32 E = cpu.esp;
    const Real x = ld(E + 4);
    if (x < ld(0x7d7d3c))
        rc::stf32(E + 4, float(-x));
    physics::Registers out = {cpu.eax, cpu.ecx, cpu.edx};
    physics::angle(cpu, x, ld(E + 8), out);
    cpu.eax = out.eax;
    cpu.ecx = out.ecx;
    cpu.edx = out.edx;
    cpu.esp = E + 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{cpu.esp + 4, 4, false});
}

const x86::reg8 compared = shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp |
                           shadow::Esi | shadow::Edi;
const shadow::Spec spec = {"sub_43c570", compared, 0, false, false};
check::Stats s_check = {"sub_43c570"};

}

void sub_43c570(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, compared, app, cpu, body, sub_43c570_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_43c570_guest, declare);
}

}}
