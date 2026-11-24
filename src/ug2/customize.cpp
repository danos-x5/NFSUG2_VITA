// The Customize screen as data: see customize.h.

#include "customize.h"

#include <cmath>

namespace ug2 { namespace customize {

namespace {

namespace data = controlsdata;

const char* const Tokens[data::ControlCount] =
{
    "BIND_TRIANGLE", "BIND_CIRCLE", "BIND_CROSS", "BIND_SQUARE", "BIND_L", "BIND_R", "BIND_DPAD_UP", "BIND_DPAD_DOWN",
    "BIND_DPAD_LEFT", "BIND_DPAD_RIGHT", "BIND_LSTICK", "BIND_RSTICK", "BIND_REAR_UL", "BIND_REAR_UR", "BIND_REAR_LL",
    "BIND_REAR_LR",
};

// PS2 button index to our control, -1 where none binds.
const int ButtonControls[data::ButtonCount] =
{
    data::CtlTriangle, data::CtlCircle, data::CtlCross, data::CtlSquare, data::CtlRearL2, data::CtlRearR2, data::CtlL,
    data::CtlR, -1, -1, data::CtlRearL3, data::CtlRearR3, data::CtlDpadUp, data::CtlDpadRight, data::CtlDpadDown,
    data::CtlDpadLeft,
};

bool worldEntry(Entry e)
{
    return e == data::EntryEnterShop || e == data::EntryWorldMap || e == data::EntryReadSms || e == data::EntryCareerStats;
}

float magnitude(const float sticks[4], int side)
{
    return std::sqrt(sticks[2 * side] * sticks[2 * side] + sticks[2 * side + 1] * sticks[2 * side + 1]);
}

}

std::string entryValue(const data::Binds& binds, Entry entry)
{
    const unsigned mask = data::controlsOf(binds, entry);
    std::string value;
    for (int c = 0; c < data::ControlCount; ++c)
    {
        if (mask & (1u << c))
            value += std::string("$") + Tokens[c] + "$";
    }
    if (value.empty() && worldEntry(entry))
        value = "Touchscreen";
    return value;
}

uint32_t armAction(Entry entry)
{
    const uint32_t action = data::entryAction(entry);
    return action ? action : SpareAction;
}

Verdict onButton(Entry entry, int button, Control* control)
{
    if (button == ButtonStart)
        return Cancel;
    if (button < 0 || button >= data::ButtonCount || ButtonControls[button] < 0 || entry == data::EntryFreeLook)
        return Ignore;
    *control = Control(ButtonControls[button]);
    return Bind;
}

Verdict onPadFrame(uint32_t inputs)
{
    return inputs & win32::controls::bit(win32::controls::Start) ? Cancel : Ignore;
}

bool touchReachesGame(bool inputHeld)
{
    return !inputHeld;
}

Verdict onAxis(Entry entry, int32_t axisId, Control* control)
{
    const Control stick = axisId == data::AxisX || axisId == data::AxisY ? data::CtlLeftStick :
                          axisId == data::AxisZ || axisId == data::AxisRz ? data::CtlRightStick : data::ControlCount;
    const data::Binds none = {};
    if (stick == data::ControlCount || watchesSticks(entry) || !data::entryTakes(none, entry, stick))
        return Ignore;
    *control = stick;
    return Bind;
}

bool watchesSticks(Entry entry)
{
    return entry == data::EntryShiftUp || entry == data::EntryShiftDown || entry == data::EntryFreeLook;
}

Verdict onSticks(StickWatch* watch, Entry entry, const float sticks[4], Control* control)
{
    if (!watchesSticks(entry))
        return Ignore;
    for (int side = 0; side < 2; ++side)
    {
        const Control stick = side ? data::CtlRightStick : data::CtlLeftStick;
        const data::Binds none = {};
        if (!data::entryTakes(none, entry, stick))
            continue;
        const float m = magnitude(sticks, side);
        if (m < StickOff)
            watch->centred[side] = true;
        else if (watch->centred[side] && m > StickOn)
        {
            *control = stick;
            return Bind;
        }
    }
    return Ignore;
}

bool released(uint32_t inputs, const float sticks[4])
{
    namespace in = win32::controls;
    const uint32_t rear = in::bit(in::RearL2) | in::bit(in::RearR2) | in::bit(in::RearL3) | in::bit(in::RearR3);
    return (inputs & ~rear) == 0 && magnitude(sticks, 0) < StickOff && magnitude(sticks, 1) < StickOff;
}

}}
