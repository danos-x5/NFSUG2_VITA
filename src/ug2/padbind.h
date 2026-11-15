#ifndef UG2_PADBIND_H_
#define UG2_PADBIND_H_

#include <lib/winapp.h>

// Pad bindings: the port's defaults, then the game's own Controls screen.

namespace ug2 { namespace padbind {

// Before guest code runs: installs the defaults, hooks the message pump.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_PADBIND_H_ */
