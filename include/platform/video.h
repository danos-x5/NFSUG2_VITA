#ifndef PLATFORM_VIDEO_H_
#define PLATFORM_VIDEO_H_

// Video file decoding for the movie backend: the Vita's hardware H.264 decoder on a worker thread; the host build
// pipes ffmpeg when it is installed.

#include <cstdint>

namespace pf { namespace video
{

struct Info
{
    int width = 0;
    int height = 0;
    int frames = 0;
};

class Stream;

// Starts decoding an MP4 file; null (logged) when this build cannot decode it.
Stream* open(const char* path, Info* info);
// The next frame in display order as planar I420 (Y, U, V, pitch width, width/2), valid until the next call. Blocks
// until it is decoded; false at the end of the stream.
bool next(Stream* stream, const uint8_t** data);
void close(Stream* stream);

}}

#endif
