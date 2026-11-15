// Native sub_5be060: stores a colour (bytes reordered A, R, G, B from the argument's byte order) into the shadow outline buffer (cdecl).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_5be060_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 Buffer = 0x870958;
const x86::reg32 Index = 0x87096c;

signed char s_on;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "shadow_points_native")) [[unlikely]]
    {
        sub_5be060_guest(app, cpu);
        return;
    }
    const x86::reg32 c = argument(cpu, 0);
    const x86::reg32 packed = (c & 0xff00ff00u) | ((c & 0xffu) << 16) | ((c >> 16) & 0xffu);
    rc::st32(rc::ld32(Buffer) + rc::ld32(Index) * 24 + 0x10, packed);
    rc::st32(Index, rc::ld32(Index) + 1);
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    (void)cpu;
    spans.push_back(shadow::Span{Index, 4, false});
    spans.push_back(shadow::Span{rc::ld32(Buffer) + rc::ld32(Index) * 24 + 0x10, 4, false});
}

const shadow::Spec spec = {"sub_5be060", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5be060(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5be060_guest, declare);
}

}}
