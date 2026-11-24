// The Controls rows as data: see controlsrows.h.

#include "controlsrows.h"

#include <platform/config.h>

#include <cmath>
#include <string>

namespace ug2 { namespace controlsrows {

namespace {

const int TypeCount = 5;

float* field(win32::controls::StickShape* s, StickPart part)
{
    return part == PartDeadZone ? &s->deadZone : part == PartOuterLimit ? &s->saturation : &s->sensitivity;
}

}

int nextType(int type, int direction)
{
    const int t = type < 0 || type >= TypeCount ? 0 : type;
    return ((t + (direction < 0 ? -1 : direction > 0 ? 1 : 0)) % TypeCount + TypeCount) % TypeCount;
}

StickLimits stickLimits(StickPart part)
{
    switch (part)
    {
    case PartDeadZone:   return { 0, 50, 1 };
    case PartOuterLimit: return { 50, 100, 1 };
    default:             return { 0, 100, 5 };
    }
}

int stickPercent(const win32::controls::StickShape& shape, StickPart part)
{
    win32::controls::StickShape s = shape;
    return int(std::lround(*field(&s, part) * 100.0f));
}

win32::controls::StickShape stepStick(const win32::controls::StickShape& shape, StickPart part, int direction)
{
    const StickLimits l = stickLimits(part);
    int v = stickPercent(shape, part) + (direction < 0 ? -l.step : direction > 0 ? l.step : 0);
    v = v < l.low ? l.low : v > l.high ? l.high : v;
    win32::controls::StickShape s = shape;
    *field(&s, part) = float(v) / 100.0f;
    return s;
}

void saveShape(bool right)
{
    const win32::controls::StickShape s = win32::controls::stickShape(right);
    const std::string side = right ? "right_stick_" : "left_stick_";
    const char* const keys[PartCount] = { "deadzone", "saturation", "sensitivity" };
    for (int part = 0; part < PartCount; ++part)
        pf::Config::set((side + keys[part]).c_str(), std::to_string(stickPercent(s, StickPart(part))).c_str());
    pf::Config::save();
}

}}
