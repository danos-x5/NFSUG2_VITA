// EALayer3 v1 granule parsing and MPEG-1 Layer III frame building. Each frame stands alone: main_data_begin 0, the
// smallest standard bit rate that holds both granules, zero padding.

#include "ealayer3.h"

#include <cstring>
#include <initializer_list>

namespace ug2 { namespace ealayer3
{

namespace
{

const uint32_t SideBits1 = 59;      // per granule and channel, MPEG-1
const uint32_t SideBits2 = 63;      // MPEG-2
const uint8_t  PcmInsert = 0xee;
const uint32_t Bitrates1[15] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
const uint32_t Rates1[3] = {44100, 48000, 32000};

struct Reader
{
    const uint8_t* data;
    uint32_t bit;

    uint32_t read(uint32_t count)
    {
        uint32_t value = 0;
        for (uint32_t i = 0; i < count; ++i, ++bit)
            value = (value << 1) | ((data[bit >> 3] >> (7 - (bit & 7))) & 1);
        return value;
    }
};

struct Writer
{
    uint8_t* data;
    uint32_t bit;

    void write(uint64_t value, uint32_t count)
    {
        for (uint32_t i = count; i-- > 0; ++bit)
        {
            if ((value >> i) & 1)
                data[bit >> 3] |= uint8_t(0x80 >> (bit & 7));
        }
    }

    // Zeroed destination; may read one byte past the source bits.
    void copy(const uint8_t* source, uint32_t sourceBit, uint32_t count)
    {
        for (; count >= 8; count -= 8, sourceBit += 8, bit += 8)
        {
            const uint32_t at = sourceBit >> 3, shift = sourceBit & 7;
            const uint8_t byte = shift ? uint8_t((source[at] << shift) | (source[at + 1] >> (8 - shift))) : source[at];
            const uint32_t to = bit >> 3, offset = bit & 7;
            data[to] |= uint8_t(byte >> offset);
            if (offset)
                data[to + 1] |= uint8_t(byte << (8 - offset));
        }
        Reader rest = {source, sourceBit};
        write(rest.read(count), count);
    }
};

}

bool parse(const uint8_t* data, Granule& granule)
{
    Reader reader = {data, 0};
    const uint32_t header = reader.read(8);
    granule.data = data;
    granule.version = uint8_t(header >> 6);
    granule.rateIndex = uint8_t((header >> 4) & 3);
    granule.mode = uint8_t((header >> 2) & 3);
    granule.modeExtension = uint8_t(header & 3);
    if (header == 0 || granule.version == 1 || granule.rateIndex == 3)
        return false;
    const bool mpeg1 = granule.version == 3;
    granule.channels = granule.mode == 3 ? 1 : 2;
    granule.index = uint8_t(reader.read(1));
    granule.scfsi = mpeg1 && granule.index ? reader.read(4 * granule.channels) : 0;
    const uint32_t rest = (mpeg1 ? SideBits1 : SideBits2) - 12;
    granule.mainBits = 0;
    for (uint32_t c = 0; c < granule.channels; ++c)
    {
        const uint32_t length = reader.read(12);
        const uint64_t high = reader.read(rest - 32);
        const uint64_t low = reader.read(32);
        granule.side[c] = (uint64_t(length) << rest) | (high << 32) | low;
        granule.mainBits += length;
    }
    granule.mainBit = reader.bit;
    granule.bytes = (reader.bit + granule.mainBits + 7) >> 3;
    return true;
}

const uint8_t* next(const Granule& granule)
{
    const uint8_t* p = granule.data + granule.bytes;
    if (*p == PcmInsert)
        p += 5 + ((uint32_t(p[3]) << 8) | p[4]) * 2u * granule.channels;
    return *p == 0 && p[1] != 0 ? p + 1 : nullptr;
}

uint32_t frame(const Granule* first, const Granule* second, uint8_t* out)
{
    const Granule* shape = first ? first : second;
    if (!shape || shape->version != 3)
        return 0;
    const uint32_t channels = shape->channels;
    const uint32_t sideBits = 9 + (channels == 1 ? 5 : 3) + 4 * channels + 2 * SideBits1 * channels;
    const uint32_t mainBits = (first ? first->mainBits : 0) + (second ? second->mainBits : 0);
    const uint32_t need = 4 + sideBits / 8 + (mainBits + 7) / 8;
    const uint32_t rate = Rates1[shape->rateIndex];
    uint32_t size = 0, bitrate = 0, padding = 0;
    for (uint32_t b = 1; b < 15; ++b)
    {
        for (uint32_t pad = 0; pad < 2; ++pad)
        {
            const uint32_t bytes = 144000 * Bitrates1[b] / rate + pad;
            if (bytes >= need && (!size || bytes < size))
            {
                size = bytes;
                bitrate = b;
                padding = pad;
            }
        }
    }
    if (!size || size > MaxFrameBytes)
        return 0;
    memset(out, 0, size);
    Writer writer = {out, 0};
    writer.write(0x7ff, 11);
    writer.write(3, 2);                 // MPEG-1
    writer.write(1, 2);                 // Layer III
    writer.write(1, 1);                 // no CRC
    writer.write(bitrate, 4);
    writer.write(shape->rateIndex, 2);
    writer.write(padding, 1);
    writer.write(0, 1);
    writer.write(shape->mode, 2);
    writer.write(shape->modeExtension, 2);
    writer.write(0, 1);
    writer.write(1, 1);                 // original
    writer.write(0, 2);
    writer.write(0, 9);                 // main_data_begin
    writer.write(0, channels == 1 ? 5 : 3);
    writer.write(second ? second->scfsi : 0, 4 * channels);
    for (const Granule* g : {first, second})
    {
        for (uint32_t c = 0; c < channels; ++c)
            writer.write(g ? g->side[c] : 0, SideBits1);
    }
    for (const Granule* g : {first, second})
    {
        if (g)
            writer.copy(g->data, g->mainBit, g->mainBits);
    }
    return size;
}

}}
