// Native sub_487270: 4x4 matrix copy constructor, zeroes this then copies 16 words from the argument (thiscall, ret 4).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_487270_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "vec_helpers_native")) [[unlikely]]
    {
        sub_487270_guest(app, cpu);
        return;
    }
    const x86::reg32 self = cpu.ecx;
    const x86::reg32 source = argument(cpu, 0);
    x86::reg32 word = 0;
    if (self - source >= 64 && source - self >= 64) [[likely]]
    {
        memcpy(rc::host(self), rc::host(source), 64);
        cpu.edx = rc::ld32(source + 0x38);
        word = rc::ld32(source + 0x3c);
    }
    else
    {
        for (x86::reg32 i = 0; i < 64; i += 4)
            rc::st32(self + i, 0);
        for (x86::reg32 i = 0; i < 64; i += 4)
        {
            word = rc::ld32(source + i);
            rc::st32(self + i, word);
            if (i == 0x38)
                cpu.edx = word;
        }
    }
    cpu.eax = self;
    cpu.ecx = word;
    cpu.esp += 8;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{cpu.ecx, 64, false});
}

const shadow::Spec spec = {"sub_487270", shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_487270(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_487270_guest, declare);
}

}}
