#include "bench.h"

#include <platform/config.h>
#include <platform/limiter.h>
#include <platform/log.h>
#include <platform/perf.h>
#ifdef NFSU_PGO_GENERATE
#include <platform/pgo.h>
#endif
#include <platform/sampler.h>
#include <platform/sync.h>
#include <platform/time.h>
#include <lib/window.h>

#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <vector>

namespace ug2 { namespace bench {

namespace {

const uint32_t MaxFrames = 20 * 60 * 60;

enum Phase { Idle, Recording, Stopping, Done };

std::atomic<int> s_phase{Idle};
std::vector<uint32_t> s_frames;
uint32_t s_frameCount = 0;                     // render thread
uint32_t s_lastPresent = 0;
uint32_t s_startUs = 0;
uint32_t s_limitUs = 0;                        // benchmark_seconds, 0 none
uint32_t s_limitFrames = 0;                    // benchmark_frames, 0 none
bool s_exit = false;
bool s_powerTick = true;                       // boot_power_tick
uint32_t s_tickUs = 0;
const char* s_endReason = "";
uint32_t s_windowUs = 0;                       // benchmark_window, 0 none
uint32_t s_windowStartUs = 0;
uint32_t s_windowFirst = 0;                    // first frame of the window
uint32_t s_windowIndex = 0;
WindowNote s_windowNote = nullptr;

void logSummary()
{
    const uint32_t frames = s_frameCount;
    uint64_t total = 0;
    uint32_t max = 0, over33 = 0, over42 = 0, over50 = 0;
    for (uint32_t i = 0; i < frames; ++i)
    {
        const uint32_t t = s_frames[i];
        total += t;
        max = std::max(max, t);
        over33 += t > 33400;
        over42 += t > 41700;
        over50 += t > 50000;
    }
    uint32_t p99 = 0;
    if (frames)
    {
        std::vector<uint32_t> sorted(s_frames.begin(), s_frames.begin() + frames);
        const size_t rank = std::min<size_t>(frames - 1, (size_t(frames) * 99 + 99) / 100 - 1);
        std::nth_element(sorted.begin(), sorted.begin() + rank, sorted.end());
        p99 = sorted[rank];
    }
    const double seconds = double(total) / 1000000.0;
    pf::logInfo("bench: summary (%s): %u frames in %.1f s, %.2f fps | frame avg %.2f ms p99 %.2f max %.2f"
                " | over 33.4 ms %u (%.1f%%), over 41.7 ms (a missed vblank at 30 fps) %u (%.1f%%),"
                " over 50 ms %u (%.1f%%)",
                s_endReason, unsigned(frames), seconds, seconds > 0.0 ? double(frames) / seconds : 0.0,
                frames ? double(total) / 1000.0 / frames : 0.0, double(p99) / 1000.0, double(max) / 1000.0,
                unsigned(over33), frames ? 100.0 * over33 / frames : 0.0,
                unsigned(over42), frames ? 100.0 * over42 / frames : 0.0,
                unsigned(over50), frames ? 100.0 * over50 / frames : 0.0);
    pf::limiter::report(s_frames.data(), frames);
    if (s_frameCount == s_frames.size())
        pf::logInfo("bench: frame buffer full, later frames not counted");
}

// One window's frame rate, p99 and slow frames.
void logWindow(uint32_t presentUs)
{
    const uint32_t first = s_windowFirst, frames = s_frameCount - first;
    uint64_t total = 0;
    uint32_t max = 0, over42 = 0;
    for (uint32_t i = first; i < s_frameCount; ++i)
    {
        total += s_frames[i];
        max = std::max(max, s_frames[i]);
        over42 += s_frames[i] > 41700;
    }
    uint32_t p99 = 0;
    if (frames)
    {
        std::vector<uint32_t> sorted(s_frames.begin() + first, s_frames.begin() + s_frameCount);
        const size_t rank = std::min<size_t>(frames - 1, (size_t(frames) * 99 + 99) / 100 - 1);
        std::nth_element(sorted.begin(), sorted.begin() + rank, sorted.end());
        p99 = sorted[rank];
    }
    char note[128] = "";
    if (s_windowNote)
        s_windowNote(note, sizeof(note));
    pf::logInfo("bench: window %u (%.0f-%.0f s): %u frames, %.2f fps | p99 %.2f ms max %.2f | over 41.7 ms %u | %s",
                unsigned(s_windowIndex), double(s_windowStartUs - s_startUs) / 1e6, double(presentUs - s_startUs) / 1e6,
                unsigned(frames), total ? frames * 1e6 / double(total) : 0.0, p99 / 1000.0, max / 1000.0,
                unsigned(over42), note);
    ++s_windowIndex;
    s_windowFirst = s_frameCount;
    s_windowStartUs = presentUs;
}

// Last resort if the orderly shutdown hangs.
int forceExit(void*)
{
    pf::sleepMs(15000);
    pf::logInfo("NeedForSpeedUnderground2 exit timed out, forcing");
#ifdef __vita__
    sceKernelExitProcess(0);
#else
    fflush(nullptr);
    _exit(0);
#endif
    return 0;
}

// Render thread: records frame times, logs the summary, exits.
void framePresented(uint32_t presentUs)
{
#ifdef __vita__
    // Unattended race, no input: the system's idle timers would otherwise put the app to sleep.
    if (s_powerTick && presentUs - s_tickUs >= 1000000u)
    {
        s_tickUs = presentUs;
        sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
    }
#endif
    int phase = s_phase.load(std::memory_order_acquire);
    if (phase == Recording && s_lastPresent && s_frameCount < s_frames.size())
        s_frames[s_frameCount++] = presentUs - s_lastPresent;
    s_lastPresent = presentUs;
    if (phase == Recording && s_windowUs && presentUs - s_windowStartUs >= s_windowUs)
        logWindow(presentUs);
    const bool timeUp = s_limitUs && presentUs - s_startUs >= s_limitUs;
    const bool framesUp = s_limitFrames && s_frameCount >= s_limitFrames;
    if (phase == Recording && (timeUp || framesUp))
    {
        int expected = Recording;
        if (s_phase.compare_exchange_strong(expected, Stopping, std::memory_order_acq_rel))
        {
            s_endReason = framesUp ? "frame limit" : "time limit";
            phase = Stopping;
            pf::sampler::race(false);
        }
    }
    if (phase != Stopping)
        return;
    if (s_windowUs && s_frameCount > s_windowFirst)
        logWindow(presentUs);
    logSummary();
    s_phase.store(Done, std::memory_order_release);
    if (!s_exit)
        return;
    // Quits through the game's own shutdown, so main stops every thread before the process exits.
    pf::logInfo("NeedForSpeedUnderground2 quitting (benchmark_exit)");
    static pf::Thread watchdog("bench_exit", forceExit, nullptr, pf::Thread::PriorityLow, 16 * 1024);
    watchdog.start();
    win32::Window::postQuit();
}

}

void configure(int defaultSeconds, int defaultWindow)
{
    s_limitUs = uint32_t(std::max(pf::Config::getInt("benchmark_seconds", defaultSeconds), 0)) * 1000000u;
    s_limitFrames = uint32_t(std::max(pf::Config::getInt("benchmark_frames", 0), 0));
    s_windowUs = uint32_t(std::max(pf::Config::getInt("benchmark_window", defaultWindow), 0)) * 1000000u;
    s_exit = pf::Config::getBool("benchmark_exit", false);
    s_powerTick = pf::Config::getBool("boot_power_tick", true);
    s_frames.assign(MaxFrames, 0);
    pf::perf::setFrameObserver(framePresented);
    pf::logInfo("bench: frame times from the race start %s, %s; %u s windows (benchmark_window); power ticks %s "
                "(boot_power_tick)", s_limitUs ? "for benchmark_seconds" : "to the finish",
                s_exit ? "then quit (benchmark_exit)" : "then keep running", unsigned(s_windowUs / 1000000u),
                s_powerTick ? "on" : "off");
}

void raceStarted()
{
    int expected = Idle;
    s_startUs = pf::perf::now();
    s_windowStartUs = s_startUs;
    pf::limiter::start();
    if (s_phase.compare_exchange_strong(expected, Recording, std::memory_order_acq_rel))
    {
        pf::sampler::race(true);
        pf::logInfo("bench: race started, measuring");
    }
}

void setWindowNote(WindowNote note)
{
    s_windowNote = note;
}

void raceEnded(const char* reason)
{
    if (s_phase.load(std::memory_order_acquire) != Recording)
        return;
    s_endReason = reason;
    int expected = Recording;
    if (s_phase.compare_exchange_strong(expected, Stopping, std::memory_order_acq_rel))
    {
        pf::sampler::race(false);
        pf::logInfo("bench: measurement ended: %s", reason);
    }
}

bool measurementOver()
{
    const int phase = s_phase.load(std::memory_order_acquire);
    return phase == Stopping || phase == Done;
}

}}
