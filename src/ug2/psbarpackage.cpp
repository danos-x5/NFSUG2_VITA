// Builds the PS2 bottom bar package from the PC help bar package and a screen package that has the PS2 bar group.

#include "psbarpackage.h"
#include "psbarlogic.h"
#include <cstring>

namespace ug2 { namespace psbar {

namespace {

const uint16_t TagData = 0x4153;     // an object's start data
const size_t DataY = 0x20;           // its y in that data
const float IconDrop = 1.0f / 1.1333f;   // 1 Vita pixel lower than the PS2 places the bar's button icons

}

bool buildBar(fepackage::Package& bar, const fepackage::Package& donor, Built* out)
{
    fepackage::Package work = bar;
    const size_t resourcesBefore = work.resourceNames().size();
    size_t hidden = 0;
    for (size_t i = 0; i < work.objectCount(); ++i)
    {
        if (!(work.flags(i) & fepackage::FlagHidden))
        {
            work.setFlags(i, work.flags(i) | fepackage::FlagHidden);
            ++hidden;
        }
    }
    for (size_t i : work.find(PcRealGroup))
        work.setHash(i, PcRealGroup + 1);
    size_t root = donor.objectCount();
    for (size_t i : donor.find(PsRoot))
    {
        if (donor.parent(i) == 0)
            root = i;
    }
    if (root == donor.objectCount())
    {
        out->error = "the donor has no top-level PS2 bar group";
        return false;
    }
    const std::vector<size_t> added = work.copyFrom(donor, root, Salt, &out->error);
    if (added.empty())
        return false;
    for (uint32_t hash : { PsHelpIcon, PsSecondIcon })
    {
        for (size_t i : work.find(hash))
        {
            std::vector<uint8_t>* data = work.tagData(i, TagData);
            if (data && data->size() >= DataY + 4)
            {
                float y;
                memcpy(&y, &(*data)[DataY], 4);
                y += IconDrop;
                memcpy(&(*data)[DataY], &y, 4);
            }
        }
    }
    for (uint32_t hash : { PsMoney, PsDpad, PsSecondWord, PsSecondIcon })
        work.hide(hash);
    work.hide(PsRoot);
    out->copied = added.size();
    out->hiddenPc = hidden;
    out->resourcesAdded = work.resourceNames().size() - resourcesBefore;
    bar = work;
    return true;
}

}}
