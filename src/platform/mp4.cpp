// Minimal MP4 reader: finds the H.264 video track in moov and lists its samples.

#include <platform/mp4.h>
#include <platform/log.h>

#include <cstring>

namespace pf { namespace mp4
{

namespace
{

struct Box
{
    uint32_t type = 0;
    const uint8_t* data = nullptr;  // payload
    size_t size = 0;                // payload bytes
};

uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
uint32_t be16(const uint8_t* p) { return uint32_t(p[0]) << 8 | p[1]; }
uint64_t be64(const uint8_t* p) { return uint64_t(be32(p)) << 32 | be32(p + 4); }
uint32_t fourcc(const char* s) { return be32(reinterpret_cast<const uint8_t*>(s)); }

// The next child box of [p, end); false at the end or on a damaged size.
bool nextBox(const uint8_t*& p, const uint8_t* end, Box* box)
{
    if (end - p < 8)
        return false;
    uint64_t size = be32(p);
    size_t header = 8;
    if (size == 1 && end - p >= 16)
    {
        size = be64(p + 8);
        header = 16;
    }
    else if (size == 0)
    {
        size = uint64_t(end - p);
    }
    if (size < header || size > uint64_t(end - p))
        return false;
    box->type = be32(p + 4);
    box->data = p + header;
    box->size = size_t(size - header);
    p += size;
    return true;
}

bool find(const Box& parent, const char* type, Box* out)
{
    const uint8_t* p = parent.data;
    Box box;
    while (nextBox(p, parent.data + parent.size, &box))
    {
        if (box.type == fourcc(type))
        {
            *out = box;
            return true;
        }
    }
    return false;
}

void startCode(std::vector<uint8_t>& out)
{
    const uint8_t code[4] = { 0, 0, 0, 1 };
    out.insert(out.end(), code, code + 4);
}

bool readAvcC(const Box& avcC, Track* track)
{
    const uint8_t* p = avcC.data;
    if (avcC.size < 7 || p[0] != 1)
        return false;
    track->nalLength = (p[4] & 3) + 1;
    size_t pos = 5;
    const int spsCount = p[pos++] & 0x1F;
    for (int pass = 0; pass < 2; ++pass)
    {
        const int count = pass == 0 ? spsCount : (pos < avcC.size ? p[pos++] : 0);
        for (int i = 0; i < count; ++i)
        {
            if (pos + 2 > avcC.size)
                return false;
            const size_t length = be16(p + pos);
            pos += 2;
            if (pos + length > avcC.size)
                return false;
            startCode(track->parameterSets);
            track->parameterSets.insert(track->parameterSets.end(), p + pos, p + pos + length);
            pos += length;
        }
    }
    return !track->parameterSets.empty();
}

bool readTrack(const Box& trak, Track* track)
{
    Box mdia, hdlr, minf, stbl, stsd, stsz, stsc, stco;
    if (!find(trak, "mdia", &mdia) || !find(mdia, "hdlr", &hdlr) || hdlr.size < 12
        || be32(hdlr.data + 8) != fourcc("vide"))
        return false;
    if (!find(mdia, "minf", &minf) || !find(minf, "stbl", &stbl) || !find(stbl, "stsd", &stsd)
        || !find(stbl, "stsz", &stsz) || !find(stbl, "stsc", &stsc))
        return false;
    const bool wide = !find(stbl, "stco", &stco) && find(stbl, "co64", &stco);
    if (stco.data == nullptr && !wide)
        return false;
    if (stsd.size < 16)
        return false;
    // The sample entry: 8 box header bytes, then 78 bytes of visual fields, then child boxes.
    const uint8_t* entry = stsd.data + 8;
    const uint8_t* entryEnd = stsd.data + stsd.size;
    Box avc;
    if (!nextBox(entry, entryEnd, &avc) || avc.type != fourcc("avc1") || avc.size < 78)
        return false;
    track->width = int(be16(avc.data + 24));
    track->height = int(be16(avc.data + 26));
    const uint8_t* child = avc.data + 78;
    Box avcC, box;
    bool haveConfig = false;
    while (nextBox(child, avc.data + avc.size, &box))
    {
        if (box.type == fourcc("avcC"))
        {
            avcC = box;
            haveConfig = true;
        }
    }
    if (!haveConfig || !readAvcC(avcC, track))
        return false;

    if (stsz.size < 12)
        return false;
    const uint32_t fixedSize = be32(stsz.data + 4);
    const uint32_t samples = be32(stsz.data + 8);
    if (fixedSize == 0 && stsz.size < 12 + size_t(samples) * 4)
        return false;
    track->sizes.resize(samples);
    for (uint32_t i = 0; i < samples; ++i)
        track->sizes[i] = fixedSize ? fixedSize : be32(stsz.data + 12 + size_t(i) * 4);

    if (stco.size < 8)
        return false;
    const uint32_t chunks = be32(stco.data + 4);
    const size_t entrySize = wide ? 8 : 4;
    if (stco.size < 8 + size_t(chunks) * entrySize || stsc.size < 8)
        return false;
    const uint32_t runs = be32(stsc.data + 4);
    if (stsc.size < 8 + size_t(runs) * 12)
        return false;
    track->offsets.reserve(samples);
    uint32_t sample = 0;
    for (uint32_t chunk = 0; chunk < chunks && sample < samples; ++chunk)
    {
        uint32_t perChunk = 0;
        for (uint32_t r = 0; r < runs; ++r)
        {
            if (be32(stsc.data + 8 + r * 12) <= chunk + 1)
                perChunk = be32(stsc.data + 8 + r * 12 + 4);
        }
        const uint8_t* entryPtr = stco.data + 8 + size_t(chunk) * entrySize;
        uint64_t position = wide ? be64(entryPtr) : be32(entryPtr);
        for (uint32_t i = 0; i < perChunk && sample < samples; ++i, ++sample)
        {
            track->offsets.push_back(position);
            position += track->sizes[sample];
        }
    }
    return track->offsets.size() == samples;
}

}

bool open(FILE* file, Track* track)
{
    fseek(file, 0, SEEK_END);
    const long long fileSize = ftell(file);
    long long position = 0;
    std::vector<uint8_t> moov;
    while (position + 8 <= fileSize)
    {
        uint8_t header[16];
        fseek(file, long(position), SEEK_SET);
        if (fread(header, 1, 8, file) != 8)
            break;
        uint64_t size = be32(header);
        size_t headerSize = 8;
        if (size == 1)
        {
            if (fread(header + 8, 1, 8, file) != 8)
                break;
            size = be64(header + 8);
            headerSize = 16;
        }
        else if (size == 0)
        {
            size = uint64_t(fileSize - position);
        }
        if (size < headerSize || position + (long long)size > fileSize)
            break;
        if (be32(header + 4) == fourcc("moov"))
        {
            moov.resize(size_t(size) - headerSize);
            fseek(file, long(position + headerSize), SEEK_SET);
            if (fread(moov.data(), 1, moov.size(), file) != moov.size())
                moov.clear();
            break;
        }
        position += (long long)size;
    }
    if (moov.empty())
    {
        pf::logError("mp4: no moov box");
        return false;
    }
    Box root;
    root.type = fourcc("moov");
    root.data = moov.data();
    root.size = moov.size();
    const uint8_t* p = root.data;
    Box box;
    while (nextBox(p, root.data + root.size, &box))
    {
        if (box.type == fourcc("trak") && readTrack(box, track))
            return true;
    }
    pf::logError("mp4: no H.264 video track");
    return false;
}

}}
