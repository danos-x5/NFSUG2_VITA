#ifndef UG2_MOVIES_H_
#define UG2_MOVIES_H_

#include <lib/winapp.h>

// Movie playback through the platform video decoder instead of the game's VP6 decoder (movies.cpp).

namespace ug2 { namespace movies {

// Before guest code runs: reads the movies settings, runs movies_selftest, installs the hooks.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_MOVIES_H_ */
