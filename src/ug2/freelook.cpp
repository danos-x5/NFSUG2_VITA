// Free look: the stick's direction turns the chase camera rig about the car like Look Back; the rear half raises
// the game's look-back flag, so down is vanilla.

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
const reg32 MoverNoMirror = 0x30;           // nonzero: look-back keeps the rig unmirrored
const reg32 MoverView = 0x4C;
const reg32 MoverLookBack = 0x51;           // byte: the rig is mirrored behind the car
const reg32 DirectorMirrors = 0x2B;         // byte; zero keeps the rig unmirrored
const reg32 DirectorMover = 0x238;          // the active mover
const reg32 DirectorOldMovers = 0x23C;      // two movers kept after a camera change

// The update's look-at call (0x453b44): its frame from the callee's esp.
const reg32 LookAtReturn = 0x00453B49;
const reg32 FrameFrom = 0x54, FrameTo = 0x44, FrameRig = 0x184;     // eye, target, rig matrix (car space to world)

const uint32_t StaleFrames = 4;

struct State
{
    reg32    camera = 0;
    uint32_t frame = 0;         // of the camera's last update
    bool     live = false;
    bool     held = false;      // the stick is out of its dead zone
    float    yaw = 0;           // view yaw, degrees right of ahead
    bool     back = false;      // the rear half: free look holds the game's look-back flag
    uint8_t  gameBack = 0;      // the flag as the game set it
    uint8_t  written = 0;       // the flag as free look last set it
    uint32_t logFrame = 0;
};

State s_state;
reg32 s_camera = 0;
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

bool paused()
{
    const reg32 coordinator = rc::ld32(RaceCoordinator);
    return (coordinator && rc::ld32(coordinator) == 6) || rc::ld8(OptionsExit) != 0;
}

bool hideRightStick()
{
    return s_state.live && frontend::frameCounter() - s_state.frame <= StaleFrames;
}

void setLookBack(reg32 camera, uint8_t value)
{
    rc::st8(camera + MoverLookBack, value);
    s_state.written = value;
}

void release(reg32 camera, const char* why)
{
    State& s = s_state;
    if (s.held && camera)
    {
        setLookBack(camera, s.gameBack);
        if (logging())
            pf::logInfo("freelook: release (%s): chase view, game look back %u", why, unsigned(s.gameBack));
    }
    s.held = false;
    s.back = false;
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
            release(camera, "camera replaced");
            setLive(false, camera);
            s.camera = 0;
        }
        return;
    }
    if (camera != s.camera || frame - s.frame > StaleFrames)
    {
        if (camera == s.camera)
            release(camera, "no camera update for a while");
        else
        {
            const reg32 old = s.camera;
            const bool kept = rc::ld32(director + DirectorOldMovers) == old ||
                              rc::ld32(director + DirectorOldMovers + 4) == old;
            if (old && kept)
                release(old, "camera changed");
            if (logging() && old)
                pf::logInfo("freelook: camera %08X -> %08X", unsigned(old), unsigned(camera));
        }
        s.held = s.back = false;
        s.yaw = 0;
        s.camera = camera;
        s.gameBack = s.written = rc::ld8(camera + MoverLookBack);
    }
    s.frame = frame;
    if (paused())
    {
        setLive(false, camera);
        return;
    }
    if (rc::ld32(camera + MoverType) != 1 || rc::ld8(ReplayState))
    {
        release(camera, "not the race chase camera");
        setLive(false, camera);
        return;
    }
    setLive(true, camera);

    const uint8_t flag = rc::ld8(camera + MoverLookBack);
    if (!s.held || flag != s.written)
    {
        if (s.held && logging())
            pf::logInfo("freelook: game look back %u -> %u", unsigned(s.gameBack), unsigned(flag));
        s.gameBack = flag;
    }
    const float x = s_input[0], y = s_input[1];
    const fm::Aim aim = fm::aim(x, y);
    if (!aim.held)
    {
        release(camera, "stick centred");
        return;
    }
    const bool back = fm::backHalf(aim.yaw, s.held && s.back);
    if (logging() && (!s.held || back != s.back))
        pf::logInfo("freelook: %s: stick %.2f,%.2f at %.1f deg, yaw %.1f, look back %u", s.held ? "half" : "snap",
                    x, y, stickDegrees(x, y), aim.yaw, unsigned(back));
    s.held = true;
    s.yaw = aim.yaw;
    s.back = back;
    setLookBack(camera, back ? 1 : 0);
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

reg32 enter(reg32 camera)
{
    const reg32 saved = s_camera;
    s_camera = camera;
    if (camera)
        update(camera);
    return saved;
}

void leave(reg32 saved)
{
    s_camera = saved;
}

void beforeLookAt(win32::WinApplication*, x86::CPU& cpu)
{
    const State& s = s_state;
    const reg32 camera = s_camera;
    if (!camera || camera != s.camera || !s.live || !s.held || rc::ld32(cpu.esp) != LookAtReturn)
        return;
    const reg32 from = rc::ld32(cpu.esp + 8), to = rc::ld32(cpu.esp + 12), up = rc::ld32(cpu.esp + 16);
    if (from != cpu.esp + FrameFrom || to != cpu.esp + FrameTo)
        return;
    const reg32 director = rc::ld32(camera + MoverDirector);
    const bool mirrored = rc::ld8(camera + MoverLookBack) && !rc::ld32(camera + MoverNoMirror) &&
                          (!director || rc::ld8(director + DirectorMirrors));
    const float turn = fm::residual(s.yaw, mirrored);
    const bool log = logging() && frontend::frameCounter() - s.logFrame >= 6;
    if (log)
    {
        const float x = s_input[0], y = s_input[1];
        s_state.logFrame = frontend::frameCounter();
        pf::logInfo("freelook: stick %.2f,%.2f at %.1f deg, yaw %.1f, look back %u (game %u, mirrored %u), turn %.1f",
                    x, y, stickDegrees(x, y), s.yaw, unsigned(s.back), unsigned(s.gameBack), unsigned(mirrored), turn);
    }
    if (turn == 0.0f)
        return;

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
