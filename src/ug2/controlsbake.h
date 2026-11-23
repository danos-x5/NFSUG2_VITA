#ifndef UG2_CONTROLSBAKE_H_
#define UG2_CONTROLSBAKE_H_

// Edits baked into the Controls screens' package copies (hidden objects, titles, row slots, bars, picture, instruction line).

#include "fepackage.h"

#include <cstdint>

namespace ug2 { namespace controlsbake {

const uint32_t TextConfigure = 0x8bab66ef, TextPressButton = 0x23cf49aa;     // "Configure your settings", the bind wait
const uint32_t PictureObject = 0xf1534c25;     // CONTROLLER_IMAGE
const int SlotModel = 12;                      // the last row slot of UI_Options.fng

struct Result
{
    size_t hidden = 0;
    int slots = 0;          // rows the copy can show
    int cloned = 0;         // slot objects copied from SlotModel
    int bars = 0;           // row bars fitted
    long instruction = -1;  // the instruction line object, -1 for none
};

// CONTROLS_INSTRUCTION: the Customize instruction line's own object.
uint32_t instructionHash();
// Rows the package can show: every slot object up to n exists.
int countSlots(const fepackage::Package& p);
// A copy of UI_Options.fng: the Controls list, or Customize with a row slot per entry and the instruction line.
Result options(fepackage::Package& p, uint32_t title, bool customize, int entries);
// A copy of UI_OptionsController.fng: Joypad Configuration.
Result joypad(fepackage::Package& p, uint32_t title);

}}

#endif /* !UG2_CONTROLSBAKE_H_ */
