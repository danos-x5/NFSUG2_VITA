#ifndef PLATFORM_MP3DEC_H_
#define PLATFORM_MP3DEC_H_

// MPEG-1 Layer III frame decoding on the Vita's hardware audio decoder (SceAudiodec), one frame per call.

#include <cstdint>

namespace pf { namespace mp3
{

class Decoder;

const int MaxDecoders = 6;

// Sets up the library once; false (logged) when unavailable.
bool init();
// A stereo decoder, or null when all are in use.
Decoder* create();
void destroy(Decoder* decoder);
// Forgets the previous frames, as at a stream start.
bool reset(Decoder* decoder);
// Frame to interleaved stereo; samples per channel, or -1 on error.
int decode(Decoder* decoder, const uint8_t* frame, uint32_t size, int16_t* pcm);
// The last decoder error code, for the log.
int lastError();

}}

#endif /* !PLATFORM_MP3DEC_H_ */
