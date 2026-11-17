#ifndef UG2_TEXTURES_H_
#define UG2_TEXTURES_H_

#include <lib/winapp.h>

// The game's queue of texture releases, emptied each frame (texture_release_all) and before a world load
// (texture_load_drain), and the texture_retry counts.

namespace ug2 { namespace textures {

// Before guest code runs, on the game thread.
void configure(win32::WinApplication& app);
// Before 0x5CE170 creates a record's texture.
void creating();
// A texture_retry creation (sub_5bcc30) and whether it worked.
void retried(bool created);

}}

#endif /* !UG2_TEXTURES_H_ */
