// Free-roam HUD touch zones: the player's polygons, drawn on a widescreen Vita screenshot, placed through the
// canvas mapping the HUD is drawn with.
#ifndef UG2_HUD_TOUCH_H_
#define UG2_HUD_TOUCH_H_

#include <lib/controls.h>

namespace ug2 { namespace hudtouch {

// How a HUD part is drawn: screen rectangle in display pixels, canvas x map clip = x * scale - offset.
struct DrawMap
{
    float x0, y0, x1, y1;
    float scale, offset;
};

// widescreen=on at 960x544: full screen, left-edge HUD placement.
inline DrawMap referenceMap()
{
    const float k = 544.0f / 960.0f / 0.75f;
    return { 0.0f, 0.0f, 960.0f, 544.0f, k / 320.0f, 1.0f };
}

inline win32::controls::ZonePoint toDisplay(const DrawMap& m, float canvasX, float canvasY)
{
    return { m.x0 + (canvasX * m.scale - m.offset + 1.0f) * 0.5f * (m.x1 - m.x0), m.y0 + canvasY / 480.0f * (m.y1 - m.y0) };
}

inline win32::controls::ZonePoint toCanvas(const DrawMap& m, float px, float py)
{
    return { ((px - m.x0) / (0.5f * (m.x1 - m.x0)) - 1.0f + m.offset) / m.scale, (py - m.y0) * 480.0f / (m.y1 - m.y0) };
}

// Vita screenshot 2026-11-24-060708, clockwise; edges horizontal, vertical or 45 degrees.
inline const win32::controls::ZonePolygon& referenceZone(win32::controls::HudZone zone)
{
    static const win32::controls::ZonePolygon Zones[win32::controls::HudZoneCount] = {
        { 4, { { 16, 331 }, { 207, 331 }, { 207, 529 }, { 16, 529 } } },
        { 6, { { 96, 133 }, { 109, 120 }, { 199, 120 }, { 199, 170 }, { 109, 170 }, { 96, 157 } } },
        { 6, { { 35, 40 }, { 199, 40 }, { 199, 112 }, { 105, 112 }, { 90, 127 }, { 35, 127 } } },
        { 6, { { 52, 177 }, { 65, 164 }, { 89, 164 }, { 102, 177 }, { 102, 267 }, { 52, 267 } } },
    };
    return Zones[zone];
}

// Where the screenshot shows the anchors: the ring's centre and the minimap's.
const float RingCentreX = 77.0f, RingCentreY = 146.0f, MinimapCentreX = 115.0f, MinimapCentreY = 430.0f;

// Screenshot polygon moved to where this map draws the HUD.
inline win32::controls::ZonePolygon placeZone(win32::controls::HudZone zone, const DrawMap& drawn)
{
    const win32::controls::ZonePolygon& ref = referenceZone(zone);
    const DrawMap reference = referenceMap();
    win32::controls::ZonePolygon out = ref;
    for (int i = 0; i < ref.count; ++i)
    {
        const win32::controls::ZonePoint c = toCanvas(reference, ref.points[i].x, ref.points[i].y);
        out.points[i] = toDisplay(drawn, c.x, c.y);
    }
    return out;
}

}}

#endif
