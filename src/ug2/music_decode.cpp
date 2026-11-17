// The EALayer3 decoder object's granule method (0x72C270) on the hardware MP3 decoder: granule pairs become MPEG-1
// frames. Stream reading, PCM inserts, start delay and end trim stay the game's.

#include "music_decode.h"
#include "ealayer3.h"

#include <platform/config.h>
#include <platform/log.h>
#include <platform/mp3dec.h>
#include <platform/perf.h>
#include <recomp.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ug2 { namespace music
{

namespace
{

using x86::reg32;
using ealayer3::Granule;
using ealayer3::GranuleSamples;

const reg32 DecodeGranule   = 0x72c270;     // vtable +0x18, thiscall: granule at [+0x24] to PCM at [+0x1B8]
const reg32 StartStream     = 0x72beb3;     // vtable +8, thiscall (first granule, 0)
const reg32 Destroy         = 0x72c2a6;     // vtable +0, thiscall (flags)
const reg32 ParseHeader     = 0x72be11;     // vtable +4, thiscall (header byte)
const reg32 ReadPointer     = 0x24;
const reg32 FrameStart      = 0x28;
const reg32 GranuleBytes    = 0x14;
const reg32 Output          = 0x1b8;        // float, 576 per channel, channel after channel
const int   MaxStreams      = 16;
const uint32_t StealAfter   = 8000;         // granules decoded since a stream's last one
const uint32_t CheckGranules = 48;          // left channel kept for the delay search (music_hw_check)
const int   MaxLag          = 1152;
const uint32_t CheckLogEvery = 500;

struct Check
{
    uint32_t granules = 0, sizeMismatches = 0, over2 = 0, apart = 0, clipped = 0, reported = 0;
    uint32_t inserts = 0, insertsFound = 0;
    FILE* dump = nullptr;
    uint32_t dumped = 0;
    uint32_t insertOffset = 0, insertCount = 0;  // PCM insert after the last granule, checked at the next call
    int16_t insertPcm[GranuleSamples * 2];
    float maxDiff = 0;
    double sumSquares = 0;
    uint32_t kept = 0;
    bool searched = false;
    float game[CheckGranules * GranuleSamples];
    float hardware[CheckGranules * GranuleSamples];
};

struct Stream
{
    reg32 object = 0;
    pf::mp3::Decoder* decoder = nullptr;
    bool hardware = false;
    int header = -1;                        // last header byte given to the game's parser
    reg32 pendingAt = 0;                    // granule 1 whose samples are in pending
    bool paired = false;                    // the last granule's frame had both granules
    uint32_t lone = 0;                      // frames with an empty partner granule
    uint32_t lastUse = 0;
    int16_t pending[GranuleSamples * 2];
    Check* check = nullptr;
};

Stream s_streams[MaxStreams];
win32::MethodPtr s_decode = nullptr;
win32::MethodPtr s_start = nullptr;
win32::MethodPtr s_destroy = nullptr;
bool s_check = false;                       // music_hw_check: both decoders, compared
const char* s_dumpPath = nullptr;           // music_hw_check_dump: first granules of each stream, both decoders
int s_dumps = 0;
const uint32_t DumpGranules = 300;
uint32_t s_clock = 0;                       // granules decoded
bool s_failLogged = false;

struct Timing
{
    uint32_t frames = 0, us = 0, maxUs = 0, gameGranules = 0, since = 0;
};
Timing s_timing;

Stream* find(reg32 object)
{
    for (Stream& s : s_streams)
    {
        if (s.object == object)
            return &s;
    }
    return nullptr;
}

pf::mp3::Decoder* acquire()
{
    if (pf::mp3::Decoder* decoder = pf::mp3::create())
        return decoder;
    Stream* idle = nullptr;
    for (Stream& s : s_streams)
    {
        if (s.decoder && s_clock - s.lastUse > StealAfter && (!idle || s.lastUse < idle->lastUse))
            idle = &s;
    }
    if (!idle)
        return nullptr;
    pf::mp3::Decoder* decoder = idle->decoder;
    idle->decoder = nullptr;
    return decoder;
}

void logCheck(const Stream& s)
{
    const Check& c = *s.check;
    if (!c.granules)
        return;
    const double samples = double(c.granules) * GranuleSamples * 2;
    pf::logInfo("music check 0x%08x: %u granules, max difference %.2f, rms %.4f, %u samples over 2 LSB in %u granules, %u past "
                "16 bits in the game's (left out), %u granule size mismatches, %u of %u PCM inserts in place, "
                "%u lone granules",
                unsigned(s.object), unsigned(c.granules), double(c.maxDiff), std::sqrt(c.sumSquares / samples),
                unsigned(c.over2), unsigned(c.apart), unsigned(c.clipped), unsigned(c.sizeMismatches), unsigned(c.insertsFound),
                unsigned(c.inserts), unsigned(s.lone));
}

void release(Stream& s)
{
    if (s.check)
    {
        if (s.check->dump)
            fclose(s.check->dump);
        logCheck(s);
        delete s.check;
    }
    if (s.decoder)
        pf::mp3::destroy(s.decoder);
    s = Stream();
}

void failed(Stream& s, const char* what)
{
    if (!s_failLogged)
    {
        s_failLogged = true;
        pf::logError("music: hardware decoder %s (0x%08X); the game's decoder takes over", what,
                     unsigned(pf::mp3::lastError()));
    }
    s.hardware = false;
}

void store(const int16_t* pcm, float* out)
{
    for (uint32_t i = 0; i < GranuleSamples; ++i)
    {
        out[i] = float(pcm[2 * i]);
        out[GranuleSamples + i] = float(pcm[2 * i + 1]);
    }
}

bool sameShape(const Granule& a, const Granule& b)
{
    return a.version == b.version && a.rateIndex == b.rateIndex && a.channels == b.channels;
}

// Decodes frame (first, second) and keeps both halves; false on a decoder error.
bool decodeFrame(Stream& s, const Granule* first, const Granule* second, int16_t* pcm)
{
    if (!s.decoder)
    {
        s.decoder = acquire();
        if (!s.decoder || !pf::mp3::reset(s.decoder))
            return false;
    }
    uint8_t frame[ealayer3::MaxFrameBytes];
    const uint32_t size = ealayer3::frame(first, second, frame);
    const bool timed = pf::perf::enabled();
    const uint32_t start = timed ? pf::perf::now() : 0;
    const int samples = size ? pf::mp3::decode(s.decoder, frame, size, pcm) : -1;
    if (timed)
    {
        const uint32_t us = pf::perf::now() - start;
        Timing& t = s_timing;
        ++t.frames;
        t.us += us;
        t.maxUs = std::max(t.maxUs, us);
    }
    return samples == int(ealayer3::FrameSamples);
}

// The granule's samples from the hardware decoder into out (576 per channel).
bool hardwareGranule(Stream& s, reg32 at, const Granule& g, float* out)
{
    int16_t pcm[ealayer3::FrameSamples * 2];
    if (g.index == 0)
    {
        const uint8_t* nextData = ealayer3::next(g);
        Granule second;
        const bool pair = nextData && ealayer3::parse(nextData, second) && second.index == 1 && sameShape(g, second);
        if (!decodeFrame(s, &g, pair ? &second : nullptr, pcm))
            return false;
        store(pcm, out);
        memcpy(s.pending, pcm + GranuleSamples * 2, sizeof(s.pending));
        s.pendingAt = pair ? at + reg32(nextData - g.data) : 0;
        s.paired = pair;
        s.lone += !pair;
        return true;
    }
    if (s.pendingAt == at)
    {
        store(s.pending, out);
        s.pendingAt = 0;
        s.paired = true;
        return true;
    }
    if (!decodeFrame(s, nullptr, &g, pcm))
        return false;
    store(pcm + GranuleSamples * 2, out);
    s.pendingAt = 0;
    s.paired = false;
    ++s.lone;
    return true;
}

void parseHeader(win32::WinApplication* app, x86::CPU& cpu, reg32 object, uint8_t header)
{
    const x86::CPU saved = cpu;
    cpu.esp -= 4;
    rc::st32(cpu.esp, header);
    cpu.esp -= 4;
    cpu.ecx = object;
    app->dynamic_call(ParseHeader, cpu);
    if (cpu.terminate)
        return;
    const x86::FPU fpu = cpu.fpu;
    cpu = saved;
    cpu.fpu = fpu;
}

void searchDelay(Stream& s)
{
    Check& c = *s.check;
    c.searched = true;
    const int n = int(CheckGranules * GranuleSamples);
    double best = 1e30, atZero = 0;
    int bestLag = 0;
    for (int lag = -MaxLag; lag <= MaxLag; ++lag)
    {
        double sum = 0;
        for (int i = MaxLag; i < n - MaxLag; i += 2)
            sum += std::fabs(double(c.game[i]) - double(c.hardware[i + lag]));
        if (sum < best)
        {
            best = sum;
            bestLag = lag;
        }
        if (lag == 0)
            atZero = sum;
    }
    const double count = double(n - 2 * MaxLag) / 2;
    pf::logInfo("music check 0x%08x: delay search over %u granules: hardware output %d samples from the game's "
                "(mean difference %.3f there, %.3f at 0)", unsigned(s.object), unsigned(CheckGranules), -bestLag,
                best / count, atZero / count);
}

// Keeps the PCM insert at p (0xEE, offset, count, big-endian pairs) for checkInsert.
void keepInsert(Check& c, const uint8_t* p)
{
    ++c.inserts;
    const uint32_t offset = (uint32_t(p[1]) << 8) | p[2], count = (uint32_t(p[3]) << 8) | p[4];
    if (offset > GranuleSamples || count > offset)
        return;
    c.insertOffset = offset;
    c.insertCount = count;
    for (uint32_t i = 0; i < 2 * count; ++i)
        c.insertPcm[i] = int16_t((p[5 + 2 * i] << 8) | p[6 + 2 * i]);
}

// The game wrote the kept insert into the last granule's samples at 576 - offset.
void checkInsert(Check& c, const float* out)
{
    bool same = true;
    for (uint32_t i = 0; i < c.insertCount; ++i)
    {
        for (uint32_t ch = 0; ch < 2; ++ch)
            same &= out[ch * GranuleSamples + GranuleSamples - c.insertOffset + i] == float(c.insertPcm[2 * i + ch]);
    }
    c.insertsFound += same;
    c.insertCount = 0;
}

void compare(Stream& s, const float* game, const float* hardware, uint32_t gameBytes, const Granule& g, bool paired)
{
    Check& c = *s.check;
    const uint32_t bytes = g.bytes;
    const uint32_t over2 = c.over2;
    ++c.granules;
    c.sizeMismatches += gameBytes != bytes;
    float peak = 0;
    for (uint32_t i = 0; i < GranuleSamples * 2; ++i)
    {
        const float d = std::fabs(game[i] - hardware[i]);
        if (std::fabs(game[i]) > 32767.0f)
        {
            ++c.clipped;
            continue;
        }
        c.maxDiff = std::max(c.maxDiff, d);
        c.sumSquares += double(d) * d;
        c.over2 += d > 2.0f;
        peak = std::max(peak, std::fabs(game[i]));
    }
    c.apart += c.over2 > over2;
    if (!c.searched && (c.kept || peak > 1000.0f))
    {
        memcpy(c.game + c.kept * GranuleSamples, game, GranuleSamples * sizeof(float));
        memcpy(c.hardware + c.kept * GranuleSamples, hardware, GranuleSamples * sizeof(float));
        if (++c.kept == CheckGranules)
            searchDelay(s);
    }
    if (!c.dump && c.over2 > over2 && s_dumpPath && *s_dumpPath && s_dumps < 4)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s%02d.f32", s_dumpPath, s_dumps++);
        c.dump = fopen(path, "wb");
        pf::logInfo("music check 0x%08x: first difference over 2 LSB at granule %u, dumping to %s", unsigned(s.object),
                    unsigned(c.granules), path);
    }
    if (c.dump && c.dumped < DumpGranules)
    {
        ++c.dumped;
        const uint32_t head[4] = {c.granules, g.index, paired, bytes};
        fwrite(head, sizeof(head), 1, c.dump);
        fwrite(game, sizeof(float), GranuleSamples * 2, c.dump);
        fwrite(hardware, sizeof(float), GranuleSamples * 2, c.dump);
        fwrite(g.data, 1, 1536, c.dump);
        fflush(c.dump);
    }
    if (c.granules - c.reported >= CheckLogEvery)
    {
        c.reported = c.granules;
        logCheck(s);
    }
}

void logTiming()
{
    Timing& t = s_timing;
    const uint32_t now = pf::perf::now();
    if (int32_t(now - t.since) < 5000000)
        return;
    if (t.frames || t.gameGranules)
        pf::logInfo("music hw 5s: %u frames, %u us average, %u us max; %u granules on the game's decoder",
                    unsigned(t.frames), unsigned(t.frames ? t.us / t.frames : 0), unsigned(t.maxUs),
                    unsigned(t.gameGranules));
    t = Timing();
    t.since = now;
}

void decodeHook(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 object = cpu.ecx;
    Stream* s = find(object);
    if (pf::perf::enabled())
        logTiming();
    if (!s || !s->hardware)
    {
        ++s_timing.gameGranules;
        s_decode(app, cpu);
        return;
    }
    ++s_clock;
    s->lastUse = s_clock;
    const reg32 at = rc::ld32(object + ReadPointer);
    Granule g;
    if (!ealayer3::parse(rc::host(at), g) || g.version != 3 || g.channels != 2)
    {
        failed(*s, "got an unexpected granule");
        s_decode(app, cpu);
        return;
    }
    float* out = reinterpret_cast<float*>(rc::host(rc::ld32(object + Output)));
    if (s->check)
    {
        if (s->check->insertCount)
            checkInsert(*s->check, out);
        if (g.data[g.bytes] == 0xee)
            keepInsert(*s->check, g.data + g.bytes);
        s_decode(app, cpu);
        if (cpu.terminate)
            return;
        float game[GranuleSamples * 2];
        memcpy(game, out, sizeof(game));
        if (!hardwareGranule(*s, at, g, out))
        {
            failed(*s, "failed");
            memcpy(out, game, sizeof(game));
            return;
        }
        compare(*s, game, out, rc::ld32(object + GranuleBytes), g, s->paired);
        return;
    }
    rc::st32(object + FrameStart, at);
    if (g.data[0] != s->header)
    {
        s->header = g.data[0];
        parseHeader(app, cpu, object, g.data[0]);
        if (cpu.terminate)
            return;
    }
    if (!hardwareGranule(*s, at, g, out))
    {
        failed(*s, "failed");
        s_decode(app, cpu);
        return;
    }
    rc::st32(object + GranuleBytes, g.bytes);
    cpu.eax = 0;
    cpu.esp += 4;
}

// Least recently used entry when all are taken.
Stream& add(reg32 object)
{
    Stream* slot = &s_streams[0];
    for (Stream& s : s_streams)
    {
        if (!s.object)
        {
            slot = &s;
            break;
        }
        if (s.lastUse < slot->lastUse)
            slot = &s;
    }
    if (slot->object)
        release(*slot);
    slot->object = object;
    return *slot;
}

// Checks a PCM insert the stream ended with, before the samples go.
void lastInsert(Stream* s)
{
    if (s && s->check && s->check->insertCount)
        checkInsert(*s->check, reinterpret_cast<const float*>(rc::host(rc::ld32(s->object + Output))));
}

void startHook(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 object = cpu.ecx;
    const reg32 first = rc::ld32(cpu.esp + 4);
    lastInsert(find(object));
    s_start(app, cpu);
    if (cpu.terminate)
        return;
    Granule g;
    const bool eligible = ealayer3::parse(rc::host(first), g) && g.version == 3 && g.channels == 2;
    Stream* s = find(object);
    if (!eligible)
    {
        if (s)
            release(*s);
        pf::logInfo("music: stream 0x%08x starts, header 0x%02x, game decoder (not MPEG-1 stereo)", unsigned(object),
                    unsigned(rc::ld8(first)));
        return;
    }
    if (!s)
        s = &add(object);
    if (s->check)
    {
        if (s->check->dump)
            fclose(s->check->dump);
        logCheck(*s);
        delete s->check;
        s->check = nullptr;
    }
    if (!s->decoder)
        s->decoder = acquire();
    s->hardware = s->decoder && pf::mp3::reset(s->decoder);
    s->header = -1;
    s->pendingAt = 0;
    s->lastUse = s_clock;
    if (s->hardware && s_check)
    {
        s->check = new Check();
    }
    pf::logInfo("music: stream 0x%08x starts, header 0x%02x, %s", unsigned(object), unsigned(rc::ld8(first)),
                s->hardware ? "hardware decoder" : "game decoder (no hardware decoder free)");
}

void destroyHook(win32::WinApplication* app, x86::CPU& cpu)
{
    if (Stream* s = find(cpu.ecx))
    {
        lastInsert(s);
        release(*s);
    }
    s_destroy(app, cpu);
}

// Decodes an empty frame: the decoder works and gives 1152 samples.
bool selfTest()
{
    pf::mp3::Decoder* decoder = pf::mp3::create();
    if (!decoder)
        return false;
    Granule silent;
    silent.version = 3;
    silent.rateIndex = 2;
    silent.mode = 1;
    silent.channels = 2;
    uint8_t frame[ealayer3::MaxFrameBytes];
    int16_t pcm[ealayer3::FrameSamples * 2];
    const uint32_t size = ealayer3::frame(&silent, &silent, frame);
    const uint32_t start = pf::perf::now();
    const int first = pf::mp3::decode(decoder, frame, size, pcm);
    const int second = pf::mp3::decode(decoder, frame, size, pcm);
    const uint32_t us = pf::perf::now() - start;
    pf::mp3::destroy(decoder);
    pf::logInfo("music: hardware decoder test: %d and %d samples from two %u-byte frames in %u us", first, second,
                unsigned(size), unsigned(us));
    return first == int(ealayer3::FrameSamples) && second == int(ealayer3::FrameSamples);
}

}

void configure(win32::WinApplication& app)
{
    if (!pf::Config::getBool("music_hw_decode", false))
        return;
    if (!pf::mp3::init() || !selfTest())
    {
        pf::logError("music: hardware decoder unavailable (0x%08X); music_hw_decode is off",
                     unsigned(pf::mp3::lastError()));
        return;
    }
    s_check = pf::Config::getBool("music_hw_check", false);
    s_dumpPath = pf::Config::getString("music_hw_check_dump", "");
    s_decode = app.replaceMethod(DecodeGranule, &decodeHook);
    s_start = app.replaceMethod(StartStream, &startHook);
    s_destroy = app.replaceMethod(Destroy, &destroyHook);
    pf::logInfo("music: EALayer3 streams on the hardware MP3 decoder (music_hw_decode=on)%s",
                s_check ? ", compared with the game's decoder (music_hw_check)" : "");
}

}}
