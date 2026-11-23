#ifndef UG2_PSBARPACKAGE_H_
#define UG2_PSBARPACKAGE_H_

#include "fepackage.h"

#include <string>

// Rebuilds UI_PC_Help_Bar.fng bytes: the PC art hidden, the PS2 bar group copied in from a screen package. No game access.

namespace ug2 { namespace psbar {

struct Built
{
    size_t copied = 0, resourcesAdded = 0, hiddenPc = 0;
    std::string error;
};

// Edits bar in place; on failure bar is left unchanged and Built::error says why.
bool buildBar(fepackage::Package& bar, const fepackage::Package& donor, Built* out);

}}

#endif /* !UG2_PSBARPACKAGE_H_ */
