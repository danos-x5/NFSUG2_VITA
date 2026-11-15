// Movie backend: replaces the VP6 codec's frame decode (0x700aff) with frames from <name>.mp4 beside <name>.vp6,
// laid out as the codec's bordered, bottom-up YV12. The game's demuxer, clock, audio and skip stay as they are.

#include "movies.h"

#include <lib/memmap.h>
#include <platform/config.h>
#include <platform/log.h>
#include <platform/perf.h>
#include <platform/time.h>
#include <platform/video.h>
#include <recomp.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <strings.h>
#include <string>
#include <vector>

namespace ug2 { namespace movies {

namespace
{

using x86::reg32;
namespace video = pf::video;
using win32::MemMap;

const reg32 CreateFileSlot = 0x007831D0;       // kernel32 CreateFileA import
const reg32 CodecDecode = 0x00700AFF;          // VP6 codec vtable 0x7C35B8 entry 2: thiscall (time), ret 4
const reg32 CodecDestroy = 0x00700D67;         // vtable entry 0: thiscall (flags), ret 4
const reg32 PopPacket = 0x007007F3;            // thiscall (packet**), ret 4; counts the packet in codec+0x20
const reg32 ReleasePacket = 0x0070080E;        // thiscall (packet), ret 4
const reg32 PacketTag = 0x007007C0;            // eax packet in, eax the 4-byte tag out
const reg32 GetFrame = 0x007008EF;             // thiscall, returns a FRAME from the codec's free list
const reg32 ImageHeader = 0x006F4820;          // thiscall on an image, returns its header
const reg32 SetImageData = 0x006F49D0;         // thiscall (pointer, size), ret 8
const reg32 SetImageWidth = 0x00700775;        // thiscall (width), ret 4
const reg32 SetImageHeight = 0x0070078A;       // thiscall (height), ret 4
const reg32 TagKey = 0x4D56304B;               // 'MV0K'
const reg32 TagPredicted = 0x4D563046;         // 'MV0F'
const reg32 CodecFirstFlag = 0x1C;             // nonzero until the header packet is read
const reg32 CodecPopped = 0x20;                // packets popped
const reg32 CodecWidth = 0x8;
const reg32 CodecHeight = 0xC;
const reg32 FrameImage = 0x4;
const int LateFrames = 0x10;                   // frames behind the clock before the codec skips to a key frame
const int Border = 48;                         // VP6 frame border, luma pixels
const int RingSize = 4;

enum Mode { ModeGame, ModeNative };

struct Session
{
    video::Stream* stream = nullptr;
    video::Info info = {};
    std::string name;
    reg32 codec = 0;
    int popped = 0;                            // frame packets popped
    int delivered = 0;                         // frames taken from the stream
    int used = 0;
    int skipped = 0;
    int repeated = 0;
    uint64_t waitUs = 0;                       // time blocked in the decoder
    bool ended = false;
    unsigned startMs = 0;
    std::vector<uint8_t> last;
};

Mode s_mode = ModeGame;
std::string s_dir;
win32::MethodPtr s_createFile = nullptr;
win32::MethodPtr s_decode = nullptr;
win32::MethodPtr s_destroy = nullptr;
Session s_session;
std::atomic_flag s_nameLock = ATOMIC_FLAG_INIT;
char s_lastName[96] = {};
MemMap* s_ring[RingSize] = {};
reg32 s_ringBytes = 0;
int s_ringNext = 0;

void lockName()
{
    while (s_nameLock.test_and_set(std::memory_order_acquire))
    {
    }
}

void unlockName()
{
    s_nameLock.clear(std::memory_order_release);
}

// Calls a guest function from native code: ecx, eax and stack arguments (callee or caller cleans; esp is restored).
reg32 call(win32::WinApplication* app, x86::CPU& cpu, reg32 address, reg32 self, std::initializer_list<reg32> args,
           reg32 eax = 0)
{
    const reg32 saved = cpu.esp;
    reg32 sp = saved;
    for (const reg32* it = args.end(); it != args.begin();)
    {
        --it;
        sp -= 4;
        rc::st32(sp, *it);
    }
    sp -= 4;
    rc::st32(sp, 0);
    cpu.esp = sp;
    cpu.ecx = self;
    cpu.eax = eax;
    app->dynamic_call(address, cpu);
    cpu.esp = saved;
    return cpu.eax;
}

bool isFrameTag(reg32 tag)
{
    return tag == TagKey || tag == TagPredicted;
}

// Name of an opened ".vp6" path: letters and digits between the last separator and the extension.
bool movieName(reg32 path, char* name, size_t size)
{
    char text[260];
    size_t n = 0;
    while (n < sizeof(text) - 1 && rc::ld8(path + reg32(n)))
    {
        text[n] = char(rc::ld8(path + reg32(n)));
        ++n;
    }
    text[n] = 0;
    if (n < 5 || strcasecmp(text + n - 4, ".vp6") != 0)
        return false;
    text[n - 4] = 0;
    const char* base = text;
    for (const char* c = text; *c; ++c)
    {
        if (*c == '/' || *c == '\\')
            base = c + 1;
    }
    if (!*base || strlen(base) >= size)
        return false;
    strcpy(name, base);
    return true;
}

void createFileHook(win32::WinApplication* app, x86::CPU& cpu)
{
    char name[sizeof(s_lastName)];
    if (movieName(rc::ld32(cpu.esp + 4), name, sizeof(name)))
    {
        lockName();
        strcpy(s_lastName, name);
        unlockName();
    }
    s_createFile(app, cpu);
}

std::string moviePath(const char* name)
{
    return s_dir + "/" + name + ".mp4";
}

void endSession()
{
    Session& s = s_session;
    if (!s.stream)
        return;
    pf::logInfo("movies: %s ended: %d frames used, %d skipped, %d repeated, %d of %d taken from the decoder, %u ms, waited %u ms for it",
                s.name.c_str(), s.used, s.skipped, s.repeated, s.delivered, s.info.frames,
                unsigned(pf::ticksMs() - s.startMs), unsigned(s.waitUs / 1000));
    video::close(s.stream);
    s = Session();
}

// After the first original decode: opens the movie's MP4 when it matches the codec's frame size.
void startSession(reg32 codec)
{
    char name[sizeof(s_lastName)];
    lockName();
    strcpy(name, s_lastName);
    unlockName();
    if (!name[0])
        return;
    video::Info info;
    video::Stream* stream = video::open(moviePath(name).c_str(), &info);
    if (!stream)
    {
        pf::logInfo("movies: %s: no native video, the game decodes it", name);
        return;
    }
    const int width = int(rc::ld32(codec + CodecWidth));
    const int height = int(rc::ld32(codec + CodecHeight));
    if (info.width != width || info.height != height)
    {
        pf::logError("movies: %s: the file is %dx%d, the movie %dx%d; the game decodes it", name, info.width,
                     info.height, width, height);
        video::close(stream);
        return;
    }
    Session& s = s_session;
    s.stream = stream;
    s.info = info;
    s.name = name;
    s.codec = codec;
    s.popped = 1;
    s.used = 1;
    s.last.assign(size_t(width) * size_t(height) * 3 / 2, 0);
    s.startMs = unsigned(pf::ticksMs());
    pf::logInfo("movies: %s plays natively, %dx%d, %d frames", name, width, height, info.frames);
}

// Frame k of the stream (in the session's I420 copy), or the last one when the stream is short.
const uint8_t* frameAt(Session& s, int k)
{
    const uint8_t* data = nullptr;
    while (!s.ended && s.delivered <= k)
    {
        const uint64_t before = pf::ticksUs();
        const bool got = video::next(s.stream, &data);
        s.waitUs += pf::ticksUs() - before;
        if (!got)
        {
            s.ended = true;
            break;
        }
        if (s.delivered == k)
            memcpy(s.last.data(), data, s.last.size());
        else if (s.delivered != 0)
            ++s.skipped;
        ++s.delivered;
    }
    if (s.delivered <= k)
        ++s.repeated;
    return s.last.data();
}

void copyPlane(uint8_t* dst, int dstPitch, const uint8_t* src, int srcPitch, int width, int height, int border)
{
    for (int row = 0; row < height; ++row)
    {
        uint8_t* line = dst + (border + row) * dstPitch + border;
        memcpy(line, src + (height - 1 - row) * srcPitch, size_t(width));
        memset(line - border, line[0], size_t(border));
        memset(line + width, line[width - 1], size_t(border));
    }
    for (int row = 0; row < border; ++row)
    {
        memcpy(dst + row * dstPitch, dst + border * dstPitch, size_t(dstPitch));
        memcpy(dst + (border + height + row) * dstPitch, dst + (border + height - 1) * dstPitch, size_t(dstPitch));
    }
}

reg32 ringBuffer(reg32 bytes)
{
    if (s_ringBytes != bytes)
    {
        for (MemMap*& m : s_ring)
        {
            delete m;
            m = nullptr;
        }
        s_ringBytes = bytes;
    }
    MemMap*& slot = s_ring[s_ringNext];
    s_ringNext = (s_ringNext + 1) % RingSize;
    if (!slot)
        slot = new MemMap(bytes, MemMap::High, pf::perf::AllocOther);
    return slot->getBlockStart();
}

// The codec's output: a FRAME whose image points at a bordered, bottom-up YV12 buffer of the movie frame.
reg32 buildFrame(win32::WinApplication* app, x86::CPU& cpu, Session& s, int k)
{
    const reg32 frame = call(app, cpu, GetFrame, s.codec, {});
    if (!frame)
        return 0;
    const int w = s.info.width, h = s.info.height;
    const int yPitch = w + 2 * Border, cPitch = yPitch / 2;
    const int yRows = h + 2 * Border, cRows = yRows / 2;
    const reg32 bytes = reg32(yPitch * yRows + 2 * cPitch * cRows);
    const reg32 buffer = ringBuffer(bytes);
    uint8_t* dst = win32::g_guestBase + buffer;
    const uint8_t* src = frameAt(s, k);
    copyPlane(dst, yPitch, src, w, w, h, Border);
    copyPlane(dst + yPitch * yRows, cPitch, src + w * h, w / 2, w / 2, h / 2, Border / 2);
    copyPlane(dst + yPitch * yRows + cPitch * cRows, cPitch, src + w * h * 5 / 4, w / 2, w / 2, h / 2, Border / 2);
    ++s.used;
    const reg32 image = rc::ld32(frame + FrameImage);
    const reg32 header = call(app, cpu, ImageHeader, image, {});
    call(app, cpu, SetImageData, header, { buffer, bytes });
    call(app, cpu, SetImageWidth, image, { reg32(w) });
    call(app, cpu, SetImageHeight, image, { reg32(h) });
    return frame;
}

reg32 popPacket(win32::WinApplication* app, x86::CPU& cpu, reg32 codec, reg32 slot, Session& s)
{
    const reg32 packet = call(app, cpu, PopPacket, codec, { slot });
    if (packet && isFrameTag(call(app, cpu, PacketTag, 0, {}, packet)))
        ++s.popped;
    return packet;
}

// Native version of the codec's decode for a movie in a session: the original's steps without the VP6 work.
void decodeNative(win32::WinApplication* app, x86::CPU& cpu)
{
    Session& s = s_session;
    const reg32 codec = cpu.ecx;
    const int time = int(rc::ld32(cpu.esp + 4));
    const reg32 entryEsp = cpu.esp;
    cpu.esp -= 16;
    const reg32 slot = cpu.esp;
    reg32 packet = popPacket(app, cpu, codec, slot, s);
    const int late = time - int(rc::ld32(codec + CodecPopped));
    if (late > LateFrames)
    {
        while (packet)
        {
            call(app, cpu, ReleasePacket, codec, { packet });
            packet = popPacket(app, cpu, codec, slot, s);
            if (packet && call(app, cpu, PacketTag, 0, {}, packet) == TagKey)
                break;
        }
    }
    else if (late > 0 && packet && isFrameTag(call(app, cpu, PacketTag, 0, {}, packet)))
    {
        call(app, cpu, ReleasePacket, codec, { packet });
        packet = popPacket(app, cpu, codec, slot, s);
    }
    reg32 frame = 0;
    if (packet)
    {
        frame = buildFrame(app, cpu, s, s.popped > 0 ? s.popped - 1 : 0);
        call(app, cpu, ReleasePacket, codec, { packet });
    }
    cpu.eax = frame;
    cpu.esp = entryEsp + 8;
}

void decodeHook(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 codec = cpu.ecx;
    const bool first = rc::ld32(codec + CodecFirstFlag) != 0;
    if (!first && s_session.stream && s_session.codec == codec)
    {
        decodeNative(app, cpu);
        return;
    }
    if (first)
        endSession();
    s_decode(app, cpu);
    if (first && s_mode == ModeNative)
        startSession(codec);
}

void destroyHook(win32::WinApplication* app, x86::CPU& cpu)
{
    if (s_session.stream && s_session.codec == cpu.ecx)
        endSession();
    s_destroy(app, cpu);
}

// Decodes a whole file through the platform decoder and logs what came out.
void selfTest(const char* name)
{
    video::Info info;
    const unsigned start = unsigned(pf::ticksMs());
    video::Stream* stream = video::open(moviePath(name).c_str(), &info);
    if (!stream)
    {
        pf::logError("movies_selftest: %s did not open", moviePath(name).c_str());
        return;
    }
    int count = 0;
    uint32_t sum = 0, firstSum = 0;
    const uint8_t* data = nullptr;
    const size_t bytes = size_t(info.width) * size_t(info.height) * 3 / 2;
    while (video::next(stream, &data))
    {
        uint32_t h = 2166136261u;
        for (size_t i = 0; i < bytes; i += 61)
            h = (h ^ data[i]) * 16777619u;
        if (count == 0)
            firstSum = h;
        sum ^= h;
        ++count;
    }
    video::close(stream);
    pf::logInfo("movies_selftest: %s %dx%d: %d of %d frames decoded in %u ms, first frame hash %08X, all %08X", name,
                info.width, info.height, count, info.frames, unsigned(pf::ticksMs()) - start,
                unsigned(firstSum), unsigned(sum));
}

}

void configure(win32::WinApplication& app)
{
#ifdef __vita__
    const char* const defaultMode = "native";
    const char* const defaultDir = "ux0:data/NeedForSpeedUnderground2/MOVIES";
#else
    const char* const defaultMode = "game";
    const char* const defaultDir = "";
#endif
    static const char* const modes[] = { "game", "native" };
    s_mode = Mode(pf::Config::getChoice("movies", modes, 2, strcmp(defaultMode, "native") == 0 ? 1 : 0));
    s_dir = pf::Config::getString("movies_dir", defaultDir);
    while (s_dir.size() > 1 && (s_dir.back() == '/' || s_dir.back() == '\\'))
        s_dir.pop_back();
    const std::string test = pf::Config::getString("movies_selftest", "");
    if (!test.empty())
        selfTest(test.c_str());
    if (s_mode != ModeNative)
    {
        pf::logInfo("movies: movies=game, the game decodes its VP6 movies");
        return;
    }
    if (s_dir.empty())
    {
        pf::logError("movies: movies=native needs movies_dir; the game decodes its movies");
        s_mode = ModeGame;
        return;
    }
    s_createFile = app.replaceMethod(rc::ld32(CreateFileSlot), createFileHook);
    s_decode = app.replaceMethod(CodecDecode, decodeHook);
    s_destroy = app.replaceMethod(CodecDestroy, destroyHook);
    pf::logInfo("movies: native playback from %s (the game's VP6 decoder when a file is missing)", s_dir.c_str());
}

}}
