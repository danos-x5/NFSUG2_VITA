#ifndef UG2_CONTROLSBAKE_H_
#define UG2_CONTROLSBAKE_H_

// Edits baked into the Controls screens' package copies (hidden objects, titles, row slots, bars, picture, instruction line).

#include "fepackage.h"

#include <cstdint>
#include <string>

namespace ug2 { namespace controlsbake {

const uint32_t TextConfigure = 0x8bab66ef, TextPressButton = 0x23cf49aa;     // "Configure your settings", the bind wait
const uint32_t PictureObject = 0xf1534c25;     // CONTROLLER_IMAGE
const uint32_t TitleGroupObject = 0x8d7e0c53;  // the group holding the title and the panel
const uint32_t TitleMasterObject = 0xa753c46c;  // TITLE_MASTER: the first row's label box
const int OptionsLines = 8, PcControllerLines = 7;     // lines the options screen (0x4CF750) and PC controller screen show

// The list's place from the package: TITLE_MASTER's top and height (the row step), and the last visible row's bottom.
struct ListLayout
{
    float top = 0, row = 0, bottom = 0;
    bool valid = false;
};
ListLayout listLayout(const fepackage::Package& p, int lines);
// The bottom edge of the inner panel box behind the list, in screen units; false when the package has none.
bool panelBottom(const fepackage::Package& p, float* bottom);
// An object's position (data x, y) from its package.
bool position(const fepackage::Package& p, uint32_t hash, float* x, float* y);
const int SlotModel = 12;                      // the last row slot of UI_Options.fng

struct Result
{
    size_t hidden = 0;
    int slots = 0;          // rows the copy can show
    int cloned = 0;         // slot objects copied from SlotModel
    int bars = 0;           // row bars fitted
    long instruction = -1;  // the instruction line object, -1 for none
    int steady = 0;         // looping Highlight scripts made steady (kept as HighlightFlash)
    std::string error;      // why the instruction line is missing
    float gap = 0;          // the PC screen's gap from its last row to its label, kept here
};

// The flashing highlight, kept under its own name for the bind wait.
uint32_t flashScript();
// CONTROLS_INSTRUCTION: the Customize instruction line's own object.
uint32_t instructionHash();
// Rows the package can show: every slot object up to n exists.
int countSlots(const fepackage::Package& p);
// The package the instruction line comes from, and the object.
const char* const InstructionDonor = "UI_Options_PC_Controller.fng";
const char* const InstructionLabel = "Instructions_Label";
// A copy of UI_Options.fng: the Controls list, or Customize with a row slot per entry and the instruction line from donor.
Result options(fepackage::Package& p, uint32_t title, bool customize, int entries, const fepackage::Package* donor = nullptr);
// A copy of UI_OptionsController.fng: Joypad Configuration.
Result joypad(fepackage::Package& p, uint32_t title);

}}

#endif /* !UG2_CONTROLSBAKE_H_ */
