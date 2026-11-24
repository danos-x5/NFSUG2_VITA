#ifndef UG2_CONTROLSROWS_H_
#define UG2_CONTROLSROWS_H_

// The Controls and Joypad Configuration rows as data: Control Type cycling, the stick shape rows' steps and limits,
// and the shape's cfg keys. No game access, so the host tests cover it.

#include <lib/controls.h>

namespace ug2 { namespace controlsrows {

// Controls_Type 0..3 (Type 1..4), 4 Custom; wraps both ways.
int nextType(int type, int direction);

enum StickPart { PartDeadZone, PartOuterLimit, PartSensitivity, PartCount };
struct StickLimits
{
    int low, high, step;    // percent
};
StickLimits stickLimits(StickPart part);
// The part's value in whole percent.
int stickPercent(const win32::controls::StickShape& shape, StickPart part);
// One step left (-1) or right (+1), held at the limits.
win32::controls::StickShape stepStick(const win32::controls::StickShape& shape, StickPart part, int direction);
// The stick's shape into its three cfg keys, then the cfg saved.
void saveShape(bool right);

}}

#endif /* !UG2_CONTROLSROWS_H_ */
