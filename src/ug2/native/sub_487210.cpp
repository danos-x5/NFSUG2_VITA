// Native sub_487210: matrix times a by-value 4-float vector into an output vector (thiscall: this = matrix; out, 4 vector words; ret 0x14).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_487210_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "vec_helpers_native")) [[unlikely]]
    {
        sub_487210_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 m = cpu.ecx;
    const x86::reg32 out = argument(cpu, 0);
    // The fourth word is the callee's uninitialised local; the translated body reads that stack slot.
    const x86::reg32 junk = rc::ld32((((entry - 4) & ~15u) - 0x20) + 0xc);
    float x, y, z;
    if (fast::aligned(out | m | entry)) [[likely]]
    {
        fast::transform< true >(m, entry + 8, x, y, z);
        fast::sf< true >(out, x);
        fast::sf< true >(out + 4, y);
        fast::sf< true >(out + 8, z);
    }
    else
    {
        fast::transform< false >(m, entry + 8, x, y, z);
        rc::stf32(out, x);
        rc::stf32(out + 4, y);
        rc::stf32(out + 8, z);
    }
    rc::st32(out + 12, junk);
    cpu.eax = out;
    cpu.esp = entry + 0x18;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 16, false});
}

const shadow::Spec spec = {"sub_487210", shadow::Eax | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_487210(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_487210_guest, declare);
}

}}
