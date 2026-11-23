// The Controls screens' baked edits (controlsbake.h): pure package edits, logged by the caller.

#include "controlsbake.h"

#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

namespace ug2 { namespace controlsbake {

namespace {

using fepackage::nameHash;

const uint32_t TitleObject = 0x42adb44c;
const uint32_t RowBar = 0xa67c3fe2, RowBarRight = 0xd8fdd418, RowBarLeft = 0xd8fdd419;     // a row group's bar and end caps
const float BarCenter = 3.115f, BarWidth = 523.35f;
const float PictureShiftX = -37.0f, PictureWidth = 640.0f, PictureHeight = 320.0f;     // controls-overhaul's picture area
const float InstructionX = 37.0f, InstructionY = 293.0f;     // controls-overhaul's line, in the title group's units
const uint16_t TagData = 0x4153, TagTextId = 0x4853, TagBuffer = 0x6253, TagJustify = 0x6a53, TagText = 0x7453;
const char* const SlotPrefixes[] = { "OPTION_NAME_", "OPTION_DATA_", "OPTION_BACKING_", "LEFT_ARROW_", "RIGHT_ARROW_" };

void setFloat(std::vector<uint8_t>* data, size_t at, float value)
{
    if (data && data->size() >= at + 4)
        memcpy(&(*data)[at], &value, 4);
}

void common(fepackage::Package& p, Result* r, uint32_t title, std::initializer_list<uint32_t> hidden)
{
    for (uint32_t hash : hidden)
        r->hidden += p.hide(hash);
    for (size_t t : p.find(TitleObject))
        p.setTag32(t, TagTextId, title);
}

void roomForTexts(fepackage::Package& p, int rows)
{
    for (int n = 1; n <= rows; ++n)
    {
        for (const char* prefix : { "OPTION_NAME_", "OPTION_DATA_" })
        {
            for (size_t o : p.find(nameHash(prefix + std::to_string(n))))
            {
                if (p.tag32(o, TagBuffer) < 64)
                    p.setTag32(o, TagBuffer, 64);
            }
        }
    }
}

// A copy of the calibration text (Help's grey, font and size, no scripted colour), placed under the list.
long instructionLine(fepackage::Package& p)
{
    const std::vector<size_t> model = p.find(nameHash("ColorCal_Blurb"));
    if (model.empty())
        return -1;
    const size_t line = p.clone(model[0], instructionHash());
    p.setFlags(line, p.flags(line) & ~fepackage::FlagHidden);
    setFloat(p.tagData(line, TagData), 0x1c, InstructionX);
    setFloat(p.tagData(line, TagData), 0x20, InstructionY);
    p.setTag32(line, TagTextId, TextConfigure);
    p.setTag32(line, TagJustify, 1);
    if (p.tag32(line, TagBuffer) < 64)
        p.setTag32(line, TagBuffer, 64);
    if (std::vector<uint8_t>* text = p.tagData(line, TagText))
    {
        const std::string plain = "Configure your settings";
        text->assign((plain.size() + 1) * 2, 0);
        for (size_t i = 0; i < plain.size(); ++i)
            (*text)[2 * i] = uint8_t(plain[i]);
        text->resize((text->size() + 3) & ~size_t(3), 0);
    }
    return long(line);
}

}

uint32_t instructionHash()
{
    return nameHash("CONTROLS_INSTRUCTION");
}

int countSlots(const fepackage::Package& p)
{
    int n = 0;
    auto has = [&](int k)
    {
        for (const char* prefix : SlotPrefixes)
        {
            if (p.find(nameHash(prefix + std::to_string(k))).empty())
                return false;
        }
        return true;
    };
    while (has(n + 1))
        ++n;
    return n;
}

Result options(fepackage::Package& p, uint32_t title, bool customize, int entries)
{
    Result r;
    common(p, &r, title, { nameHash("ColorCal_Logo"), nameHash("ColorCal_Blurb"), nameHash("Colour_Calibration_Backing"),
                           nameHash("PLAYER"), 0x010cba23, 0x97b71dde, 0xc90dc4ab, 0x444969fd, 0x444969fe });
    if (customize)
    {
        for (const char* prefix : SlotPrefixes)
            r.cloned += p.extendSeries(prefix, SlotModel, entries);
        r.instruction = instructionLine(p);
    }
    roomForTexts(p, entries > 13 ? entries : 13);
    r.slots = countSlots(p);
    return r;
}

Result joypad(fepackage::Package& p, uint32_t title)
{
    Result r;
    common(p, &r, title, { nameHash("PLAYER"), 0x010cba23, 0x97b71dde, 0xc90dc4ab, 0x444969fd, 0x444969fe, 0xbaa4198e,
                           0xbaa4198f, 0xbaa41990, 0xbaa41991, 0xbaa41992, 0xbaa41993, 0xbaa41994, 0xbaa41995, 0xbaa41996,
                           0x0f274b7e, 0x0f274b7f, 0x0f274b80, 0x0f274b81, 0x0f274b82 });
    roomForTexts(p, 13);
    for (size_t group = 0; group < p.objectCount(); ++group)
    {
        if (p.type(group) != 5)
            continue;
        const std::vector<size_t> parts = p.subtree(group);
        if (parts.size() != 4 || p.hash(parts[1]) != RowBar || p.hash(parts[2]) != RowBarRight || p.hash(parts[3]) != RowBarLeft)
            continue;
        setFloat(p.tagData(parts[1], TagData), 0x1c, BarCenter);
        setFloat(p.tagData(parts[1], TagData), 0x38, BarWidth);
        setFloat(p.tagData(parts[2], TagData), 0x1c, BarCenter + BarWidth / 2 + 18);
        setFloat(p.tagData(parts[3], TagData), 0x1c, BarCenter - BarWidth / 2 - 18);
        ++r.bars;
    }
    for (size_t image : p.find(PictureObject))
    {
        std::vector<uint8_t>* sa = p.tagData(image, TagData);
        if (!sa || sa->size() < 0x40)
            continue;
        float x;
        memcpy(&x, &(*sa)[0x1c], 4);
        setFloat(sa, 0x1c, x + PictureShiftX);
        setFloat(sa, 0x38, PictureWidth);
        setFloat(sa, 0x3c, PictureHeight);
    }
    r.slots = countSlots(p);
    return r;
}

}}
