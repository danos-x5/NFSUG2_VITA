// Pad bindings: installs the port's default bindings and leaves the rest to the game's Controls screen and profile.
// Also the pad log, the pad script and the hidden mouse cursor.

#include "padbind.h"
#include "pad_defaults.h"
#include "controlsshell.h"
#include "ownscreens.h"
#include "psbar.h"
#include "psprompt.h"

#include <lib/controls.h>
#include <lib/gamepad.h>
#include <platform/config.h>
#include <platform/log.h>
#include <cmath>
#include <cstring>
#include <string>
#include <recomp.h>

namespace ug2 { namespace padbind {

namespace {

using x86::reg32;
namespace controls = win32::controls;

const reg32 PeekMessageSlot = 0x00783330;   // import slot of PeekMessageA, called every frame by 0x5BF1C0
const reg32 GameState = 0x008654A4;         // 3 front end
const reg32 InputMap = 0x00870788;          // set by 0x5CFBF0: 1 menu map, else a race map
const reg32 Slots = 0x00874C40;             // six input slots built by 0x5C11A0, mask made by 0x5C1840
const reg32 SlotSize = 0x370;
const reg32 SlotCount = 6;
const reg32 SlotChannel = 0x230;            // device index, -1 unused
const reg32 SlotType = 0x234;               // 0x13 keyboard
const reg32 SlotRace = 0x18;                // 16 button actions, then 4 hats of 4 directions: down, up, left, right
const reg32 SlotMenu = 0x98;                // same, for the menu map
const reg32 KeyboardType = 0x13;
const reg32 SlotMask = 0x8;                 // 64 bits, a cleared bit is an active action
const reg32 Players = 0x008900AC;           // player pointers, player 0 first; the car is at +4, its position at +0x60
const reg32 VehiclePosition = 0x60;
const reg32 WorldSeconds = 0x007FB718;      // float
const reg32 RaceCoordinator = 0x008900D8;
const reg32 GearOffset = 0x27C;             // the car's gear, found by diffing memory around shifts
const float CarLogPeriod = 0.5f;
const reg32 BindingName = 0x005C20E0;       // thiscall slot (action, 0/1 primary/secondary): Controls screen text

win32::MethodPtr s_peekMessage = nullptr;
bool s_log = false;
controls::PadScript s_script;
bool s_scripted = false;
uint64_t s_scriptStep = 0;       // frames since the race map was first active
int s_layout = -1;
reg32 s_namesSlot = 0;          // slot whose Controls names pad_log prints next

void logCar()
{
    static float lastX = 0, lastY = 0, lastTime = 0;
    static unsigned long long lastMask = ~0ull;
    const unsigned long long mask = rc::ld32(Slots + SlotMask) | (unsigned long long)rc::ld32(Slots + SlotMask + 4) << 32;
    if (mask != lastMask)
    {
        pf::logInfo("padbind: action mask %016llX (cleared bits are active; bit 0 is the empty action)", mask);
        lastMask = mask;
    }
    const reg32 player = rc::ld32(Players);
    const reg32 vehicle = player ? rc::ld32(player + 4) : 0;
    const float now = rc::ldf32(WorldSeconds);
    if (!vehicle || now - lastTime < CarLogPeriod)
        return;
    const float x = rc::ldf32(vehicle + VehiclePosition), y = rc::ldf32(vehicle + VehiclePosition + 4);
    const reg32 coordinator = rc::ld32(RaceCoordinator);
    pf::logInfo("padbind: world %.1f s, car (%.1f, %.1f) %.1f m/s, gear %u", now, x, y,
                lastTime > 0 ? std::hypot(x - lastX, y - lastY) / (now - lastTime) : 0.0f,
                unsigned(coordinator ? rc::ld32(coordinator + GearOffset) : 0));
    lastX = x;
    lastY = y;
    lastTime = now;
}

bool padSlot(reg32 slot)
{
    return rc::ld32(slot + SlotChannel) == 0 && rc::ld32(slot + SlotType) != KeyboardType;
}

// Logs the pad's race and menu maps whenever they change.
void logMaps()
{
    static reg32 last[2][20];
    static bool logged = false;
    for (reg32 slot = Slots; slot < Slots + SlotCount * SlotSize; slot += SlotSize)
    {
        if (!padSlot(slot))
            continue;
        reg32 maps[2][20];
        for (reg32 i = 0; i < 20; ++i)
        {
            maps[0][i] = rc::ld32(slot + SlotRace + 4 * i);
            maps[1][i] = rc::ld32(slot + SlotMenu + 4 * i);
        }
        if (logged && memcmp(maps, last, sizeof(maps)) == 0)
            return;
        memcpy(last, maps, sizeof(maps));
        logged = true;
        s_namesSlot = slot;
        for (int m = 0; m < 2; ++m)
        {
            const reg32* a = maps[m];
            pf::logInfo("padbind: %s map, buttons 0-15: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u; "
                        "unread hat entries: %u %u %u %u", m ? "menu" : "race", unsigned(a[0]), unsigned(a[1]),
                        unsigned(a[2]), unsigned(a[3]), unsigned(a[4]), unsigned(a[5]), unsigned(a[6]),
                        unsigned(a[7]), unsigned(a[8]), unsigned(a[9]), unsigned(a[10]), unsigned(a[11]),
                        unsigned(a[12]), unsigned(a[13]), unsigned(a[14]), unsigned(a[15]), unsigned(a[16]),
                        unsigned(a[17]), unsigned(a[18]), unsigned(a[19]));
        }
        return;
    }
}

void program()
{
    if (s_log)
        logCar();
    const bool driving = rc::ld32(InputMap) != 1;
    if (s_scripted && (s_scriptStep || (driving && rc::ld32(GameState) == 6)))
        win32::Gamepad::setInjectedPad(s_script.sampleAt(s_scriptStep++));
    if (!s_log)
        return;
    const int layout = driving ? 1 : 2;
    if (layout != s_layout)
        pf::logInfo("padbind: %s map (game state %u, input map %u)", driving ? "race" : "menu",
                    unsigned(rc::ld32(GameState)), unsigned(rc::ld32(InputMap)));
    s_layout = layout;
    logMaps();
}

std::string guestString(reg32 address)
{
    std::string text;
    for (char c; address && (c = char(rc::ld8(address))) != 0 && text.size() < 64; ++address)
        text += c;
    return text;
}

// Logs the Controls screen rows as the game names them.
void logNames(win32::WinApplication* app, x86::CPU& cpu, reg32 slot)
{
    static const struct { reg32 action; const char* row; } Rows[] = {
        { 3, "steer left" }, { 4, "steer right" }, { 1, "throttle" }, { 2, "brake/reverse" }, { 7, "shift up" },
        { 8, "shift down" }, { 5, "e-brake" }, { 6, "nitrous" }, { 10, "camera" }, { 9, "look back" },
        { 15, "engage" }, { 16, "world map" }, { 17, "read SMS" }, { 18, "career stats" }, { 14, "reset car" },
        { 19, "hydraulic bounce" }, { 22, "hydraulic pressurize" }, { 12, "pause" },
    };
    std::string line;
    for (const auto& r : Rows)
    {
        std::string names[2];
        for (reg32 which = 0; which < 2; ++which)
        {
            const x86::CPU saved = cpu;
            cpu.esp -= 8;
            rc::st32(cpu.esp, r.action);
            rc::st32(cpu.esp + 4, which);
            cpu.esp -= 4;
            cpu.ecx = slot;
            app->dynamic_call(BindingName, cpu);
            names[which] = guestString(cpu.eax);
            cpu = saved;
        }
        line += std::string(line.empty() ? "" : ", ") + r.row + " " + (names[0].empty() ? "---" : names[0]) + " / " +
                (names[1].empty() ? "---" : names[1]);
    }
    pf::logInfo("padbind: Controls screen: %s", line.c_str());
}

void peekMessageHook(win32::WinApplication* app, x86::CPU& cpu)
{
    s_peekMessage(app, cpu);
    program();
    ownscreens::frame(app, cpu);
    controlsshell::frame(app, cpu);
    if (s_namesSlot)
    {
        logNames(app, cpu, s_namesSlot);
        s_namesSlot = 0;
    }
}

}

}

void sub_50b460_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

// Mouse cursor draw (cdecl; x, y): none with front_touch=on.
void sub_50b460(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!win32::controls::frontTouchEnabled())
        sub_50b460_guest(app, cpu);
    else
        cpu.esp += 4;
}

}

namespace padbind
{

void configure(win32::WinApplication& app)
{
    s_log = pf::Config::getBool("pad_log", false);
    pad_defaults::install();
    psprompt::configure(app);
    psbar::install();
    controlsshell::configure(app);
    const char* script = pf::Config::getString("pad_script", "");
    s_scripted = *script && s_script.load(script);
    s_peekMessage = app.replaceMethod(rc::ld32(PeekMessageSlot), peekMessageHook);
    pf::logInfo("padbind: port defaults in the EMS table, then the game's own bindings");
}

}}
