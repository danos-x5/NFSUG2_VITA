// Free look: the stick's direction turns the chase camera rig about the car, the turn Look Back makes at 180
// degrees. The game's own Look Back is never touched.

#include "freelook.h"
#include "freelook_math.h"
#include "frontend.h"

#include <lib/gamepad.h>
#include <platform/config.h>
#include <platform/log.h>
#include <recomp.h>

#include <atomic>
#include <cmath>

namespace ug2 { namespace freelook {

namespace
{

using x86::reg32;
namespace fm = freelook_math;

const reg32 RaceCoordinator = 0x008900D8;   // pointer; its first dword is 6 while the race is paused
const reg32 OptionsExit = 0x008384DC;       // byte
const reg32 ReplayState = 0x0085EF6C;       // byte; the game ignores Look Back while it is set (0x6033c7)

// Chase camera mover (vtable 0x787f28, update 0x453570) and the camera director that creates it (0x5f1fd0).
const reg32 MoverType = 0x0C;               // 1: the player's chase camera, the one Look Back reaches
const reg32 MoverDirector = 0x20;
const reg32 MoverNoMirror = 0x30;           // nonzero: Look Back keeps the rig unmirrored
const reg32 MoverView = 0x4C;
const reg32 MoverLookBack = 0x51;           // byte: the game's Look Back mirrors the rig
const reg32 DirectorMirrors = 0x2B;         // byte; zero keeps the rig unmirrored
const reg32 DirectorMover = 0x238;          // the active mover

// The update's look-at call (0x453b44): its frame from the callee's esp.
const reg32 FrameFrom = 0x54, FrameTo = 0x44, FrameRig = 0x184;     // eye, target, rig matrix (car space to world)

const uint32_t StaleFrames = 4;

struct State
{
    reg32       camera = 0;
    uint32_t    frame = 0;          // of the camera's last update
    bool        live = false;
    bool        held = false;       // the stick is out of its dead zone
    float       yaw = 0;            // view yaw, degrees right of ahead
    uint32_t    logFrame = 0;
    bool        turned = false;     // the look-at hook has turned this camera
    const char* declined = nullptr; // the reason last logged
};

State s_state;
Update s_update = {};
std::atomic<float> s_input[2];

bool rawStick()
{
    static const bool on = pf::Config::getBool("free_look", false);
    return on;
}

bool logging()
{
#ifdef DEV_TOOLS
    return true;
#else
    return false;
#endif
}

float stickDegrees(float x, float y)
{
    return float(std::atan2(double(x), double(-y)) * (180.0 / 3.14159265358979323846));
}

void decline(const char* why, reg32 esp)
{
    if (!logging() || s_state.declined == why)
        return;
    s_state.declined = why;
    pf::logInfo("freelook: look-at not turned: %s (esp %08X, expected %08X)", why, unsigned(esp),
                unsigned(s_update.lookAtEsp));
}

bool paused()
{
    const reg32 coordinator = rc::ld32(RaceCoordinator);
    return (coordinator && rc::ld32(coordinator) == 6) || rc::ld8(OptionsExit) != 0;
}

bool hideRightStick()
{
    return s_state.live && frontend::frameCounter() - s_state.frame <= StaleFrames;
}

void release(const char* why)
{
    State& s = s_state;
    if (s.held && logging())
        pf::logInfo("freelook: release (%s): chase view", why);
    s.held = false;
    s.yaw = 0;
}

void setLive(bool live, reg32 camera)
{
    if (live != s_state.live && logging())
        pf::logInfo("freelook: %s, camera %08X view %u", live ? "live" : "idle", unsigned(camera),
                    unsigned(camera ? rc::ld32(camera + MoverView) : 0));
    s_state.live = live;
}

void update(reg32 camera)
{
    State& s = s_state;
    const uint32_t frame = frontend::frameCounter();
    if (rawStick())
    {
        static bool s_hooked;
        if (!s_hooked)
        {
            win32::Gamepad::setRightStickFilter(hideRightStick);
            s_hooked = true;
        }
        float x, y;
        win32::Gamepad::getRightStick(&x, &y);
        setInput(x, y);
    }
    const reg32 director = rc::ld32(camera + MoverDirector);
    if (!director || rc::ld32(director + DirectorMover) != camera)
    {
        if (camera == s.camera)
        {
            release("camera replaced");
            setLive(false, camera);
            s.camera = 0;
        }
        return;
    }
    if (camera != s.camera || frame - s.frame > StaleFrames)
    {
        if (camera == s.camera)
            release("no camera update for a while");
        else if (logging() && s.camera)
            pf::logInfo("freelook: camera %08X -> %08X", unsigned(s.camera), unsigned(camera));
        s.held = s.turned = false;
        s.yaw = 0;
        s.camera = camera;
    }
    s.frame = frame;
    if (paused())
    {
        setLive(false, camera);
        return;
    }
    if (rc::ld32(camera + MoverType) != 1 || rc::ld8(ReplayState))
    {
        release("not the race chase camera");
        setLive(false, camera);
        return;
    }
    setLive(true, camera);

    const float x = s_input[0], y = s_input[1];
    const fm::Aim aim = fm::aim(x, y);
    if (!aim.held)
    {
        release("stick centred");
        return;
    }
    if (!s.held)
    {
        s.declined = nullptr;
        if (logging())
            pf::logInfo("freelook: snap: stick %.2f,%.2f at %.1f deg, yaw %.1f", x, y, stickDegrees(x, y), aim.yaw);
    }
    s.held = true;
    s.yaw = aim.yaw;
}

}

void setInput(float x, float y)
{
    s_input[0] = x;
    s_input[1] = y;
}

bool live()
{
    return hideRightStick();
}

Update enter(reg32 camera, reg32 esp)
{
    const Update outer = s_update;
    s_update = { camera, fm::lookAtEsp(esp) };
    if (camera)
        update(camera);
    return outer;
}

void leave(const Update& outer)
{
    s_update = outer;
}

void beforeLookAt(win32::WinApplication*, x86::CPU& cpu)
{
    State& s = s_state;
    const reg32 camera = s_update.camera;
    if (!camera || camera != s.camera || !s.live || !s.held)
        return;
    if (cpu.esp != s_update.lookAtEsp)
    {
        decline("another look-at inside the update", cpu.esp);
        return;
    }
    const reg32 from = rc::ld32(cpu.esp + 8), to = rc::ld32(cpu.esp + 12), up = rc::ld32(cpu.esp + 16);
    if (from != cpu.esp + FrameFrom || to != cpu.esp + FrameTo)
    {
        decline("eye or target not in the update's frame", cpu.esp);
        return;
    }
    const reg32 director = rc::ld32(camera + MoverDirector);
    const bool mirrored = rc::ld8(camera + MoverLookBack) && !rc::ld32(camera + MoverNoMirror) &&
                          (!director || rc::ld8(director + DirectorMirrors));
    const float turn = fm::residual(s.yaw, mirrored);
    if (logging() && frontend::frameCounter() - s.logFrame >= 6)
    {
        const float x = s_input[0], y = s_input[1];
        s.logFrame = frontend::frameCounter();
        pf::logInfo("freelook: stick %.2f,%.2f at %.1f deg, yaw %.1f, game look back %u, turn %.1f", x, y,
                    stickDegrees(x, y), s.yaw, unsigned(mirrored), turn);
    }
    if (turn == 0.0f)
    {
        decline(mirrored ? "stick down with Look Back held: the game's own view" : "stick up: the chase view", cpu.esp);
        return;
    }
    if (!s.turned && logging())
        pf::logInfo("freelook: first turn of camera %08X: yaw %.1f, turn %.1f", unsigned(camera), s.yaw, turn);
    s.turned = true;
    s.declined = nullptr;

    float m[16];
    for (int i = 0; i < 16; ++i)
        m[i] = rc::ldf32(cpu.esp + FrameRig + 4 * i);
    const fm::Vec eye = { rc::ldf32(from), rc::ldf32(from + 4), rc::ldf32(from + 8) };
    const fm::Vec look = { rc::ldf32(to), rc::ldf32(to + 4), rc::ldf32(to + 8) };
    const fm::Vec upward = { rc::ldf32(up), rc::ldf32(up + 4), rc::ldf32(up + 8) };
    const fm::Vec forward = { look.x - eye.x, look.y - eye.y, look.z - eye.z };
    float cosA, sinA;
    fm::turn(turn, fm::rightTurnSign(m, forward, upward), &cosA, &sinA);
    const fm::Vec newEye = fm::spin(m, eye, cosA, sinA), newLook = fm::spin(m, look, cosA, sinA);
    rc::stf32(from, newEye.x);
    rc::stf32(from + 4, newEye.y);
    rc::stf32(from + 8, newEye.z);
    rc::stf32(to, newLook.x);
    rc::stf32(to + 4, newLook.y);
    rc::stf32(to + 8, newLook.z);
}

}}
