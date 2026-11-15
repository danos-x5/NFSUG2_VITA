#include "drive.h"

#include <lib/controls.h>
#include <lib/gamepad.h>
#include <platform/log.h>
#include <recomp.h>

#include <atomic>
#include <cmath>
#include <cstdio>

namespace ug2 { namespace drive {

namespace {

using x86::reg32;
namespace controls = win32::controls;

const reg32 Players = 0x008900AC;              // player pointers, player 0 first
const reg32 PlayerVehicle = 0x4;
const reg32 VehiclePosition = 0x60;            // x, y, z
const reg32 WorldSeconds = 0x007FB718;         // float, advanced each frame by 0x57A0E0

const controls::Input Throttle = controls::R;
const controls::Input Brake = controls::L; // reverses once stopped
const float SamplePeriod = 0.5f;               // seconds of world time between position samples
const int StuckSamples = 8;                    // the window the car must move StuckDistance in
const float StuckDistance = 3.0f;
const float ReverseSeconds = 2.5f;
const float StraightSeconds = 6.0f;            // after the start and after reversing
const float LogPeriod = 5.0f;

enum Mode { Off, Driving, Braking, Reversing };
const char* const ModeNames[] = { "off", "driving", "braking", "reversing" };

struct Sample
{
    float x, y;
};

uint32_t s_random = 1;
Mode s_mode = Off;
float s_steer = 0.0f;
float s_modeEnd = 0.0f;
float s_nextSample = 0.0f;
float s_nextLog = 0.0f;
Sample s_samples[StuckSamples + 1] = {};
int s_sampleCount = 0;
int s_reversals = 0;
std::atomic<float> s_x{0.0f}, s_y{0.0f}, s_speed{0.0f};
std::atomic<int> s_shownMode{Off};

uint32_t next()
{
    s_random ^= s_random << 13;
    s_random ^= s_random >> 17;
    s_random ^= s_random << 5;
    return s_random;
}

// Uniform in [low, high).
float uniform(float low, float high)
{
    return low + (high - low) * float(next() >> 8) * (1.0f / 16777216.0f);
}

bool position(float* x, float* y)
{
    const reg32 player = rc::ld32(Players);
    const reg32 vehicle = player ? rc::ld32(player + PlayerVehicle) : 0;
    if (!vehicle)
        return false;
    *x = rc::ldf32(vehicle + VehiclePosition);
    *y = rc::ldf32(vehicle + VehiclePosition + 4);
    return true;
}

// Next segment: mostly straight, some turns, rare brief brakes.
void newSegment(float now)
{
    const uint32_t pick = next() % 100;
    if (pick < 6)
    {
        s_mode = Braking;
        s_steer = 0.0f;
        s_modeEnd = now + uniform(0.3f, 0.8f);
        return;
    }
    static const float Steers[] = { -1.0f, -0.5f, 0.5f, 1.0f };
    s_mode = Driving;
    s_steer = pick < 46 ? 0.0f : Steers[next() % 4];
    s_modeEnd = now + uniform(0.6f, 3.0f);
}

void straight(float now, float seconds)
{
    s_mode = Driving;
    s_steer = 0.0f;
    s_modeEnd = now + seconds;
}

bool stuck()
{
    if (s_sampleCount <= StuckSamples)
        return false;
    const Sample& a = s_samples[0];
    const Sample& b = s_samples[StuckSamples];
    return std::hypot(b.x - a.x, b.y - a.y) < StuckDistance;
}

void addSample(float x, float y)
{
    if (s_sampleCount <= StuckSamples)
    {
        s_samples[s_sampleCount++] = { x, y };
        return;
    }
    for (int i = 0; i < StuckSamples; ++i)
        s_samples[i] = s_samples[i + 1];
    s_samples[StuckSamples] = { x, y };
}

void update()
{
    float x, y;
    if (s_mode == Off || !position(&x, &y))
        return;
    const float now = rc::ldf32(WorldSeconds);
    if (now >= s_nextSample)
    {
        s_nextSample = now + SamplePeriod;
        addSample(x, y);
        if (s_sampleCount > 2)
        {
            const Sample& a = s_samples[s_sampleCount - 3];
            s_speed.store(std::hypot(x - a.x, y - a.y) / (2.0f * SamplePeriod), std::memory_order_relaxed);
        }
        if (s_mode != Reversing && stuck())
        {
            ++s_reversals;
            s_mode = Reversing;
            s_steer = s_steer > 0.0f ? -1.0f : s_steer < 0.0f ? 1.0f : (next() & 1 ? 1.0f : -1.0f);
            s_modeEnd = now + ReverseSeconds;
            s_sampleCount = 0;
        }
    }
    if (now >= s_modeEnd && s_mode == Reversing)
        straight(now, StraightSeconds * 0.5f);
    else if (now >= s_modeEnd)
        newSegment(now);
    s_x.store(x, std::memory_order_relaxed);
    s_y.store(y, std::memory_order_relaxed);
    s_shownMode.store(s_mode, std::memory_order_relaxed);
    if (now >= s_nextLog)
    {
        s_nextLog = now + LogPeriod;
        char text[96];
        describe(text, sizeof(text));
        pf::logInfo("drive: world time %.1f s, %s, %d reversals", now, text, s_reversals);
    }
}

// Guest thread, once per pad poll: the next frame's input.
void pollHook(x86::reg32* buttons, int* pov)
{
    RC_USE(buttons);
    RC_USE(pov);
    update();
    controls::PadSample pad = {};
    if (s_mode == Driving)
        pad.inputs = controls::bit(Throttle);
    else if (s_mode == Braking || s_mode == Reversing)
        pad.inputs = controls::bit(Brake);
    pad.sticks[0] = s_mode == Off ? 0.0f : s_steer;
    win32::Gamepad::setInjectedPad(pad);
}

}

void configure(uint32_t seed)
{
    s_random = seed ? seed : 1;
    win32::Gamepad::setPollHook(pollHook);
    pf::logInfo("drive: blind driver ready, seed %u: %s throttle, %s brake and reverse, left stick steering",
                unsigned(s_random), "R", "L");
}

void start()
{
    if (s_mode != Off)
        return;
    const float now = rc::ldf32(WorldSeconds);
    s_sampleCount = 0;
    s_nextSample = now;
    s_nextLog = now;
    straight(now, StraightSeconds);
    pf::logInfo("drive: driving from world time %.1f s", now);
}

void stop()
{
    if (s_mode == Off)
        return;
    s_mode = Off;
    s_shownMode.store(Off, std::memory_order_relaxed);
    win32::Gamepad::setInjectedPad(controls::PadSample());
    pf::logInfo("drive: stopped after %d reversals", s_reversals);
}

void describe(char* text, size_t size)
{
    snprintf(text, size, "car at (%.0f, %.0f) %.1f m/s %s", s_x.load(std::memory_order_relaxed),
             s_y.load(std::memory_order_relaxed), s_speed.load(std::memory_order_relaxed),
             ModeNames[s_shownMode.load(std::memory_order_relaxed)]);
}

}}
