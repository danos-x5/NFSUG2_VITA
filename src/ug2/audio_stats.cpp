// Mixer thread counters for dev_mixer_stats: exclusive time per scope split by SND mutex ownership, module runs per
// kind, voice chains and loudness, kernel element counts.

#include "audio_stats.h"
#include "audio_kernels.h"

#include <lib/heap.h>

#include <platform/config.h>
#include <platform/log.h>
#include <platform/perf.h>
#include <recomp.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ug2 { namespace mixstats
{

bool g_on = false;

namespace
{

using x86::reg32;
using x86::sreg32;

struct Kind
{
    reg32       render;
    const char* name;
};

const Kind s_kinds[] = {
    {0x72802d, "mpegA"}, {0x7282ef, "mpegB"}, {0x7285bd, "mpegC"}, {0x72a138, "bufsrc"}, {0x72a2f3, "hold"},
    {0x72a3d9, "sum"}, {0x72a541, "delay"}, {0x72a672, "gain"}, {0x72a7a1, "filtA"}, {0x72a8c5, "filtB"},
    {0x72a9b3, "filtC"}, {0x72aa55, "filtD"}, {0x72c941, "gate"}, {0x72cac7, "out16"}, {0x72d18d, "resamp"},
    {0x732866, "decloop"}, {0x732980, "decstream"}, {0x732b7e, "decbank"}, {0x732c84, "xastream"},
    {0x732dfb, "xaloop"}, {0x733023, "xasample"}, {0x7330f5, "pcmstream"}, {0x7331ff, "pcmloop"},
    {0x7332a3, "pcmsample"},
};
const int KindCount = int(sizeof(s_kinds) / sizeof(s_kinds[0]));
const int Outside = Modules + KindCount;
const int ScopeCount = Outside + 1;
const char* const s_scopeNames[Modules] = {"loop", "tick", "callbacks", "notify", "voices", "aux", "bank", "mix",
                                           "convert", "resample", "eaxa"};
const reg32 AuxHookFunction = 0x72655a;
const reg32 VoiceBufferA = 0x8b48d0;
const reg32 VoiceBufferB = 0x8b48d4;   // the voice scratch the mix kernel reads
const int MaxDepth = 48;
const int Signatures = 24;

struct Bucket
{
    uint64_t us[2];         // SND mutex free, held
    uint32_t calls;
    uint64_t elements;
    uint64_t extra;
};

struct Signature
{
    uint32_t key;
    uint32_t count;
};

struct Second
{
    Bucket   buckets[ScopeCount];
    uint32_t ticks, tickFrames, tickUsMax, tickHeldMax;
    uint64_t tickUs;
    uint32_t renders, renderFrames, voices, voicesMax;
    uint32_t loudness[5];   // no mix, zero signal, under -60 dBFS, under -40 dBFS, louder
    uint32_t chainModules, voiceMixes;
    uint64_t mixVoiceElements;
    uint32_t holds, holdMaxUs;
    uint64_t holdUs;
    uint32_t clockReads;
    uint64_t probeUs;       // loudness scans, left out of the scopes
    Signature signatures[Signatures];
};

win32::MethodPtr s_original[KindCount + 1];
reg32 s_fs = 0;
int s_stack[MaxDepth];
int s_depth = 0;            // may exceed MaxDepth; deeper entries count as their parent
uint32_t s_mark = 0;
bool s_held = false;
uint32_t s_holdStart = 0;
uint32_t s_tickStart = 0;
uint32_t s_tickHeld = 0;
uint32_t s_since = 0;
Second s_second;

int s_rootDepth = -1;       // depth of the voice chain root being rendered, else -1
bool s_voiceOpen = false;
uint32_t s_voiceKey = 0;
int s_voiceModules = 0;
int s_voiceMixes = 0;
float s_voicePeak = 0.0f;
float s_voiceGain = 0.0f;

uint32_t clock()
{
    ++s_second.clockReads;
    return pf::perf::now();
}

bool counted(const x86::CPU& cpu)
{
    return g_on && s_fs && cpu.efs == s_fs;
}

void flush(uint32_t now)
{
    const int top = s_depth ? s_stack[std::min(s_depth, MaxDepth) - 1] : Outside;
    s_second.buckets[top].us[s_held] += now - s_mark;
    s_mark = now;
}

void push(int scope)
{
    flush(clock());
    if (s_depth < MaxDepth)
        s_stack[s_depth] = scope;
    ++s_depth;
    ++s_second.buckets[scope].calls;
}

void pop()
{
    flush(clock());
    if (s_depth > 0)
        --s_depth;
}

void closeVoice()
{
    if (!s_voiceOpen)
        return;
    s_voiceOpen = false;
    Second& t = s_second;
    t.chainModules += s_voiceModules;
    t.voiceMixes += s_voiceMixes;
    const float level = s_voicePeak * s_voiceGain;
    const int kind = s_voiceMixes == 0 ? 0 : s_voicePeak == 0.0f ? 1 : level < 32.0f ? 2 : level < 328.0f ? 3 : 4;
    ++t.loudness[kind];
    for (Signature& s : t.signatures)
    {
        if (s.key == s_voiceKey || !s.key)
        {
            s.key = s_voiceKey;
            ++s.count;
            break;
        }
    }
}

void sndEdge(bool held)
{
    if (!g_on || !s_fs)
        return;
    const uint32_t now = clock();
    flush(now);
    if (held && !s_held)
        s_holdStart = now;
    if (!held && s_held)
    {
        const uint32_t us = now - s_holdStart;
        ++s_second.holds;
        s_second.holdUs += us;
        s_second.holdMaxUs = std::max(s_second.holdMaxUs, us);
        s_tickHeld += us;
    }
    s_held = held;
}

template <int K>
void moduleHook(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!counted(cpu))
    {
        s_original[K](app, cpu);
        return;
    }
    const reg32 frames = rc::ld32(cpu.esp + 8);
    const int top = s_depth ? s_stack[std::min(s_depth, MaxDepth) - 1] : Outside;
    const bool root = K < KindCount && top == Voices && rc::ld32(cpu.esp + 12) == rc::ld32(VoiceBufferA)
                      && rc::ld32(cpu.esp + 16) == rc::ld32(VoiceBufferB);
    if (root)
    {
        closeVoice();
        s_voiceOpen = true;
        s_voiceKey = 0;
        s_voiceModules = 0;
        s_voiceMixes = 0;
        s_voicePeak = 0.0f;
        s_voiceGain = 0.0f;
        s_rootDepth = s_depth;
    }
    if (K < KindCount && s_rootDepth >= 0 && s_depth >= s_rootDepth)
    {
        ++s_voiceModules;
        if (s_voiceModules <= 6)
            s_voiceKey = s_voiceKey * 32 + uint32_t(K + 1);
    }
    const int scope = K < KindCount ? Modules + K : Aux;
    push(scope);
    s_original[K](app, cpu);
    pop();
    if (root)
        s_rootDepth = -1;
    if (K < KindCount)
    {
        Bucket& b = s_second.buckets[scope];
        b.elements += std::max<sreg32>(0, sreg32(cpu.eax));
        b.extra += frames;
    }
}

template <int... K>
struct Hooks
{
    static void install(win32::WinApplication& app)
    {
        const win32::MethodPtr hooks[] = {&moduleHook<K>...};
        for (int k = 0; k < int(sizeof...(K)); ++k)
            s_original[k] = app.replaceMethod(k < KindCount ? s_kinds[k].render : AuxHookFunction, hooks[k]);
    }
};

using AllHooks = Hooks<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24>;
static_assert(KindCount == 24, "one hook per module kind and the aux hook");

const char* scopeName(int scope, char* buffer)
{
    if (scope < Modules)
        return s_scopeNames[scope];
    if (scope == Outside)
        return "outside";
    snprintf(buffer, 16, "m:%s", s_kinds[scope - Modules].name);
    return buffer;
}

void report(uint32_t elapsed)
{
    Second& t = s_second;
    const double ticks = t.ticks ? double(t.ticks) : 1.0;
    const double renders = t.renders ? double(t.renders) : 1.0;
    const uint32_t voices = t.loudness[0] + t.loudness[1] + t.loudness[2] + t.loudness[3] + t.loudness[4];
    pf::logInfo("mixstats: %.3f s: %u ticks %.1f frames/tick (tick %.2f ms max %.2f, held %.2f max %.2f) | %u voice "
                "renders %.1f frames, voices %.2f/render max %u | voice chains %u: %.2f modules, %.2f mixes each; "
                "loudness nomix %u zero %u <-60dB %u <-40dB %u louder %u | snd holds %u %.2f ms max %.2f | %u clock "
                "reads, scans %.2f ms",
                elapsed / 1e6, t.ticks, t.tickFrames / ticks, t.tickUs / ticks / 1000.0, t.tickUsMax / 1000.0,
                t.holdUs / ticks / 1000.0, t.tickHeldMax / 1000.0, t.renders, t.renderFrames / renders,
                t.voices / renders, t.voicesMax, voices, voices ? double(t.chainModules) / voices : 0.0,
                voices ? double(t.voiceMixes) / voices : 0.0, t.loudness[0], t.loudness[1], t.loudness[2],
                t.loudness[3], t.loudness[4], t.holds, t.holdUs / 1000.0, t.holdMaxUs / 1000.0, t.clockReads,
                t.probeUs / 1000.0);
    char line[960];
    int length = snprintf(line, sizeof(line), "mixstats: time ms/s free/held calls elements extra:");
    char name[16];
    for (int s = 0; s < ScopeCount; ++s)
    {
        const Bucket& b = t.buckets[s];
        if (!b.calls && !b.us[0] && !b.us[1])
            continue;
        if (length > int(sizeof(line)) - 90)
        {
            pf::logInfo("%s", line);
            length = snprintf(line, sizeof(line), "mixstats: time+");
        }
        length += snprintf(line + length, sizeof(line) - length, " %s %.2f/%.2f %u %llu %llu", scopeName(s, name),
                           b.us[0] / 1000.0, b.us[1] / 1000.0, b.calls, (unsigned long long)b.elements,
                           (unsigned long long)b.extra);
    }
    pf::logInfo("%s", line);
    length = snprintf(line, sizeof(line), "mixstats: voice mix elements %llu | chains:",
                      (unsigned long long)t.mixVoiceElements);
    for (const Signature& sig : t.signatures)
    {
        if (!sig.key || length > int(sizeof(line)) - 80)
            break;
        length += snprintf(line + length, sizeof(line) - length, " ");
        uint32_t digits[6];
        int n = 0;
        for (uint32_t key = sig.key; key && n < 6; key /= 32)
            digits[n++] = key % 32;
        for (int k = n - 1; k >= 0; --k)
            length += snprintf(line + length, sizeof(line) - length, "%s%s", k == n - 1 ? "" : "<",
                               digits[k] ? s_kinds[digits[k] - 1].name : "?");
        length += snprintf(line + length, sizeof(line) - length, " %u", sig.count);
    }
    pf::logInfo("%s", line);
    t = Second();
}

// Mix kernel throughput on one voice block: buffers in cache, then spread over 4 MB.
void benchKernels()
{
    const reg32 Size = 0x400000;
    const reg32 base = win32::heap::allocate(Size, true);
    if (!base)
        return;
    for (reg32 i = 0; i < Size; i += 4)
        rc::stf32(base + i, float(i % 997) * 0.25f);
    uint8_t* const m = rc::host(0);
    const reg32 n = 297;
    const reg32 Rounds = 4000;
    const reg32 Block = 0x980;
    const uint32_t t0 = pf::perf::now();
    for (reg32 k = 0; k < Rounds; ++k)
        kernels::mix(m, n, 0.5f, base, base + Block / 2);
    const uint32_t t1 = pf::perf::now();
    for (reg32 k = 0; k < Rounds; ++k)
    {
        const reg32 at = base + (k * Block) % (Size - Block);
        kernels::mix(m, n, 0.5f, at, at + Block / 2);
    }
    const uint32_t t2 = pf::perf::now();
    pf::logInfo("mixstats: mix kernel, %u floats: %.2f ns per element cached, %.2f ns spread over 4 MB (%u calls each)",
                unsigned(n), (t1 - t0) * 1000.0 / (Rounds * n), (t2 - t1) * 1000.0 / (Rounds * n), unsigned(Rounds));
    win32::heap::release(base);
}

}

void configure(win32::WinApplication& app)
{
    g_on = pf::Config::getBool("dev_mixer_stats", false);
    if (!g_on)
        return;
    AllHooks::install(app);
    const uint32_t Reads = 20000;
    const uint32_t start = pf::perf::now();
    uint32_t last = start;
    for (uint32_t i = 0; i < Reads; ++i)
        last = pf::perf::now();
    pf::logInfo("mixstats: clock read %.0f ns (%u reads in %u us)", (last - start) * 1000.0 / Reads, unsigned(Reads),
                unsigned(last - start));
    benchKernels();
    pf::perf::setSndObserver(&sndEdge);
    pf::logInfo("mixstats: counting the mixer thread (dev_mixer_stats), %d module kinds wrapped", KindCount);
}

void enter(const x86::CPU& cpu, int scope)
{
    if (counted(cpu))
        push(scope);
}

void leave(const x86::CPU& cpu)
{
    if (counted(cpu))
        pop();
}

void loopEnter(const x86::CPU& cpu, reg32 frames)
{
    if (!g_on)
        return;
    if (s_fs != cpu.efs)
    {
        s_fs = cpu.efs;
        s_depth = 0;
        s_mark = clock();
        s_since = s_mark;
        s_second = Second();
    }
    push(Loop);
    s_tickStart = s_mark;
    s_tickHeld = 0;
    ++s_second.ticks;
    s_second.tickFrames += frames;
}

void loopLeave(const x86::CPU& cpu)
{
    if (!counted(cpu))
        return;
    pop();
    Second& t = s_second;
    const uint32_t us = s_mark - s_tickStart;
    t.tickUs += us;
    t.tickUsMax = std::max(t.tickUsMax, us);
    const uint32_t held = s_tickHeld + (s_held ? s_mark - s_holdStart : 0);
    t.tickHeldMax = std::max(t.tickHeldMax, held);
    if (s_mark - s_since >= 1000000)
    {
        report(s_mark - s_since);
        s_since = s_mark;
    }
}

void voicesEnter(const x86::CPU& cpu, reg32 frames)
{
    if (!counted(cpu))
        return;
    push(Voices);
    ++s_second.renders;
    s_second.renderFrames += frames;
    const reg32 voices = rc::ld32(0x8b4a00);
    const reg32 count = rc::ld8(0x8b47f8);
    uint32_t playing = 0;
    for (reg32 i = 0; i < count; ++i)
        playing += rc::ld8(voices + i * 0x5c) == 2;
    s_second.voices += playing;
    s_second.voicesMax = std::max(s_second.voicesMax, playing);
}

void mixCall(const x86::CPU& cpu, reg32 n, float gain, reg32 src)
{
    if (!counted(cpu))
        return;
    Bucket& b = s_second.buckets[Mix];
    b.elements += n;
    if (!s_voiceOpen || src != rc::ld32(VoiceBufferB))
        return;
    s_second.mixVoiceElements += n;
    if (s_voiceMixes++ == 0)
    {
        flush(clock());
        float peak = 0.0f;
        for (reg32 i = 0; i < n; ++i)
            peak = std::max(peak, std::fabs(rc::ldf32(src + 4 * i)));
        s_voicePeak = peak;
        const uint32_t now = clock();
        s_second.probeUs += now - s_mark;
        s_mark = now;
    }
    s_voiceGain = std::max(s_voiceGain, std::fabs(gain));
}

void work(const x86::CPU& cpu, int scope, uint32_t elements, uint32_t extra)
{
    if (!counted(cpu))
        return;
    Bucket& b = s_second.buckets[scope];
    b.elements += elements;
    b.extra += extra;
}

void voicesLeave(const x86::CPU& cpu)
{
    if (!counted(cpu))
        return;
    closeVoice();
    pop();
}

}}
