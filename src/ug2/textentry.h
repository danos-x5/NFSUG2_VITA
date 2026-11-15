#ifndef UG2_TEXTENTRY_H_
#define UG2_TEXTENTRY_H_

#include <lib/winapp.h>
#include <cstdint>

// Text entry: watches the game's line editor and hands its field to the on-screen keyboard.

namespace ug2 { namespace textentry {

// Before guest code runs: hooks the game's message pump to poll the editor.
void configure(win32::WinApplication& app);

// Characters besides letters and digits the editor of this type accepts (0x4FF310).
const char* extraChars(uint32_t type);
// Dialog title for an editor type.
const char* title(uint32_t type);

}}

#endif /* !UG2_TEXTENTRY_H_ */
