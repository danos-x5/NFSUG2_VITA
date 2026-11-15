#ifndef UG2_PADBIND_H_
#define UG2_PADBIND_H_

#include <lib/winapp.h>

// Pad bindings: the game decides (pad_layout=vita, game) or a forced port layout (development branch: docs/pad-binding.md).

namespace ug2 { namespace padbind {

// Before guest code runs: applies pad_layout and hooks the message pump.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_PADBIND_H_ */
