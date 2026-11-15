#ifndef UG2_PAD_DEFAULTS_H_
#define UG2_PAD_DEFAULTS_H_

// The port's default bindings for UG2: written into the game's EMS adapter table, so saved profiles and rebinds win.

namespace ug2 { namespace pad_defaults {

// Before guest code runs: replaces the EMS race and menu defaults.
void install();

}}

#endif /* !UG2_PAD_DEFAULTS_H_ */
