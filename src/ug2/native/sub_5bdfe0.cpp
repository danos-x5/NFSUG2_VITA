// Native sub_5bdfe0: resets the shadow outline buffer (allocates via 0x440d40) and loads the 4x4 matrix argument or identity (cdecl: a, unused, matrix).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5bdfe0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_440d40(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 Pool = 0x870954;
const x86::reg32 Buffer = 0x870958;
const x86::reg32 Matrix = 0x872bc0;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("shadow_points_native"))
    {
        sub_5bdfe0_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 first = argument(cpu, 0);
    const x86::reg32 source = argument(cpu, 2);
    const x86::reg32 savedEbp = cpu.ebp;
    cpu.ecx = rc::ld32(Pool);
    cpu.esp = entry - 12;
    cpu.ebp = entry - 4;
    sub_440d40(app, cpu);
    if (cpu.terminate)
        return;
    cpu.ebp = savedEbp;
    rc::st32(Buffer, cpu.eax);
    rc::st32(0x870964, 0);
    rc::st32(0x870968, 0);
    rc::st32(0x87096c, 0);
    rc::st32(0x870960, first);
    if (source == 0)
    {
        for (x86::reg32 i = 0; i < 64; i += 4)
            rc::st32(Matrix + i, 0);
        rc::st32(Matrix, 0x3f800000);
        rc::st32(Matrix + 0x14, 0x3f800000);
        rc::st32(Matrix + 0x28, 0x3f800000);
        rc::st32(Matrix + 0x3c, 0x3f800000);
        cpu.eax = (Matrix & ~0xffu) | 1;
    }
    else
    {
        for (x86::reg32 i = 0; i < 64; i += 4)
            rc::st32(Matrix + i, rc::ld32(source + i));
        cpu.eax = 1;
    }
    cpu.ecx = 0;
    cpu.esp = entry + 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    spans.push_back(shadow::Span{Buffer, 4, false});
    spans.push_back(shadow::Span{0x870960, 16, false});
    spans.push_back(shadow::Span{Matrix, 64, false});
}

const shadow::Spec spec = {"sub_5bdfe0", shadow::Eax | shadow::Ecx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5bdfe0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bdfe0_guest, declare);
}

}}
