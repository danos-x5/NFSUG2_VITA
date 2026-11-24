// Garage camera orbit (sub_4a5d90): the right stick reaches the garage screen as the game's own car orbit
// events, through its handler 0x4A5740.

#include "../garage_stick.h"
#include <native/common.h>
#include <lib/gamepad.h>
#include <platform/config.h>

namespace ug2
{

void sub_4a5d90_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_4a5740(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 OrbitInput = 0x23C;        // the garage screen's orbit object; its +4 is the handler's gate
const x86::reg32 Busy = 0x00838578;         // the handler's other gate
const x86::reg32 IdleSpin = 0x008363FC, IdleSince = 0x00837AA0, Ticks = 0x008651AC;

bool enabled()
{
    static const bool on = pf::Config::getBool("garage_stick", true);
    return on;
}

// Sends one joy event to the screen's orbit handler.
void orbitEvent(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 screen, x86::reg32 event, uint8_t value)
{
    const x86::reg32 esp = cpu.esp;
    rc::st32(esp - 4, value);
    rc::st32(esp - 8, event);
    rc::st32(esp - 12, 0);
    cpu.esp = esp - 16;
    cpu.ecx = screen + OrbitInput;
    sub_4a5740(app, cpu);
    cpu.esp = esp;
}

}

void sub_4a5d90(win32::WinApplication* app, x86::CPU& cpu)
{
    const x86::reg32 screen = cpu.ecx;
    float x = 0.0f, y = 0.0f;
    if (enabled())
        win32::Gamepad::getFixedRightStick(&x, &y);
    const garage_stick::OrbitBytes orbit = garage_stick::orbitBytes(x, y, 0.0f);
    static bool s_held;
    if (orbit.held || s_held)
    {
        orbitEvent(app, cpu, screen, garage_stick::OrbitLeftRight, orbit.leftRight);
        orbitEvent(app, cpu, screen, garage_stick::OrbitUpDown, orbit.upDown);
        if (orbit.held && rc::ld32(screen + OrbitInput + 4) && !rc::ld32(Busy))
        {
            rc::st8(IdleSpin, 0);
            rc::st32(IdleSince, rc::ld32(Ticks));
        }
        s_held = orbit.held;
        cpu.ecx = screen;
    }
    sub_4a5d90_guest(app, cpu);
}

}

}
