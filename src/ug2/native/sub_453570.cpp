// Chase camera update (sub_453570, thiscall): free look sets the look-back flag and marks its look-at call.

#include "../freelook.h"
#include <native/common.h>

namespace ug2
{

void sub_453570_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

void sub_453570(win32::WinApplication* app, x86::CPU& cpu)
{
    const x86::reg32 saved = freelook::enter(cpu.ecx);
    sub_453570_guest(app, cpu);
    freelook::leave(saved);
}

}

}
