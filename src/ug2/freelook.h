#ifndef UG2_FREELOOK_H_
#define UG2_FREELOOK_H_

#include <lib/winapp.h>

// Free look for the race chase camera: a stick's direction sets the view yaw, down is the game's own Look Back.

namespace ug2 { namespace freelook {

// Free look stick: x right, y down, -1..1; guest thread.
void setInput(float x, float y);
// True while free look drives the race camera.
bool live();

// Chase camera update starts; returns the previous camera.
x86::reg32 enter(x86::reg32 camera);
void leave(x86::reg32 saved);

// Turns the chase camera before its look-at matrix.
void beforeLookAt(win32::WinApplication* app, x86::CPU& cpu);

}}

#endif /* !UG2_FREELOOK_H_ */
