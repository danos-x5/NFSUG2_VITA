#ifndef UG2_BOOT_H_
#define UG2_BOOT_H_

#include <lib/winapp.h>

// Direct boot into a race, and the virtual clock (boot.cpp).

namespace ug2 { namespace boot {

// Before guest code runs: race settings, guest writes and hooks.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_BOOT_H_ */
