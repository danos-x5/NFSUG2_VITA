#ifndef UG2_CUSTOMIZE_H_
#define UG2_CUSTOMIZE_H_

// The Customize screen as data: entry values, bind wait verdicts on PC captures, our stick watch and the release
// guard. Host tested.

#include "controlsdata.h"

#include <cstdint>
#include <string>

namespace ug2 { namespace customize {

using controlsdata::Control;
using controlsdata::Entry;

// The entry's value: icon tokens, "Touchscreen", or empty for None.
std::string entryValue(const controlsdata::Binds& binds, Entry entry);

const uint32_t SpareAction = 11;    // armed for Free Look: no race action uses it
// The action the PC capture (0x5C2100) is armed with.
uint32_t armAction(Entry entry);

enum Verdict { Ignore, Bind, Cancel };
const int ButtonSelect = 8, ButtonStart = 9;
// A button the PC capture found (0x5B8460), by PS2 index.
Verdict onButton(Entry entry, int button, Control* control);
// An axis the PC capture found (0x5B8250), by axis id.
Verdict onAxis(Entry entry, int32_t axisId, Control* control);

// Our own look at the pad each frame of the wait: Start cancels whatever the PC capture is doing, so a capture that keeps
// ending (a stuck Escape, a hat at rest) can never trap the player.
Verdict onPadFrame(uint32_t inputs);
// Front touch reaches the menu only when no bind wait holds the input.
bool touchReachesGame(bool inputHeld);

// Shift pair and Free Look: our watch takes their sticks.
bool watchesSticks(Entry entry);
const float StickOn = 0.7f, StickOff = 0.3f;
struct StickWatch
{
    bool centred[2] = { false, false };     // left, right: seen under StickOff since the wait began
};
// A stick counts once centred, then pushed past StickOn.
Verdict onSticks(StickWatch* watch, Entry entry, const float sticks[4], Control* control);

// Release guard: nothing held but rear touch, sticks centred.
bool released(uint32_t inputs, const float sticks[4]);

}}

#endif /* !UG2_CUSTOMIZE_H_ */
