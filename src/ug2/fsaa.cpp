// Display > Full Screen Anti-Aliasing as a text row (Off, 2x, 4x) over g_FSAALevel; the port's renderer multisamples.
// The game's own FSAA path reads an always-zero slot instead (tools/recompiler/games/ug2.py PATCHES).

#include "fsaa.h"

#include <native/common.h>
#include <platform/log.h>
#include <winapi/d3d9_native.h>

#include <initializer_list>

namespace ug2
{

void sub_5b9a60_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace
{

using x86::reg32;

const reg32 Level = 0x870D08;               // g_FSAALevel
const reg32 Cap = 0x876520;                 // highest level: NONMASKABLE quality levels - 1 (0x5D2714)
const reg32 GraphicsDirty = 0x8709D1;       // byte; the main loop's 0x5D2850 then Resets the device
const reg32 RowVtable = 0x7924A8;           // the FSAA row's (a slider's in the game)
const reg32 TextRowVtable = 0x79B3B8;       // base class of the game's text rows
const reg32 TextRowSlots = 17;
const reg32 MessageHandler = 0x49C160, Update = 0x49C1B0;
const reg32 TextTwo = 0x7E7FF8, TextFour = 0x7E7FFC;    // "2x", "4x": zero bytes past .rdata
const reg32 SetText = 0x50C900;             // cdecl (text object, language hash)
const reg32 Print = 0x537BE0;               // cdecl (text object, format, ...)
const reg32 LabelHash = 0x4CD775AC;         // OPT_FSAA
const reg32 OffHash = 0xCCFA;               // OFF, as the game's other Display rows show it
const reg32 MessageAccept = 0x0C407210, MessageLeft = 0x9120409E, MessageRight = 0xB5971BF1;
const reg32 SlotArrows = 0x40, SlotUpdate = 0x0C;
const reg32 LabelObject = 0x2C, ValueObject = 0x30;

reg32 call(win32::WinApplication* app, x86::CPU& cpu, reg32 function, reg32 self, std::initializer_list<reg32> args)
{
    const x86::CPU saved = cpu;
    reg32 esp = cpu.esp;
    for (const reg32* it = args.end(); it != args.begin();)
        rc::st32(esp -= 4, *--it);
    cpu.esp = esp - 4;
    cpu.ecx = self;
    app->dynamic_call(function, cpu);
    const reg32 result = cpu.eax;
    cpu = saved;
    return result;
}

void storeText(reg32 address, const char* text)
{
    for (reg32 i = 0; i < 4; ++i)
    {
        rc::st8(address + i, x86::reg8(*text));
        if (*text)
            ++text;
    }
}

// The FSAA row's vtable becomes a text row's with its own handler and texts.
void makeTextRow()
{
    for (reg32 slot = 0; slot < TextRowSlots; ++slot)
        rc::st32(RowVtable + 4 * slot, rc::ld32(TextRowVtable + 4 * slot));
    rc::st32(RowVtable + 4, MessageHandler);
    rc::st32(RowVtable + SlotUpdate, Update);
    storeText(TextTwo, fsaa::valueText(1));
    storeText(TextFour, fsaa::valueText(2));
}

}

namespace native
{

// Row message (thiscall; -, message): arrows step the level.
void sub_49c160(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 self = cpu.ecx, message = argument(cpu, 1);
    const reg32 vtable = rc::ld32(self);
    if (message == MessageAccept || message == MessageLeft || message == MessageRight)
    {
        call(app, cpu, rc::ld32(vtable + SlotArrows), self, { message });
        const int32_t level = int32_t(rc::ld32(Level));
        const int32_t next = fsaa::step(level, int32_t(rc::ld32(Cap)), message == MessageLeft);
        if (next != level)
        {
            rc::st32(Level, reg32(next));
            rc::st8(GraphicsDirty, 1);
        }
    }
    call(app, cpu, rc::ld32(vtable + SlotUpdate), self, {});
    cpu.esp += 12;
}

// Row texts (thiscall): label, then OFF, 2x or 4x.
void sub_49c1b0(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 self = cpu.ecx;
    call(app, cpu, SetText, 0, { rc::ld32(self + LabelObject), LabelHash });
    const int32_t level = int32_t(rc::ld32(Level));
    const reg32 value = rc::ld32(self + ValueObject);
    const char* text = fsaa::valueText(level);
    if (!text)
        call(app, cpu, SetText, 0, { value, OffHash });
    else
        call(app, cpu, Print, 0, { value, level == 1 ? TextTwo : TextFour });
    static int32_t s_shown = -1;
    if (level != s_shown)
        pf::logInfo("fsaa: Display row shows %s (level %d, value object 0x%08X)", text ? text : "OFF", int(level),
                    unsigned(value));
    s_shown = level;
    cpu.esp += 4;
}

// Present parameters (cdecl; width, height): the renderer follows the level.
void sub_5b9a60(win32::WinApplication* app, x86::CPU& cpu)
{
    static bool s_row = false;
    if (!s_row)
    {
        makeTextRow();
        s_row = true;
    }
    sub_5b9a60_guest(app, cpu);
    static uint32_t s_samples = 0;
    const int32_t level = int32_t(rc::ld32(Level));
    const uint32_t samples = fsaa::samples(level);
    if (samples != s_samples)
        pf::logInfo("fsaa: level %d (menu maximum %d): %s", int(level), int(int32_t(rc::ld32(Cap))),
                    samples == 1 ? "off" : samples == 2 ? "2x" : "4x");
    s_samples = samples;
    win32::d3d9::bridge::setMsaaSamples(samples);
}

}

}
