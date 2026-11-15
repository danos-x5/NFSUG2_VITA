#ifndef UG2_HITCH_H_
#define UG2_HITCH_H_

#include <lib/winapp.h>

// Hitch log (hitch_log=on): a line per present over 41.7 ms with its dominant cause, 5 s window totals while racing,
// a summary per race. Kept in memory, written outside races.

namespace ug2 { namespace hitch {

// Before guest code runs, on the game thread: hooks the frame and the timed load calls.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_HITCH_H_ */
