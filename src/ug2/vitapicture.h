#ifndef UG2_VITAPICTURE_H_
#define UG2_VITAPICTURE_H_

// Placeholder picture of the Joypad Configuration screen: the Vita body layer of diagram_art.h as DXT3. Host tested.

#include <cstdint>
#include <vector>

namespace ug2 { namespace vitapicture {

int width();
int height();
// RGBA, straight alpha: the body layer on a clear canvas.
std::vector<uint8_t> bodyRgba();
// DXT3 data of an RGBA picture (width and height multiples of 4).
std::vector<uint8_t> encodeDxt3(const uint8_t* rgba, int width, int height);

}}

#endif /* !UG2_VITAPICTURE_H_ */
