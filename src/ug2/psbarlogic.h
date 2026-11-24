#ifndef UG2_PSBARLOGIC_H_
#define UG2_PSBARLOGIC_H_

// The PS2 bottom bar as data: its two prompts and where they sit. No game access, so the host tests cover it.

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ug2 { namespace psbar {

const uint32_t HelpText = 0x00169268;

// A prompt: the language text, and the front-end button the PS2 shows before it.
struct Prompt
{
    uint32_t    text;
    const char* token;      // JOY_EVENT name without the dollar signs
};

// The messages the PC bar items send (from 0x52D170) and the front-end button the PS2 shows for them.
const uint32_t MsgAccept = 0x406415E3, MsgBack = 0x911AB364, MsgButton0 = 0xC519BFBF, MsgButton1 = 0xC519BFC0;

// Button token for an item message, nullptr when the PS2 bar never shows it (Back, Button0, unnamed messages).
inline const char* tokenForMessage(uint32_t message)
{
    if (message == MsgAccept)
        return "JOY_EVENT_FENG_SELECT";
    if (message == MsgButton1)
        return "JOY_EVENT_FENG_BUTTON1";
    return nullptr;
}

// Bits the PS2 bar never shows: Quit Game (user rule) and EA Messenger (no EA online on the Vita).
const uint32_t DroppedBits = 0x1 | 0x4000;

// Prompts of our own screens by name, overriding the PC flags: Remove on the Controls Customize screen is Square + 0x7758F62D.
struct ScreenPrompt
{
    const char* screen;
    Prompt      prompt;
};

const ScreenPrompt ScreenPrompts[] =
{
    { "UI_ControlsCustomize.fng", { 0x7758F62D, "JOY_EVENT_FENG_BUTTON1" } },
};

// Bits the PC bar fill never gives a text.
inline bool bitHasText(int bit)
{
    return !(bit == 15 || bit == 16 || bit == 17 || bit == 31);
}

// The second prompt of a screen: a table row, else the first flag bit whose PC message has a PS2 button. Square actions come
// before Cross ones, as the PS2 shows Customize rather than Continue on car select.
template <typename MessageOfBit, typename TextOfBit>
Prompt secondPrompt(const char* screen, uint32_t flags, MessageOfBit messageOfBit, TextOfBit textOfBit)
{
    for (const ScreenPrompt& s : ScreenPrompts)
    {
        if (s.prompt.text && screen && strcmp(s.screen, screen) == 0)
            return s.prompt;
    }
    Prompt cross = { 0, nullptr };
    for (int bit = 0; bit < 32; ++bit)
    {
        const uint32_t mask = 1u << bit;
        if (!(flags & mask) || (mask & DroppedBits) || !bitHasText(bit))
            continue;
        const char* token = tokenForMessage(messageOfBit(mask));
        if (!token)
            continue;
        if (strcmp(token, "JOY_EVENT_FENG_BUTTON1") == 0)
            return { textOfBit(mask), token };
        if (!cross.token)
            cross = { textOfBit(mask), token };
    }
    return cross;
}

// The PS2 bar group of the screen packages (UI_OptionsController.fng) copied into UI_PC_Help_Bar.fng. Hashes the bar package
// already has (the PC's name, money, d-pad group and panels) get a salt on the copy.
const uint32_t Salt = 0x5A5A5A5A;
const uint32_t PsRoot = 0x02DDF58C, PsName = 0xEB406FEC ^ Salt, PsMoney = 0xEB3A688A ^ Salt, PsDpad = 0x2BAC0CEE ^ Salt;
const uint32_t PsHelpWord = 0x00169268, PsHelpIcon = 0x35E8F0B2, PsSecondWord = 0x010CBA23, PsSecondIcon = 0x97B71DDE;
const uint32_t PcRealGroup = 0x3B949CD0;    // the PC art group the fill shows; renamed so the fill finds nothing

// The text a prompt shows: its button token, then the language word.
inline void promptLine(const Prompt& prompt, const char* word, char* out, size_t size)
{
    snprintf(out, size, "$%s$ %s", prompt.token, word);
}

}}

#endif /* !UG2_PSBARLOGIC_H_ */
