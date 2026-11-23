#ifndef UG2_CONTROLSSHELL_H_
#define UG2_CONTROLSSHELL_H_

#include <lib/winapp.h>

// Options > Controls as a navigable shell on own screens (Controls, Joypad Configuration, Customize); no logic yet.

namespace ug2 { namespace controlsshell {

// Before guest code runs, after psprompt::configure.
void configure(win32::WinApplication& app);
// Per frame on the game thread.
void frame(win32::WinApplication* app, x86::CPU& cpu);

}}

#endif /* !UG2_CONTROLSSHELL_H_ */
