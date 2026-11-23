#ifndef UG2_FSAA_H_
#define UG2_FSAA_H_

// Anti-Aliasing: the game's g_FSAALevel (Display menu, registry) is 0 off, 1 2x, 2 4x; the port's renderer multisamples.

#include <cstdint>

namespace ug2 { namespace fsaa {

// Renderer samples for a level.
inline uint32_t samples(int32_t level)
{
    return level <= 0 ? 1 : level == 1 ? 2 : 4;
}

// The level after Left (back) or Right/Accept; 0..cap wraps.
inline int32_t step(int32_t level, int32_t cap, bool back)
{
    if (cap <= 0)
        return level;
    level = level < 0 ? 0 : level > cap ? cap : level;
    if (back)
        return level == 0 ? cap : level - 1;
    return level == cap ? 0 : level + 1;
}

// The row's value text; null for the game's own OFF text.
inline const char* valueText(int32_t level)
{
    return level <= 0 ? nullptr : level == 1 ? "2x" : "4x";
}

}}

#endif /* !UG2_FSAA_H_ */
