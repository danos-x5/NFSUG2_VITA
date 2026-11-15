#ifndef PLATFORM_MP4_H_
#define PLATFORM_MP4_H_

// Minimal MP4 reader: the first H.264 video track's sizes, sample positions and parameter sets.

#include <cstdint>
#include <cstdio>
#include <vector>

namespace pf { namespace mp4
{

struct Track
{
    int width = 0;
    int height = 0;
    int nalLength = 4;                  // bytes of each NAL size prefix in a sample
    std::vector<uint8_t> parameterSets; // SPS and PPS as Annex B (start codes)
    std::vector<uint64_t> offsets;      // file position of each sample
    std::vector<uint32_t> sizes;
};

// False (logged) when the file is not an MP4 with an H.264 video track.
bool open(FILE* file, Track* track);

}}

#endif
