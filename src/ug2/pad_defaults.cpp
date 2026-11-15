// UG2 default bindings for the EMS PS2 adapter (src/lib/controls.cpp reports it as on Windows). The game copies
// them into a new input slot; a profile's saved race map replaces them. Table: docs/pad-binding.md.

#include "pad_defaults.h"

#include <recomp.h>

namespace ug2 { namespace pad_defaults {

namespace {

using x86::reg32;

const reg32 EmsEntry = 0x007FE8A8;          // built-in table, 0xB0 apart: GUID, 20 race actions, 20 menu actions
const reg32 EmsRace = EmsEntry + 0x10;
const reg32 EmsMenu = EmsEntry + 0x60;

// By EMS button: Triangle, Circle, Cross, Square, L2, R2, L1, R1, Select, Start, L3, R3, d-pad up, right, down,
// left, then the four hat directions (the adapter has no hat).
const reg32 Race[20] = {
    7,  6,  5,  8,      // shift up, nitrous, handbrake, shift down
    8,  7,  2,  1,      // shift down, shift up, brake/reverse, throttle
    14, 12, 9,  10,     // reset car, pause, look back, camera
    16, 15, 18, 17,     // world map, enter race/shop, career statistics, read SMS
    0,  0,  0,  0,
};
const reg32 Menu[20] = {
    35, 34, 31, 37,     // BUTTON1, back, accept, BUTTON3
    0,  0,  38, 39,     // L and R triggers
    0,  36, 0,  0,      // START
    27, 30, 28, 29,     // up, right, down, left
    0,  0,  0,  0,
};

}

void install()
{
    for (reg32 i = 0; i < 20; ++i)
    {
        rc::st32(EmsRace + 4 * i, Race[i]);
        rc::st32(EmsMenu + 4 * i, Menu[i]);
    }
}

}}
