// Sampling profiler and thread registers (sampler.h). The context layout of sceKernelGetThreadContextForVM is
// undocumented: calibrate finds the words holding pc, lr and sp from a thread spinning in a known function.
#include <platform/sampler.h>

#include <platform/config.h>
#include <platform/cores.h>
#include <platform/log.h>
#include <platform/sync.h>

#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr/eventflag.h>
#include <psp2/kernel/threadmgr.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
int sceKernelSuspendThreadForVM(SceUID thid);
int sceKernelResumeThreadForVM(SceUID thid);
int sceKernelGetThreadContextForVM(SceUID thid, void* cpuRegisters, void* vfpRegisters);
}

namespace pf { namespace sampler
{

namespace
{

const uint32_t MaxSamples = 600000;
const uint32_t ContextWords = 128;
const uint32_t Magic = 0x32504d53;     // "SMP2": pc and lr pairs
const uint32_t WaitingSample = 0xff000000u;     // | the wait type: the thread was not running
const uint32_t MarkSample = 0xffffffffu;        // followed by the mark's value

uint32_t s_samples[MaxSamples];
std::atomic<uint32_t> s_count{0};
std::atomic<bool> s_run{false};
std::atomic<bool> s_spin{true};
uint32_t s_cpu[ContextWords];
uint32_t s_vfp[ContextWords * 4];
std::atomic<SceUID> s_target{-1};
bool s_mixer = false;                            // sampler_thread=mixer
bool s_io = false;                               // sampler_thread=io
std::atomic<uint32_t> s_mark{0};
uint32_t s_markWritten = 0;
int s_pcWord = -1;
uint32_t s_intervalUs = 1000;
uint32_t s_failures = 0;
Thread* s_thread = nullptr;
std::string s_path;
FILE* s_file = nullptr;
uint32_t s_written = 0;     // words in the file
const uint32_t ChunkWords = 8192;
const uint32_t TablesAtWords = 200000;  // the copy and wait tables are logged after about 100 s of samples
bool s_tablesLogged = false;

// Appends new samples, so a hung run still leaves them.
void writeSamples()
{
    const uint32_t n = s_count.load(std::memory_order_relaxed);
    if (s_file && n > s_written)
    {
        fwrite(s_samples + s_written, 4, n - s_written, s_file);
        fflush(s_file);
        s_written = n;
    }
}

void writeHeader(uint32_t pairs)
{
    const uint32_t header[6] = {Magic, pairs, s_intervalUs, uint32_t(s_pcWord),
                                uint32_t(reinterpret_cast<uintptr_t>(&start)), s_failures};
    fseek(s_file, 0, SEEK_SET);
    fwrite(header, sizeof(header), 1, s_file);
    fseek(s_file, 0, SEEK_END);
    fflush(s_file);
}

__attribute__((noinline)) void calibrationSpin()
{
    while (s_spin.load(std::memory_order_relaxed))
    {
    }
}

std::atomic<SceUID> s_spinner{-1};

int calibrationEntry(void*)
{
    s_spinner = sceKernelGetThreadId();
    calibrationSpin();
    return 0;
}

const int IllegalSize = int(0x8002000B);
uint32_t s_cpuSize = 0, s_vfpSize = 0;

// Context call with the buffers' leading size words set.
int getContext(SceUID id, uint32_t* cpu, uint32_t* vfp)
{
    cpu[0] = s_cpuSize;
    if (s_vfpSize)
        vfp[0] = s_vfpSize;
    return sceKernelGetThreadContextForVM(id, cpu, s_vfpSize ? vfp : nullptr);
}

// Tries buffer sizes until the context call accepts them.
int findSizes(SceUID id)
{
    int r = IllegalSize, last = 0;
    for (uint32_t v : {268u, 0u})
        for (uint32_t c = 4; c <= ContextWords * 4; c += 4)
        {
            s_cpuSize = c;
            s_vfpSize = v;
            r = getContext(id, s_cpu, s_vfp);
            if (r >= 0)
                return r;
            if (r != last)
                logInfo("context: sizes cpu %u vfp %u: 0x%08x", unsigned(c), unsigned(v), unsigned(r));
            last = r;
        }
    return r;
}

struct Layout
{
    bool    done = false;
    int     pc = -1, lr = -1, sp = -1;
};
Layout s_layout;

bool within(uint32_t word, uint32_t lo, uint32_t size)
{
    return (word & ~1u) - lo < size;
}

// Context words of pc, lr and sp in calibrationSpin.
void findLayout()
{
    s_spin = true;
    Thread spinner("context_calibration", &calibrationEntry, nullptr, Thread::PriorityNormal, 16 * 1024,
                   cores::hasCore3() ? Thread::Cpu3 : Thread::Cpu2);
    spinner.start();
    sceKernelDelayThread(5000);
    const uint32_t spin = reinterpret_cast<uintptr_t>(&calibrationSpin) & ~1u;
    const uint32_t entry = reinterpret_cast<uintptr_t>(&calibrationEntry) & ~1u;
    Layout& l = s_layout;
    const SceUID id = s_spinner.load();
    int suspend = id;
    if (id >= 0 && (suspend = sceKernelSuspendThreadForVM(id)) >= 0)
    {
        SceKernelThreadInfo info;
        info.size = sizeof(info);
        const bool stackKnown = sceKernelGetThreadInfo(id, &info) >= 0;
        const uint32_t stack = uint32_t(reinterpret_cast<uintptr_t>(info.stack));
        const int r = findSizes(id);
        sceKernelResumeThreadForVM(id);
        const uint32_t words = r >= 0 ? s_cpuSize / 4 : 0;
        for (uint32_t i = 1; i < words && l.pc < 0; ++i)
            if (within(s_cpu[i], spin, 64))
                l.pc = int(i);
        if (l.pc > 1 && within(s_cpu[l.pc - 1], entry, 256))
            l.lr = l.pc - 1;
        for (uint32_t i = 1; i < words && l.lr < 0; ++i)
            if (int(i) != l.pc && within(s_cpu[i], entry, 256))
                l.lr = int(i);
        if (l.pc >= 0 && l.lr < 0)
            l.lr = l.pc - 1;
        const auto onStack = [&](int i)
        {
            const uint32_t size = uint32_t(info.stackSize);
            return stackKnown && i > 0 && i < int(words) && i != l.pc && i != l.lr
                && (s_cpu[i] - stack < size || stack - s_cpu[i] < size);
        };
        if (onStack(l.lr - 1))
            l.sp = l.lr - 1;
        for (int i = 1; i < int(words) && l.sp < 0; ++i)
            if (onStack(i))
                l.sp = i;
        logInfo("context: calibration 0x%08x, sizes cpu %u vfp %u, spin at 0x%08x: pc word %d lr %d sp %d: %08x %08x "
                "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x "
                "%08x %08x",
                unsigned(r), unsigned(s_cpuSize), unsigned(s_vfpSize), unsigned(spin), l.pc, l.lr, l.sp, s_cpu[0],
                s_cpu[1], s_cpu[2], s_cpu[3], s_cpu[4], s_cpu[5], s_cpu[6], s_cpu[7], s_cpu[8], s_cpu[9], s_cpu[10],
                s_cpu[11], s_cpu[12], s_cpu[13], s_cpu[14], s_cpu[15], s_cpu[16], s_cpu[17], s_cpu[18], s_cpu[19],
                s_cpu[20], s_cpu[21], s_cpu[22], s_cpu[23]);
    }
    else
        logInfo("context: sceKernelSuspendThreadForVM refused (0x%08x): no thread registers (CapUnlocker not loaded?)",
                unsigned(suspend));
    s_spin = false;
    spinner.join();
}

void logCopies();
void logWaits();

int samplerEntry(void*)
{
    while (s_run.load(std::memory_order_relaxed))
    {
        sceKernelDelayThread(s_intervalUs);
        const uint32_t mark = s_mark.load(std::memory_order_relaxed);
        if (mark != s_markWritten)
        {
            const uint32_t n = s_count.load(std::memory_order_relaxed);
            if (n + 2 <= MaxSamples)
            {
                s_samples[n] = MarkSample;
                s_samples[n + 1] = mark;
                s_count.store(n + 2, std::memory_order_relaxed);
                s_markWritten = mark;
            }
        }
        const SceUID target = s_target.load(std::memory_order_relaxed);
        SceKernelThreadInfo info;
        info.size = sizeof(info);
        if (sceKernelGetThreadInfo(target, &info) >= 0 && (info.status & ~(SCE_THREAD_RUNNING | SCE_THREAD_READY)))
        {
            const uint32_t n = s_count.load(std::memory_order_relaxed);
            if (n + 2 <= MaxSamples)
            {
                s_samples[n] = WaitingSample | info.waitType;
                s_samples[n + 1] = uint32_t(info.waitId);
                s_count.store(n + 2, std::memory_order_relaxed);
            }
            continue;
        }
        if (sceKernelSuspendThreadForVM(target) < 0)
        {
            ++s_failures;
            continue;
        }
        const int r = getContext(target, s_cpu, s_vfp);
        sceKernelResumeThreadForVM(target);
        const uint32_t n = s_count.load(std::memory_order_relaxed);
        if (r < 0)
            ++s_failures;
        else if (n + 2 <= MaxSamples)
        {
            s_samples[n] = s_cpu[s_pcWord];
            s_samples[n + 1] = s_cpu[s_layout.lr];
            s_count.store(n + 2, std::memory_order_relaxed);
        }
        if (s_count.load(std::memory_order_relaxed) - s_written >= ChunkWords)
            writeSamples();
        if (!s_tablesLogged && s_count.load(std::memory_order_relaxed) >= TablesAtWords)
        {
            s_tablesLogged = true;
            logCopies();
            logWaits();
        }
    }
    return 0;
}

}

// Logs the cost of clock and copy calls, and stub targets.
void timeClocks()
{
    const int Calls = 20000;
    volatile uint64_t sink = 0;
    const auto time = [&](const char* name, auto read)
    {
        const uint64_t t0 = sceKernelGetProcessTimeWide();
        for (int i = 0; i < Calls; ++i)
            sink = sink + uint64_t(read());
        const uint64_t t1 = sceKernelGetProcessTimeWide();
        logInfo("sampler: %s %.0f ns per call", name, double(t1 - t0) * 1000.0 / Calls);
    };
    time("sceKernelGetProcessTimeWide", [] { return sceKernelGetProcessTimeWide(); });
    time("sceKernelGetProcessTimeLow", [] { return sceKernelGetProcessTimeLow(); });
    time("sceKernelGetSystemTimeWide", [] { return sceKernelGetSystemTimeWide(); });
    time("sceKernelGetThreadId", [] { return sceKernelGetThreadId(); });
    uint32_t bad = 0;
    for (int i = 0; i < 100000; ++i)
    {
        const uint64_t a = sceKernelGetProcessTimeWide();
        const uint32_t low = sceKernelGetProcessTimeLow();
        const uint64_t b = sceKernelGetProcessTimeWide();
        if (uint32_t(low - uint32_t(a)) > uint32_t(b - a))
            ++bad;
    }
    logInfo("sampler: process time low word outside the wide reads around it: %u of 100000", unsigned(bad));
    static uint32_t from[64], to[64];
    for (uint32_t size : {32u, 128u, 256u})
    {
        time(size == 32 ? "sceClibMemcpy 32 bytes" : size == 128 ? "sceClibMemcpy 128 bytes" : "sceClibMemcpy 256 bytes",
             [size] { sceClibMemcpy(to, from, size); return to[0]; });
        time(size == 32 ? "word loop 32 bytes" : size == 128 ? "word loop 128 bytes" : "word loop 256 bytes", [size]
        {
            volatile uint32_t* d = to;
            for (uint32_t i = 0; i < size / 4; ++i)
                d[i] = from[i];
            return to[0];
        });
    }
    // Import stubs: ARM movw r12, movt r12, bx r12.
    const auto target = [](const void* function) -> uint32_t
    {
        const uint32_t* w = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(function) & ~1u);
        const auto imm = [](uint32_t i) { return ((i >> 4) & 0xf000) | (i & 0xfff); };
        return (w[0] & 0x0ff00000) == 0x03000000 && (w[1] & 0x0ff00000) == 0x03400000 ? imm(w[0]) | (imm(w[1]) << 16)
                                                                                       : 0;
    };
#define STUB(f) logInfo("sampler: stub %s -> 0x%08x", #f, unsigned(target(reinterpret_cast<const void*>(&f))))
    STUB(sceClibMemcpy); STUB(sceClibMemmove); STUB(sceClibMemset); STUB(sceKernelLockMutex);
    STUB(sceKernelUnlockMutex); STUB(sceKernelTryLockMutex); STUB(sceKernelSignalSema); STUB(sceKernelWaitSema);
    STUB(sceKernelPollSema); STUB(sceKernelLockLwMutex); STUB(sceKernelUnlockLwMutex); STUB(sceKernelTryLockLwMutex);
    STUB(sceKernelGetProcessTimeLow); STUB(sceKernelGetProcessTimeWide); STUB(sceKernelDelayThread);
    STUB(sceKernelSignalCond); STUB(sceKernelWaitCond); STUB(sceKernelSignalCondAll); STUB(sceKernelGetThreadInfo);
    STUB(sceKernelGetThreadId); STUB(sceKernelSetEventFlag); STUB(sceKernelWaitEventFlag);
    STUB(sceKernelClearEventFlag); STUB(sceKernelPollEventFlag); STUB(sceKernelGetMutexInfo);
#undef STUB
}

namespace
{

// Copy profile of a build linked with --wrap=memcpy,memmove,memset (build-sampler): calls and bytes by caller.
const uint32_t CopySlots = 4096;
struct CopySite
{
    uint32_t caller;
    uint32_t calls;
    uint64_t bytes;
};
CopySite s_copies[CopySlots];
std::atomic<bool> s_copying{false};

void noteCopy(void* caller, size_t bytes)
{
    if (!s_copying.load(std::memory_order_relaxed))
        return;
    const uint32_t c = uint32_t(reinterpret_cast<uintptr_t>(caller));
    for (uint32_t i = (c * 0x9e3779b1u) >> 20, n = 0; n < 64; ++n, i = (i + 1) & (CopySlots - 1))
    {
        uint32_t seen = __atomic_load_n(&s_copies[i].caller, __ATOMIC_RELAXED);
        if (seen == 0 && __atomic_compare_exchange_n(&s_copies[i].caller, &seen, c, false, __ATOMIC_RELAXED,
                                                     __ATOMIC_RELAXED))
            seen = c;
        if (seen == c)
        {
            __atomic_fetch_add(&s_copies[i].calls, 1u, __ATOMIC_RELAXED);
            __atomic_fetch_add(&s_copies[i].bytes, uint64_t(bytes), __ATOMIC_RELAXED);
            return;
        }
    }
}

// The objects the thread waited on most, by name.
void logWaits()
{
    const uint32_t n = s_count.load();
    struct Wait { uint32_t id, type, count; };
    static Wait waits[64];
    uint32_t used = 0;
    for (uint32_t i = 0; i + 1 < n; i += 2)
    {
        if (s_samples[i] < WaitingSample)
            continue;
        uint32_t k = 0;
        while (k < used && waits[k].id != s_samples[i + 1])
            ++k;
        if (k == used && used < 64)
            waits[used++] = {s_samples[i + 1], s_samples[i] & 0xffffff, 0};
        if (k < used)
            ++waits[k].count;
    }
    for (uint32_t k = 0; k < used; ++k)
    {
        if (waits[k].count * 1000 < n / 2)
            continue;
        char name[32] = "?";
        SceKernelSemaInfo sema;
        sema.size = sizeof(sema);
        SceKernelMutexInfo mutex;
        mutex.size = sizeof(mutex);
        SceKernelEventFlagInfo flag;
        flag.size = sizeof(flag);
        if (sceKernelGetSemaInfo(SceUID(waits[k].id), &sema) >= 0)
            memcpy(name, sema.name, sizeof(name));
        else if (sceKernelGetMutexInfo(SceUID(waits[k].id), &mutex) >= 0)
            memcpy(name, mutex.name, sizeof(name));
        else if (sceKernelGetEventFlagInfo(SceUID(waits[k].id), &flag) >= 0)
            memcpy(name, flag.name, sizeof(name));
        name[31] = 0;
        logInfo("sampler: waits on 0x%08x %s (type 0x%x): %.2f%%", unsigned(waits[k].id), name,
                unsigned(waits[k].type), 200.0 * waits[k].count / n);
    }
}

void logCopies()
{
    s_copying = false;
    for (int guest = 0; guest < 2; ++guest)
        for (int k = 0; k < 40; ++k)
        {
            uint32_t best = CopySlots;
            for (uint32_t i = 0; i < CopySlots; ++i)
            {
                const CopySite& c = s_copies[i];
                if (!c.caller || (c.caller < 0x1000000u) != (guest == 1))
                    continue;
                if (best == CopySlots || (guest ? c.calls > s_copies[best].calls : c.bytes > s_copies[best].bytes))
                    best = i;
            }
            if (best == CopySlots)
                break;
            logInfo("sampler: %s 0x%08x: %u calls %llu KB", guest ? "clock reads from" : "copies from",
                    unsigned(s_copies[best].caller), unsigned(s_copies[best].calls),
                    (unsigned long long)(s_copies[best].bytes / 1024));
            s_copies[best].caller = 0;
        }
}

}

extern "C"
{
void pf_noteCopy(void* caller, size_t bytes)
{
    noteCopy(caller, bytes);
}

}

bool calibrate()
{
    if (!s_layout.done)
    {
        s_layout.done = true;
        findLayout();
    }
    return s_layout.pc >= 0;
}

int registers(int thread, uint32_t* pc, uint32_t* lr, uint32_t* sp)
{
    if (s_layout.pc < 0)
        return -1;
    uint32_t cpu[ContextWords], vfp[ContextWords];
    const int r = getContext(SceUID(thread), cpu, vfp);
    if (r >= 0)
    {
        *pc = cpu[s_layout.pc];
        *lr = cpu[s_layout.lr];
        *sp = s_layout.sp >= 0 ? cpu[s_layout.sp] : 0;
    }
    return r;
}

void start(const char* path)
{
    if (!Config::getBool("sampler", false))
        return;
    timeClocks();
    int interval = 1000;
    if (Config::tryGetInt("sampler_interval_us", &interval) && interval >= 200)
        s_intervalUs = uint32_t(interval);
    s_target = sceKernelGetThreadId();
    s_mixer = strcmp(Config::getString("sampler_thread", "game"), "mixer") == 0;
    s_io = strcmp(Config::getString("sampler_thread", "game"), "io") == 0;
    if (!calibrate())
    {
        logError("sampler: no context word holds the pc; off");
        return;
    }
    s_pcWord = s_layout.pc;
    s_path = path;
    s_file = fopen(path, "wb");
    if (!s_file)
    {
        logError("sampler: %s cannot be written; off", path);
        return;
    }
    writeHeader(0);
    s_run = true;
    s_copying = Config::getBool("sampler_copies", false);
    s_thread = new Thread("sampler", &samplerEntry, nullptr, Thread::PriorityHigh, 16 * 1024,
                          cores::hasCore3() ? Thread::Cpu3 : Thread::Cpu2);
    s_thread->start();
    logInfo("sampler: thread 0x%08x every %u us, pc in context word %d%s", unsigned(s_target.load()),
            unsigned(s_intervalUs), s_pcWord, s_mixer ? " (moves to the audio mixer: sampler_thread=mixer)"
                    : s_io ? " (moves to the first I/O thread: sampler_thread=io)" : "");
}

bool sampleMixer()
{
    return s_mixer && s_run.load(std::memory_order_relaxed);
}

bool sampleIo()
{
    return s_io && s_run.load(std::memory_order_relaxed);
}

void mark(uint32_t value)
{
    s_mark.store(value, std::memory_order_relaxed);
}

void retarget(int thread)
{
    s_target.store(thread, std::memory_order_relaxed);
    logInfo("sampler: now sampling thread 0x%08x", unsigned(thread));
}

void stop()
{
    if (!s_thread)
        return;
    s_run = false;
    s_thread->join();
    delete s_thread;
    s_thread = nullptr;
    if (!s_tablesLogged)
    {
        logCopies();
        logWaits();
    }
    writeSamples();
    const uint32_t n = s_count.load();
    writeHeader(n / 2);
    fclose(s_file);
    s_file = nullptr;
    logInfo("sampler: %u samples (%u failed) written to %s", unsigned(n / 2), unsigned(s_failures), s_path.c_str());
}

}}
