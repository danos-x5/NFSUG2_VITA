#ifndef UG2_FRONTEND_H_
#define UG2_FRONTEND_H_

#include <lib/winapp.h>

// Front-end state log (game state, boot-flow and FEng screen commands) and scripted pad input (boot_script).

namespace ug2 { namespace frontend {

// Before guest code runs: installs the hooks when the front end runs, or boot_script is set.
void configure(win32::WinApplication& app);
// The game's rendered frame count.
uint32_t frameCounter();
// Logs the game's file opens (and memory log world loads) without the rest of the front-end hooks.
void hookFileOpens(win32::WinApplication& app);

}}

#endif /* !UG2_FRONTEND_H_ */
