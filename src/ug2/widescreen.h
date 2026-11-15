#ifndef UG2_WIDESCREEN_H_
#define UG2_WIDESCREEN_H_

// Widescreen (docs/widescreen.md): display-size back buffer, centred 2D canvas, HUD anchoring, hor+ projection.

#include <cstdint>

namespace ug2 { namespace widescreen {

// Screen anchor of a HUD element.
enum class Place { Centre, Left, Right, Full };

// Clip x = x * scale - offset of the 640x480 canvas.
struct Map
{
    float scale, offset;
};
// k: the part of the screen width the 4:3 canvas spans.
Map placeMap(Place place, double k);

// Anchor of a HUD unit from its canvas extent.
Place placeOf(float x0, float x1);

}}

#endif /* !UG2_WIDESCREEN_H_ */
