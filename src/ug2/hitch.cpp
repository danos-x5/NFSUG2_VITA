// Hitch log (hitch.h): the game thread's work per submitted frame, matched with the render thread's slow presents.

#include "hitch.h"
#include "bench.h"

#include <platform/config.h>
#include <platform/gxm.h>
#include <platform/log.h>
#include <platform/perf.h>
#include <platform/sampler.h>
#include <platform/sync.h>
#include <recomp.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace ug2 {

void sub_574670_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ce170_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace hitch {

namespace
{

namespace perf = pf::perf;
using x86::reg32;

const reg32 PeekMessageSlot = 0x00783330;   // called every frame by 0x5BF1C0
const reg32 FramesRendered = 0x0086514C;
const reg32 RaceCoordinator = 0x008900D8;
const reg32 RacingState = 4;

const uint32_t RecordSlots = 8;
const uint32_t LineSize = 1024;
const uint32_t KeptBytes = 512 * 1024;
const char LinePrefix[] = "[info] ";
const float MinExcessUs = 1000.0f;
const float AverageWeight = 1.0f / 32.0f;
const uint32_t WindowUs = 5000000;
const uint32_t StartTraceUs = 5000000;     // slow game frames are listed this long after a race start

enum Cause
{
    CauseGameCpu,
    CauseFileWait, CauseCritSecWait, CauseObjectWait, CauseSleep, CauseFrameSlot, CausePreempted, CauseReleasedOther,
    CauseD3dQueue, CauseUploadWait, CauseReadBack, CauseGuestBlocks, CauseDecompress, CauseTextureCreate, CauseSkin,
    CauseBankLoad, CauseGpuGrowth, CauseLogWrite,
    CauseHitchLog,
    CauseResolve, CauseRenderPlan, CausePrograms, CauseListRecord, CauseListWait, CauseRenderUploadWait,
    CauseScenes, CauseRenderOffCpu, CauseRenderThreadRest, CauseDisplayQueue,
    CauseCount,
    CauseUnclear = CauseCount,  // no cause over MinExcessUs
    CauseNoRecord,              // a layer present, or the record is gone
    CauseSlots
};
const int FirstInside = CauseD3dQueue;
const int LastInside = CauseLogWrite;
const int FirstRender = CauseResolve;

const char* const CauseNames[CauseSlots] = {
    "game_cpu",
    "file_wait", "cs_wait", "object_wait", "sleep", "frame_slot", "preempted", "released_other",
    "d3d_queue", "upload_wait", "read_back", "guest_blocks", "decompress", "texture_create", "skin", "bank_load",
    "gpu_growth", "log_write",
    "hitch_log",
    "resolve", "render_plan", "programs", "list_record", "list_wait", "render_upload_wait", "scenes",
    "render_off_cpu", "render_thread_rest", "display_queue",
    "unclear", "no_game_record",
};
static_assert(sizeof(CauseNames) / sizeof(CauseNames[0]) == CauseSlots, "a name per cause");

const uint32_t TotalsWords = sizeof(perf::Totals) / sizeof(uint32_t);
static_assert(sizeof(perf::Totals) % sizeof(uint32_t) == 0, "Totals holds uint32_t fields only");

// The game thread's work between two frame starts that submitted render frames.
struct Record
{
    uint32_t firstFrame = 0;    // render::Frame indexes submitted in it
    uint32_t frame = 0;
    uint32_t wallUs = 0;
    uint32_t frames = 0;        // game frames (FramesRendered steps)
    uint32_t logUs = 0;
    bool racing = false;
    uint32_t raceUs = 0;
    int state = -1;             // race coordinator state
    perf::Totals delta = {};
    float excess[FirstRender] = {};
};

struct Line
{
    char text[LineSize] = {};
    int length = 0;

    __attribute__((format(printf, 2, 3))) void append(const char* format, ...)
    {
        if (length >= int(sizeof(text)))
            return;
        va_list args;
        va_start(args, format);
        length += vsnprintf(text + length, sizeof(text) - length, format, args);
        va_end(args);
        length = std::min(length, int(sizeof(text)) - 1);
    }
};

win32::MethodPtr s_peekMessage = nullptr;
int s_gameThread = 0;
reg32 s_frameSeen = 0;

bool s_started = false;
bool s_racing = false;
uint32_t s_raceStartUs = 0;
uint32_t s_lastEndUs = 0;
perf::Totals s_lastTotals = {};
Record s_current;
Record s_records[RecordSlots];
uint32_t s_recordCount = 0;
uint32_t s_lastFrame = 0;
float s_average[FirstRender] = {};
uint32_t s_pendingLogUs = 0;

char s_kept[KeptBytes];
uint32_t s_keptBytes = 0;
uint32_t s_keptLines = 0;
uint32_t s_droppedLines = 0;

uint32_t s_counts[CauseSlots] = {};
uint32_t s_slow = 0;
uint32_t s_droppedAtReset = 0;

// Race windows.
uint32_t s_windowIndex = 0;
uint32_t s_windowStartUs = 0;
perf::Totals s_windowTotals = {};
uint32_t s_windowFrames = 0;
uint32_t s_windowSlow = 0;
uint32_t s_windowCauses[CauseSlots] = {};
perf::Totals s_raceTotals = {};
uint32_t s_raceFrames = 0;

inline double ms(uint32_t us)
{
    return double(us) / 1000.0;
}

inline uint32_t rest(uint32_t total, uint32_t parts)
{
    return total > parts ? total - parts : 0;
}

int coordinatorState()
{
    const reg32 coordinator = rc::ld32(RaceCoordinator);
    return coordinator ? int(rc::ld32(coordinator)) : -1;
}

void addDelta(perf::Totals& sum, const perf::Totals& now, const perf::Totals& before)
{
    uint32_t* s = reinterpret_cast<uint32_t*>(&sum);
    const uint32_t* n = reinterpret_cast<const uint32_t*>(&now);
    const uint32_t* b = reinterpret_cast<const uint32_t*>(&before);
    for (uint32_t i = 0; i < TotalsWords; ++i)
        s[i] += n[i] - b[i];
}

perf::Totals difference(const perf::Totals& now, const perf::Totals& before)
{
    perf::Totals d = {};
    addDelta(d, now, before);
    return d;
}

uint32_t released(const perf::Totals& t)
{
    uint32_t sum = 0;
    for (int i = 0; i < perf::BlockCount; ++i)
        sum += t.blocked[i];
    return sum;
}

void gameCauses(const Record& r, float* v)
{
    const perf::Totals& t = r.delta;
    const uint32_t* g = t.gameEvents;
    v[CauseGameCpu] = float(rest(r.wallUs, released(t) + r.logUs));
    const uint32_t slot = t.events[perf::EventSlotWaitUs];
    v[CauseFileWait] = float(t.blocked[perf::BlockFile]);
    v[CauseCritSecWait] = float(t.blocked[perf::BlockCritSec]);
    v[CauseObjectWait] = float(t.blocked[perf::BlockWait]);
    v[CauseSleep] = float(t.blocked[perf::BlockSleep]);
    v[CauseFrameSlot] = float(slot);
    v[CausePreempted] = float(t.blocked[perf::BlockPreempt]);
    v[CauseReleasedOther] = float(rest(t.blocked[perf::BlockOther] + t.blocked[perf::BlockSwap], slot));
    v[CauseD3dQueue] = float(t.events[perf::EventDrainWaitUs]);
    v[CauseUploadWait] = float(t.uploads[perf::UploadGameWaitUs]);
    v[CauseReadBack] = float(t.uploads[perf::UploadReadBackUs]);
    v[CauseGuestBlocks] = float(t.uploads[perf::UploadGuestBlockUs]);
    v[CauseDecompress] = float(g[perf::EventJdlzUs]);
    v[CauseTextureCreate] = float(g[perf::EventTextureCreateUs]);
    v[CauseSkin] = float(g[perf::EventSkinUs]);
    v[CauseBankLoad] = float(g[perf::EventBankLoadUs]);
    v[CauseGpuGrowth] = float(g[perf::EventGpuGrowthUs]);
    v[CauseLogWrite] = float(g[perf::EventLogWriteUs]);
    v[CauseHitchLog] = float(r.logUs);
}

void renderCauses(uint32_t render, uint32_t queue, const uint32_t* parts, float* v)
{
    const uint32_t lists = parts[perf::RenderLists], listWait = parts[perf::RenderListWait];
    const uint32_t uploadWait = parts[perf::RenderUploadWait];
    v[CauseResolve] = float(parts[perf::RenderResolve]);
    v[CauseRenderPlan] = float(parts[perf::RenderPlan]);
    v[CausePrograms] = float(parts[perf::RenderPrograms]);
    v[CauseListRecord] = float(rest(lists, listWait));
    v[CauseListWait] = float(listWait);
    v[CauseRenderUploadWait] = float(uploadWait);
    v[CauseScenes] = float(parts[perf::RenderScenes]);
    v[CauseRenderOffCpu] = float(rest(rest(render, queue + listWait + uploadWait), parts[perf::RenderCpu]));
    v[CauseRenderThreadRest] = float(rest(render, queue + parts[perf::RenderResolve] + parts[perf::RenderPlan]
                                                 + parts[perf::RenderPrograms] + lists + uploadWait
                                                 + parts[perf::RenderScenes]));
    v[CauseDisplayQueue] = float(queue);
}

const Record* findRecord(uint32_t frame)
{
    const uint32_t count = std::min(s_recordCount, RecordSlots);
    for (uint32_t i = 0; i < count; ++i)
    {
        const Record& r = s_records[(s_recordCount - 1 - i) % RecordSlots];
        if (int32_t(frame - r.firstFrame) >= 0 && int32_t(r.frame - frame) >= 0)
            return &r;
    }
    return nullptr;
}

// Largest excess; a wait or load call inside game_cpu wins at half of its excess.
int dominant(const float* excess, float* largest)
{
    int best = -1;
    float bestExcess = MinExcessUs;
    int inside = -1;
    float insideExcess = 0.0f;
    for (int c = 0; c < CauseCount; ++c)
    {
        if (excess[c] > bestExcess)
        {
            best = c;
            bestExcess = excess[c];
        }
        if (c >= FirstInside && c <= LastInside && excess[c] > insideExcess)
        {
            inside = c;
            insideExcess = excess[c];
        }
    }
    if (best < 0)
    {
        *largest = 0.0f;
        return CauseUnclear;
    }
    if (best == CauseGameCpu && inside >= 0 && insideExcess * 2.0f >= bestExcess)
    {
        best = inside;
        bestExcess = insideExcess;
    }
    *largest = bestExcess;
    return best;
}

void appendWhen(Line& line, bool racing, uint32_t raceUs, int state)
{
    if (racing)
        line.append("race %.2f s", double(raceUs) / 1000000.0);
    else
        line.append("not racing (race coordinator state %d)", state);
}

void appendRender(Line& line, const perf::SlowPresent& p)
{
    const uint32_t* q = p.parts;
    const uint32_t* a = p.partAvgs;
    line.append(" | frame %u interval %.1f ms, render %.1f (avg %.1f): resolve %.1f plan %.1f programs %.1f"
                " (%u new %.1f) lists %.1f (wait %.1f) tex waits %.1f scenes %.1f queue %.1f, cpu %.1f (avg %.1f);"
                " draws %u (avg %u)",
                unsigned(p.frame), ms(p.intervalUs), ms(p.renderUs), ms(p.renderAvgUs), ms(q[perf::RenderResolve]),
                ms(q[perf::RenderPlan]), ms(q[perf::RenderPrograms]), unsigned(q[perf::RenderProgramCreates]),
                ms(q[perf::RenderProgramCreateUs]), ms(q[perf::RenderLists]), ms(q[perf::RenderListWait]),
                ms(q[perf::RenderUploadWait]), ms(q[perf::RenderScenes]), ms(p.queueUs), ms(q[perf::RenderCpu]),
                ms(a[perf::RenderCpu]), unsigned(q[perf::RenderDrawCount]), unsigned(a[perf::RenderDrawCount]));
}

// Load events: all threads, then the game thread's part.
void appendLoads(Line& line, const perf::Totals& t)
{
    const uint32_t* e = t.events;
    const uint32_t* g = t.gameEvents;
    line.append(" | file reads %u %.0f KB %.1f ms (game %u %.1f) | decompress %u %.1f (game %u %.1f),"
                " tex creates %u %.1f (game %u %.1f), skins %u %.1f, banks %u %.1f (game %u %.1f)",
                unsigned(e[perf::EventFileReads]), double(e[perf::EventFileBytes]) / 1024.0,
                ms(e[perf::EventFileUs]), unsigned(g[perf::EventFileReads]), ms(g[perf::EventFileUs]),
                unsigned(e[perf::EventJdlz]), ms(e[perf::EventJdlzUs]), unsigned(g[perf::EventJdlz]),
                ms(g[perf::EventJdlzUs]), unsigned(e[perf::EventTextureCreates]),
                ms(e[perf::EventTextureCreateUs]), unsigned(g[perf::EventTextureCreates]),
                ms(g[perf::EventTextureCreateUs]), unsigned(e[perf::EventSkins]), ms(e[perf::EventSkinUs]),
                unsigned(e[perf::EventBankLoads]), ms(e[perf::EventBankLoadUs]), unsigned(g[perf::EventBankLoads]),
                ms(g[perf::EventBankLoadUs]));
    line.append(" | uploads %u %.0f KB (waits game %u %.1f render %u %.1f), read-backs %u %.1f, guest blocks %u %.1f,"
                " d3d waits %u %.1f",
                unsigned(t.uploads[perf::UploadLevels]), double(t.uploads[perf::UploadBytes]) / 1024.0,
                unsigned(t.uploads[perf::UploadGameWaits]), ms(t.uploads[perf::UploadGameWaitUs]),
                unsigned(t.uploads[perf::UploadRenderWaits]), ms(t.uploads[perf::UploadRenderWaitUs]),
                unsigned(t.uploads[perf::UploadReadBacks]), ms(t.uploads[perf::UploadReadBackUs]),
                unsigned(t.uploads[perf::UploadGuestBlocks]), ms(t.uploads[perf::UploadGuestBlockUs]),
                unsigned(e[perf::EventDrainWaits]), ms(e[perf::EventDrainWaitUs]));
    if (e[perf::EventGpuGrowths])
        line.append(" | gpu grown %ux %u KB %.1f", unsigned(e[perf::EventGpuGrowths]),
                    unsigned(e[perf::EventGpuGrowthKb]), ms(e[perf::EventGpuGrowthUs]));
    if (e[perf::EventLogWrites])
        line.append(" | log writes %u %.1f (game %u %.1f)", unsigned(e[perf::EventLogWrites]),
                    ms(e[perf::EventLogWriteUs]), unsigned(g[perf::EventLogWrites]), ms(g[perf::EventLogWriteUs]));
}

void appendReleases(Line& line, uint32_t wallUs, uint32_t logUs, const perf::Totals& t)
{
    line.append(" | game %.1f ms: cpu %.1f, released %.1f (file %.1f cs %.1f wait %.1f sleep %.1f preempt %.1f"
                " other %.1f; %ux relock %.1f), slot %.1f (fence %.1f), held by io %.1f timer %.1f helper %.1f",
                ms(wallUs), ms(rest(wallUs, released(t) + logUs)), ms(released(t)), ms(t.blocked[perf::BlockFile]),
                ms(t.blocked[perf::BlockCritSec]), ms(t.blocked[perf::BlockWait]), ms(t.blocked[perf::BlockSleep]),
                ms(t.blocked[perf::BlockPreempt]), ms(t.blocked[perf::BlockOther] + t.blocked[perf::BlockSwap]),
                unsigned(t.releases), ms(t.relockUs), ms(t.events[perf::EventSlotWaitUs]),
                ms(t.events[perf::EventFenceWaitUs]), ms(t.held[perf::HolderIo]), ms(t.held[perf::HolderTimer]),
                ms(t.held[perf::HolderHelper]));
}

void keep(const char* text, int length)
{
    const uint32_t prefix = sizeof(LinePrefix) - 1;
    const uint32_t size = uint32_t(std::min(length, int(LineSize) - 1));
    if (s_keptBytes + prefix + size + 1 > KeptBytes)
    {
        ++s_droppedLines;
        return;
    }
    memcpy(s_kept + s_keptBytes, LinePrefix, prefix);
    memcpy(s_kept + s_keptBytes + prefix, text, size);
    s_keptBytes += prefix + size;
    s_kept[s_keptBytes++] = '\n';
    ++s_keptLines;
}

void flush()
{
    if (!s_keptLines && !s_droppedLines)
        return;
    pf::logInfo("hitch: %u lines kept since the last write, %u dropped (no space):", unsigned(s_keptLines),
                unsigned(s_droppedLines));
    pf::logBlock(s_kept, s_keptBytes);
    s_keptBytes = 0;
    s_keptLines = 0;
    s_droppedLines = 0;
}

void report(const perf::SlowPresent& present, const Record* r)
{
    float excess[CauseCount] = {};
    if (r)
        memcpy(excess, r->excess, sizeof(r->excess));
    float value[CauseCount], average[CauseCount];
    renderCauses(present.renderUs, present.queueUs, present.parts, value);
    renderCauses(present.renderAvgUs, present.queueAvgUs, present.partAvgs, average);
    for (int c = FirstRender; c < CauseCount; ++c)
        excess[c] = value[c] - average[c];
    float largest = 0.0f;
    int cause = dominant(excess, &largest);
    if (!r && cause == CauseUnclear)
        cause = CauseNoRecord;
    const bool racing = r ? r->racing : s_racing;
    if (racing)
    {
        ++s_counts[cause];
        ++s_slow;
        ++s_windowSlow;
        ++s_windowCauses[cause];
    }

    Line line;
    line.append("hitch: ");
    if (r)
        appendWhen(line, r->racing, r->raceUs, r->state);
    else
        appendWhen(line, s_racing, s_racing ? present.presentUs - s_raceStartUs : 0, coordinatorState());
    appendRender(line, present);
    if (r)
    {
        line.append(" | %u game frame%s", unsigned(r->frames), r->frames == 1 ? "" : "s");
        appendReleases(line, r->wallUs, r->logUs, r->delta);
        appendLoads(line, r->delta);
        if (r->logUs)
            line.append(" | hitch log %.1f", ms(r->logUs));
    }
    else
        line.append(" | no game frame record%s", present.frame ? "" : " (a layer present)");
    if (cause < CauseCount)
        line.append(" | dominant %s +%.1f ms", CauseNames[cause], double(largest) / 1000.0);
    else
        line.append(" | dominant %s", CauseNames[cause]);
    keep(line.text, line.length);
}

// A game thread record on its own line, whatever its presents took.
void reportGame(const Record& r, const char* what)
{
    Line line;
    line.append("hitch: %s, ", what);
    appendWhen(line, r.racing, r.raceUs, r.state);
    line.append(" | frames %u-%u | %u game frame%s", unsigned(r.firstFrame), unsigned(r.frame), unsigned(r.frames),
                r.frames == 1 ? "" : "s");
    appendReleases(line, r.wallUs, r.logUs, r.delta);
    appendLoads(line, r.delta);
    keep(line.text, line.length);
}

void reportPresents()
{
    while (const perf::SlowPresent* present = perf::peekSlowPresent())
    {
        if (present->frame && int32_t(present->frame - s_lastFrame) > 0)
            break;
        report(*present, present->frame ? findRecord(present->frame) : nullptr);
        perf::popSlowPresent();
    }
}

void appendCauses(Line& line, const uint32_t* counts)
{
    int order[CauseSlots];
    int shown = 0;
    for (int c = 0; c < CauseSlots; ++c)
    {
        if (counts[c])
            order[shown++] = c;
    }
    std::sort(order, order + shown, [counts](int a, int b) { return counts[a] > counts[b]; });
    for (int i = 0; i < shown; ++i)
        line.append("%s %s %u", i ? "," : "", CauseNames[order[i]], unsigned(counts[order[i]]));
}

// One line of totals: a window or the whole race.
void logTotals(const char* what, uint32_t index, uint32_t fromUs, uint32_t toUs, uint32_t frames,
               const perf::Totals& t, uint32_t slow, const uint32_t* causes)
{
    Line line;
    if (index)
        line.append("hitch: %s %u (%.0f-%.0f s): %u game frames, %u slow presents", what, unsigned(index),
                    double(fromUs) / 1e6, double(toUs) / 1e6, unsigned(frames), unsigned(slow));
    else
        line.append("hitch: %s (%.1f s): %u game frames, %u slow presents", what, double(toUs - fromUs) / 1e6,
                    unsigned(frames), unsigned(slow));
    appendReleases(line, toUs - fromUs, 0, t);
    appendLoads(line, t);
    if (causes && slow)
    {
        line.append(" | slow by cause:");
        appendCauses(line, causes);
    }
    keep(line.text, line.length);
}

void closeWindow(uint32_t now, const perf::Totals& totals)
{
    const perf::Totals d = difference(totals, s_windowTotals);
    ++s_windowIndex;
    logTotals("window", s_windowIndex, s_windowStartUs - s_raceStartUs, now - s_raceStartUs, s_windowFrames, d,
              s_windowSlow, s_windowCauses);
    s_windowStartUs = now;
    s_windowTotals = totals;
    s_windowFrames = 0;
    s_windowSlow = 0;
    memset(s_windowCauses, 0, sizeof(s_windowCauses));
    pf::sampler::mark(s_windowIndex + 1);
}

void resetSummary()
{
    memset(s_counts, 0, sizeof(s_counts));
    s_slow = 0;
    s_droppedAtReset = perf::droppedSlowPresents();
}

void logSummary(const char* label)
{
    flush();
    Line line;
    line.append("hitch: summary (%s): %u frames over %.1f ms by dominant cause:", label, unsigned(s_slow),
                ms(perf::HitchUs));
    appendCauses(line, s_counts);
    const uint32_t dropped = perf::droppedSlowPresents() - s_droppedAtReset;
    if (dropped)
        line.append("; %u slow presents dropped (not logged)", unsigned(dropped));
    pf::logInfo("%s", line.text);
    resetSummary();
}

// Game thread, at each game frame.
void frame()
{
    const uint32_t now = perf::now();
    perf::Totals totals;
    perf::totals(totals);
    const int state = coordinatorState();
    const bool racing = state == int(RacingState) && !bench::measurementOver();
    const bool wasRacing = s_racing;
    if (racing && !wasRacing)
    {
        s_raceStartUs = now;
        s_windowStartUs = now;
        s_windowTotals = totals;
        s_raceTotals = totals;
        s_windowIndex = 0;
        s_windowFrames = 0;
        s_windowSlow = 0;
        s_raceFrames = 0;
        memset(s_windowCauses, 0, sizeof(s_windowCauses));
        pf::sampler::mark(1);
    }
    s_racing = racing;
    if (!s_started)
    {
        s_started = true;
        s_lastEndUs = now;
        s_lastTotals = totals;
        s_lastFrame = pf::gxm::Device::get().lastSubmittedFrame();
        pf::logInfo("hitch: hitch log on (hitch_log): one line per present over %.1f ms and %u s window totals while"
                    " racing, kept in memory and written outside races", ms(perf::HitchUs),
                    unsigned(WindowUs / 1000000));
        return;
    }

    Record& c = s_current;
    c.wallUs += now - s_lastEndUs;
    ++c.frames;
    c.logUs += s_pendingLogUs;
    s_pendingLogUs = 0;
    addDelta(c.delta, totals, s_lastTotals);
    s_lastEndUs = now;
    s_lastTotals = totals;
    if (racing)
    {
        ++s_windowFrames;
        ++s_raceFrames;
    }
    const uint32_t submitted = pf::gxm::Device::get().lastSubmittedFrame();
    if (submitted != s_lastFrame)
    {
        c.firstFrame = s_lastFrame + 1;
        c.frame = submitted;
        c.racing = racing;
        c.raceUs = racing ? now - s_raceStartUs : 0;
        c.state = state;
        float v[FirstRender];
        gameCauses(c, v);
        for (int i = 0; i < FirstRender; ++i)
        {
            c.excess[i] = v[i] - s_average[i];
            s_average[i] += (v[i] - s_average[i]) * AverageWeight;
        }
        s_records[s_recordCount++ % RecordSlots] = c;
        if (racing && c.raceUs < StartTraceUs && c.wallUs > perf::HitchUs)
            reportGame(c, "slow game frame after the start");
        s_lastFrame = submitted;
        c = Record();
    }

    const bool raceEnded = wasRacing && !racing;
    const bool raceStarted = racing && !wasRacing;
    const bool windowDone = racing && now - s_windowStartUs >= WindowUs;
    const bool writeKept = s_keptLines && !racing && s_keptBytes >= KeptBytes / 4 * 3;
    if (!perf::peekSlowPresent() && !raceEnded && !raceStarted && !writeKept && !windowDone)
        return;
    const uint32_t start = perf::now();
    reportPresents();
    if (windowDone)
        closeWindow(now, totals);
    if (raceEnded)
    {
        if (now != s_windowStartUs)
            closeWindow(now, totals);
        logTotals("race totals", 0, s_raceStartUs, now, s_raceFrames, difference(totals, s_raceTotals), s_slow,
                  nullptr);
        pf::sampler::mark(0xffff);
        logSummary("race");
    }
    if (raceStarted)
    {
        resetSummary();
        const uint32_t count = std::min(s_recordCount, RecordSlots);
        for (uint32_t i = count; i > 0; --i)
            reportGame(s_records[(s_recordCount - i) % RecordSlots], "game frame around the start");
    }
    if (writeKept)
        flush();
    s_pendingLogUs += perf::now() - start;
}

void peekMessageHook(win32::WinApplication* app, x86::CPU& cpu)
{
    s_peekMessage(app, cpu);
    const reg32 frames = rc::ld32(FramesRendered);
    if (frames != s_frameSeen && pf::Thread::currentId() == s_gameThread)
    {
        s_frameSeen = frames;
        frame();
    }
}

template <perf::Event Calls, typename Body>
void timed(Body body)
{
    if (!perf::hitch())
    {
        body();
        return;
    }
    const uint32_t start = perf::now();
    body();
    perf::event(Calls, 1);
    perf::event(perf::Event(Calls + 1), perf::now() - start);
}

}

void configure(win32::WinApplication& app)
{
    if (!perf::hitch())
        return;
    s_gameThread = pf::Thread::currentId();
    perf::setGameThread(s_gameThread);
    s_peekMessage = app.replaceMethod(rc::ld32(PeekMessageSlot), peekMessageHook);
    pf::logInfo("hitch: frames counted at PeekMessageA on thread 0x%x; timed: decompression 0x574670, texture"
                " creation 0x5ce170, audio bank loads", unsigned(s_gameThread));
}

}

namespace native
{

// Decompression dispatcher (cdecl; source, destination): COMP, HUFF, RAWW, JDLZ.
void sub_574670(win32::WinApplication* app, x86::CPU& cpu)
{
    hitch::timed<pf::perf::EventJdlz>([&] { sub_574670_guest(app, cpu); });
}

// Texture creation of a texture record (thiscall).
void sub_5ce170(win32::WinApplication* app, x86::CPU& cpu)
{
    static const bool batch = pf::Config::getBool("texture_batch", true);
    if (batch)
        pf::gxm::Device::get().beginUploads();
    hitch::timed<pf::perf::EventTextureCreates>([&] { sub_5ce170_guest(app, cpu); });
    if (batch)
        pf::gxm::Device::get().endUploads();
}

}

}
