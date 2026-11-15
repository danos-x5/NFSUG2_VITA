#ifndef UG2_SCENERY_H_
#define UG2_SCENERY_H_

#include <cstdint>

// Scenery small-object cull (world_min_pixels, reflection_min_pixels), distant LOD (world_lod_pixels) and the scene
// log (scene_log=on with timing_log=on): scenery counts by width, cull, draw and flush times.

namespace ug2 { namespace scenery {

const int WidthBands = 8;   // below 2, 4, 8, 16, 32, 64, 128 pixels, and the rest

enum Phase { PhaseCull, PhaseDraw, PhaseFlush, PhaseCount };     // 0x5d69b0, 0x5d6ad0, 0x5cf420

// Sums since start-up (scale aside), written by the game thread only.
struct Counters
{
    uint32_t tested[2];                 // instances reaching the size test: player views, other views
    uint32_t engineCulled[2];           // rejected by the game's own size limits
    uint32_t smallCulled[2];            // rejected by world_min_pixels / reflection_min_pixels
    uint32_t smallPolys[2];
    uint32_t lowered, loweredFrom, loweredTo;   // world_lod_pixels: instances, polys before and after
    uint32_t drawn[2][WidthBands];
    uint32_t polys[2][WidthBands];      // model field 0x14 of the drawn ones
    uint32_t calls[PhaseCount];
    uint32_t us[PhaseCount];
    uint64_t instructions[PhaseCount];  // host builds only
    uint64_t threadInstructions;        // the game thread's count at its last timed call
    float scale;                        // the player view's pixel scale, last seen
};

struct Settings
{
    float mainMinPixels;
    float otherMinPixels;
    float lodPixels;    // world_lod_pixels: narrower scenery uses its reflection model in the player views
    uint32_t lodMinTris;        // world_lod_min_tris: normal models with fewer triangles keep their model
    uint32_t lodMaxPercent;     // world_lod_max_percent: the reflection model's triangles, at most this share
    float lodMinWidth;          // world_lod_min_width: metres; narrower types keep their model
    bool lodSigns;              // world_lod_signs: off keeps every sign type (XS_ names) on its normal model
    bool keepLights;    // world_min_pixels_lights: models with light markers are never culled
    bool log;
};

extern Settings g_settings;
extern Counters g_counters;

// Before guest code runs: reads the keys, starts the scene log.
void configure();

struct Stamp
{
    uint32_t us;
    uint64_t instructions;
};

// Scene log: the calling thread's time and instruction count.
Stamp stamp();
// Scene log: adds the time since start to a phase.
void add(Phase phase, const Stamp& start);

// Runs body, timed as a phase while the scene log is on.
template <typename Body>
inline void timed(Phase phase, Body body)
{
    if (__builtin_expect(!g_settings.log, 1))
    {
        body();
        return;
    }
    const Stamp start = stamp();
    body();
    add(phase, start);
}

inline int widthBand(float pixels)
{
    int band = 0;
    for (float edge = 2.0f; band < WidthBands - 1 && pixels >= edge; edge *= 2.0f)
        ++band;
    return band;
}

}}

#endif /* !UG2_SCENERY_H_ */
