#ifndef UG2_OWNSCREENS_H_
#define UG2_OWNSCREENS_H_

#include "fepackage.h"

#include <lib/winapp.h>

#include <functional>
#include <string>

// Port-owned FEng screens: renamed copies of FRONTB packages with baked edits, and a screen table the game reads first.

namespace ug2 { namespace ownscreens {

typedef std::function<void(fepackage::Package&)> Edit;

// Queues a renamed FRONTB package copy, registered after FRONTB loads.
void addCopy(const std::string& source, const std::string& name, const Edit& edit);
// Port table entry: UI_Options.fng's fields with this constructor.
void addScreen(const std::string& name, x86::reg32 constructor);
// The name as a permanent guest string.
x86::reg32 guestName(const std::string& name);
bool registered(const std::string& name);
// A FRONTB package as the game registered it, read from its chunk; false when not loaded or unreadable.
bool loadPackage(const std::string& name, fepackage::Package* out);
// Per frame on the game thread: registers the pending copies.
void frame(win32::WinApplication* app, x86::CPU& cpu);
// Our entry for a package hash, else 0.
x86::reg32 entryOf(x86::reg32 hash);

}}

#endif /* !UG2_OWNSCREENS_H_ */
