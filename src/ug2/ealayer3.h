#ifndef UG2_EALAYER3_H_
#define UG2_EALAYER3_H_

// EALayer3 v1 granules (EA SCHl streams, codec 0x17) and their repacking into standard MPEG-1 Layer III frames, bit
// for bit, without decoding.

#include <cstdint>

namespace ug2 { namespace ealayer3
{

struct Granule
{
    const uint8_t* data = nullptr;  // header byte
    uint32_t bytes = 0;             // header byte to the byte-aligned end
    uint8_t  version = 0;           // MPEG version field: 3 MPEG-1, 2 MPEG-2, 0 MPEG-2.5
    uint8_t  rateIndex = 0;
    uint8_t  mode = 0;
    uint8_t  modeExtension = 0;
    uint8_t  index = 0;             // granule 0 or 1 of an MPEG-1 frame
    uint8_t  channels = 0;
    uint32_t scfsi = 0;             // granule 1 only
    uint64_t side[2] = {};          // per channel: part2_3_length and the rest of its side info
    uint32_t mainBit = 0;           // first main data bit, counted from data
    uint32_t mainBits = 0;
};

const uint32_t MaxFrameBytes = 1441;    // 320 kbit/s at 32 kHz, padded
const uint32_t FrameSamples = 1152;
const uint32_t GranuleSamples = 576;

// Parses the granule at data (its header byte); false if invalid.
bool parse(const uint8_t* data, Granule& granule);

// Next granule past a PCM insert, or null at block end.
const uint8_t* next(const Granule& granule);

// Builds an MPEG-1 frame (null granules empty); 0 when too big.
uint32_t frame(const Granule* first, const Granule* second, uint8_t* out);

}}

#endif /* !UG2_EALAYER3_H_ */
