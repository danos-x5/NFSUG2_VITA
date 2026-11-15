// Vita video decoding: MP4 samples to the hardware H.264 decoder (SceVideodec) on a worker thread, frames queued.

#include <platform/video.h>
#include <platform/cores.h>
#include <platform/log.h>
#include <platform/mp4.h>
#include <platform/sync.h>

#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <psp2/videodec.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <vector>

namespace pf { namespace video
{

namespace
{

const int Slots = 6;
const uint32_t Megabyte = 1024 * 1024;
const int RefFrames = 4;

uint32_t roundUp(uint32_t value, uint32_t unit)
{
    return (value + unit - 1) / unit * unit;
}

// Physically contiguous memory, as the decoder needs.
struct Block
{
    SceUID id = -1;
    void* base = nullptr;
    uint32_t size = 0;

    bool alloc(const char* name, uint32_t bytes)
    {
        size = roundUp(bytes, Megabyte);
        id = sceKernelAllocMemBlock(name, SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, size, nullptr);
        if (id < 0 || sceKernelGetMemBlockBase(id, &base) < 0)
        {
            pf::logError("video: %s memory block of %u bytes: 0x%08X", name, unsigned(size), unsigned(id));
            release();
            return false;
        }
        return true;
    }

    void release()
    {
        if (id >= 0)
            sceKernelFreeMemBlock(id);
        id = -1;
        base = nullptr;
    }
};

bool s_libraryLoaded = false;

}

class Stream
{
public:
    Stream() : filled("video filled", 0), freed("video freed", Slots) {}

    FILE* file = nullptr;
    mp4::Track track;
    Info info;
    Block decoderMemory, esMemory, pictureMemory;
    SceAvcdecCtrl decoder = {};
    bool decoderCreated = false;
    bool libraryInit = false;
    Thread* thread = nullptr;
    Semaphore filled;
    Semaphore freed;
    std::vector<uint8_t> slots[Slots];
    std::atomic<int> produced{0};
    std::atomic<bool> done{false};
    std::atomic<bool> stop{false};
    int consumed = 0;
    bool holding = false;
    int decodedFrames = 0;

    bool start(const char* path);
    void run();
    bool decode(const uint8_t* annexB, uint32_t size);
    void shutdown();
};

namespace
{

int threadMain(void* argument)
{
    static_cast<Stream*>(argument)->run();
    return 0;
}

}

bool Stream::start(const char* path)
{
    file = fopen(path, "rb");
    if (!file)
        return false;
    if (!mp4::open(file, &track))
        return false;
    if ((track.width & 15) || (track.height & 15) || track.width > 1280 || track.height > 720)
    {
        pf::logError("video: %s: %dx%d is not a multiple of 16 within 1280x720", path, track.width, track.height);
        return false;
    }
    info.width = track.width;
    info.height = track.height;
    info.frames = int(track.sizes.size());

    if (!s_libraryLoaded)
    {
        const int result = sceSysmoduleLoadModule(SCE_SYSMODULE_AVCDEC);
        if (result < 0)
            pf::logError("video: loading the AVC decoder module: 0x%08X (continuing)", unsigned(result));
        s_libraryLoaded = true;
    }
    SceVideodecQueryInitInfoHwAvcdec init = {};
    init.size = sizeof(init);
    init.horizontal = uint32_t(track.width);
    init.vertical = uint32_t(track.height);
    init.numOfRefFrames = RefFrames;
    init.numOfStreams = 1;
    int result = sceVideodecInitLibrary(SCE_VIDEODEC_TYPE_HW_AVCDEC, &init);
    if (result < 0)
    {
        pf::logError("video: sceVideodecInitLibrary: 0x%08X", unsigned(result));
        return false;
    }
    libraryInit = true;

    SceAvcdecQueryDecoderInfo query = {};
    query.horizontal = uint32_t(track.width);
    query.vertical = uint32_t(track.height);
    query.numOfRefFrames = RefFrames;
    SceAvcdecDecoderInfo decoderInfo = {};
    result = sceAvcdecQueryDecoderMemSize(SCE_VIDEODEC_TYPE_HW_AVCDEC, &query, &decoderInfo);
    if (result < 0)
    {
        pf::logError("video: sceAvcdecQueryDecoderMemSize: 0x%08X", unsigned(result));
        return false;
    }
    if (!decoderMemory.alloc("video decoder", decoderInfo.frameMemSize) || !esMemory.alloc("video es", Megabyte)
        || !pictureMemory.alloc("video picture", uint32_t(track.width * track.height * 3 / 2)))
        return false;
    decoder.frameBuf.pBuf = decoderMemory.base;
    decoder.frameBuf.size = decoderMemory.size;
    result = sceAvcdecCreateDecoder(SCE_VIDEODEC_TYPE_HW_AVCDEC, &decoder, &query);
    if (result < 0)
    {
        pf::logError("video: sceAvcdecCreateDecoder: 0x%08X", unsigned(result));
        return false;
    }
    decoderCreated = true;
    for (std::vector<uint8_t>& slot : slots)
        slot.resize(size_t(track.width * track.height * 3 / 2));
    pf::logInfo("video: %s %dx%d, %d frames, decoder memory %u KB", path, info.width, info.height, info.frames,
                unsigned(decoderInfo.frameMemSize / 1024));
    thread = new Thread("video decode", threadMain, this, Thread::PriorityNormal, 64 * 1024,
                        cores::mask(cores::Io));
    thread->start();
    return true;
}

// One access unit in; a finished picture is queued when the decoder returns one.
bool Stream::decode(const uint8_t* annexB, uint32_t size)
{
    SceAvcdecAu au = {};
    au.pts.upper = au.pts.lower = 0xFFFFFFFF;
    au.dts.upper = au.dts.lower = 0xFFFFFFFF;
    au.es.pBuf = const_cast<uint8_t*>(annexB);
    au.es.size = size;
    SceAvcdecPicture picture = {};
    picture.size = sizeof(picture);
    picture.frame.pixelType = SCE_AVCDEC_PIXELFORMAT_YUV420_RASTER;
    picture.frame.framePitch = uint32_t(track.width);
    picture.frame.frameWidth = uint32_t(track.width);
    picture.frame.frameHeight = uint32_t(track.height);
    picture.frame.pPicture[0] = pictureMemory.base;
    SceAvcdecPicture* pictures[1] = { &picture };
    SceAvcdecArrayPicture array = {};
    array.numOfElm = 1;
    array.pPicture = pictures;
    const int result = sceAvcdecDecode(&decoder, &au, &array);
    if (result < 0)
    {
        pf::logError("video: sceAvcdecDecode: 0x%08X", unsigned(result));
        return false;
    }
    if (array.numOfOutput == 0)
        return true;
    while (!freed.waitFor(20))
    {
        if (stop.load())
            return false;
    }
    const int index = produced.load() % Slots;
    memcpy(slots[index].data(), pictureMemory.base, slots[index].size());
    ++decodedFrames;
    produced.fetch_add(1);
    filled.post();
    return true;
}

void Stream::run()
{
    uint8_t* es = static_cast<uint8_t*>(esMemory.base);
    std::vector<uint8_t> sample;
    for (size_t i = 0; i < track.sizes.size() && !stop.load(); ++i)
    {
        sample.resize(track.sizes[i]);
        if (fseek(file, long(track.offsets[i]), SEEK_SET) != 0 || fread(sample.data(), 1, sample.size(), file) != sample.size())
            break;
        size_t out = 0;
        if (i == 0)
        {
            memcpy(es, track.parameterSets.data(), track.parameterSets.size());
            out = track.parameterSets.size();
        }
        size_t in = 0;
        bool bad = false;
        while (in + size_t(track.nalLength) <= sample.size())
        {
            uint32_t length = 0;
            for (int b = 0; b < track.nalLength; ++b)
                length = length << 8 | sample[in + size_t(b)];
            in += size_t(track.nalLength);
            if (in + length > sample.size() || out + 4 + length > esMemory.size)
            {
                bad = true;
                break;
            }
            es[out++] = 0;
            es[out++] = 0;
            es[out++] = 0;
            es[out++] = 1;
            memcpy(es + out, sample.data() + in, length);
            out += length;
            in += length;
        }
        if (bad || !decode(es, uint32_t(out)))
            break;
    }
    done.store(true);
    filled.post();
}

void Stream::shutdown()
{
    stop.store(true);
    if (thread)
    {
        thread->join();
        delete thread;
        thread = nullptr;
    }
    if (decoderCreated)
        sceAvcdecDeleteDecoder(&decoder);
    if (libraryInit)
        sceVideodecTermLibrary(SCE_VIDEODEC_TYPE_HW_AVCDEC);
    pictureMemory.release();
    esMemory.release();
    decoderMemory.release();
    if (file)
        fclose(file);
    file = nullptr;
}

Stream* open(const char* path, Info* info)
{
    Stream* stream = new Stream();
    if (!stream->start(path))
    {
        stream->shutdown();
        delete stream;
        return nullptr;
    }
    *info = stream->info;
    return stream;
}

bool next(Stream* stream, const uint8_t** data)
{
    if (stream->holding)
    {
        stream->holding = false;
        stream->freed.post();
    }
    for (;;)
    {
        if (stream->consumed < stream->produced.load())
        {
            *data = stream->slots[stream->consumed % Slots].data();
            ++stream->consumed;
            stream->holding = true;
            return true;
        }
        if (stream->done.load())
            return false;
        stream->filled.waitFor(50);
    }
}

void close(Stream* stream)
{
    if (!stream)
        return;
    pf::logInfo("video: closed after %d decoded frames, %d taken", stream->decodedFrames, stream->consumed);
    stream->shutdown();
    delete stream;
}

}}
