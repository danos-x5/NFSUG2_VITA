#include <lib/timer.h>
#include <lib/scheduler.h>
#include <platform/cores.h>
#include <platform/log.h>
#include <platform/perf.h>
#include <platform/sync.h>
#include <platform/time.h>
#ifdef __vita__
#include <psp2/rtc.h>
#else
#include <sys/time.h>
#include <ctime>
#endif
#include <lib/thread.h>
#include <algorithm>
#include <vector>

namespace win32
{

namespace
{

struct PendingTimer
{
    x86::reg32              id;
    uint64_t                dueUs;
    uint32_t                periodUs;   // 0 for a one-shot timer
    WinApplication*         app;
    x86::reg32              callback;
    x86::reg32              parameter;
    Timer::NativeCallback   native;
    bool                    unlocked;
};

struct NativeEntry
{
    x86::reg32              callback;
    Timer::NativeCallback   native;
};

// Written before guest code runs, read-only afterwards.
std::vector<NativeEntry> s_natives;
std::vector<x86::reg32> s_unlocked;

Timer::NativeCallback findNative(x86::reg32 callback)
{
    for (const NativeEntry& entry : s_natives)
    {
        if (entry.callback == callback)
            return entry.native;
    }
    return nullptr;
}

}

// Single service thread firing pending timers in due order.
class TimerService
{
public:
    TimerService()
        :   m_mutex("timers", false)
        ,   m_cond("timers_cond", m_mutex)
        ,   m_running(true)
        ,   m_lastId(0)
        ,   m_firingId(0)
        ,   m_firingKilled(false)
        ,   m_firingUnlocked(false)
        ,   m_serviceId(0)
        ,   m_fired("timers_fired", m_mutex)
        ,   m_thread("timer",&threadEntry, this, scheduler::hostPriorityOf(TimeCriticalPriority), 1024 * 1024,
                     pf::cores::mask(pf::cores::Audio))
    {
        m_thread.start();
    }

    ~TimerService()
    {
        {
            pf::LockGuard guard(m_mutex);
            m_running = false;
            m_cond.signal();
        }
        m_thread.join();
    }

    x86::reg32 add(const PendingTimer& timer)
    {
        pf::LockGuard guard(m_mutex);
        if (++m_lastId == 0)
            ++m_lastId;
        m_pending.push_back(timer);
        m_pending.back().id = m_lastId;
        m_cond.signal();
        return m_lastId;
    }

    bool remove(x86::reg32 id)
    {
        pf::LockGuard guard(m_mutex);
        // An unlocked callback may be re-arming its timer: the kill waits, as the execution lock made it.
        while (m_firingUnlocked && Thread::currentThreadId() != m_serviceId)
            m_fired.wait();
        for (std::vector<PendingTimer>::iterator it = m_pending.begin(); it != m_pending.end(); ++it)
        {
            if (it->id == id)
            {
                m_pending.erase(it);
                return true;
            }
        }
        // A periodic timer killed while (or from) its callback runs is not re-armed.
        if (id != 0 && id == m_firingId)
        {
            m_firingKilled = true;
            return true;
        }
        return false;
    }

private:
    static int threadEntry(void* self)
    {
        static_cast<TimerService*>(self)->run();
        return 0;
    }

    void run()
    {
        Thread::registerHostThread();
        scheduler::raiseCurrentLevel(scheduler::LevelTimeCritical);
        if (pf::perf::enabled())
            pf::perf::trackThread("timer");
        m_mutex.lock();
        m_serviceId = Thread::currentThreadId();
        while (m_running)
        {
            if (m_pending.empty())
            {
                m_cond.wait();
                continue;
            }
            std::vector<PendingTimer>::iterator next = m_pending.begin();
            for (std::vector<PendingTimer>::iterator it = m_pending.begin() + 1; it != m_pending.end(); ++it)
            {
                if (it->dueUs < next->dueUs)
                    next = it;
            }
            const uint64_t now = pf::ticksUs();
            if (next->dueUs > now)
            {
                const uint64_t waitMs = (next->dueUs - now + 999) / 1000;
                m_cond.waitFor(uint32_t(waitMs));
                continue;
            }
            PendingTimer timer = *next;
            m_pending.erase(next);
            m_firingId = timer.periodUs ? timer.id : 0;
            m_firingKilled = false;
            m_firingUnlocked = timer.unlocked;
            m_mutex.unlock();
            fire(timer);
            m_mutex.lock();
            m_firingId = 0;
            if (m_firingUnlocked)
            {
                m_firingUnlocked = false;
                m_fired.broadcast();
            }
            if (timer.periodUs && !m_firingKilled && m_running)
            {
                // The next period counts from the due time; a late timer does not fire in a burst.
                timer.dueUs = std::max<uint64_t>(timer.dueUs + timer.periodUs, pf::ticksUs());
                m_pending.push_back(timer);
            }
        }
        m_mutex.unlock();
    }

    static void fire(const PendingTimer& timer)
    {
        const bool timed = pf::perf::enabled();
        const uint32_t start = timed ? pf::perf::now() : 0;
        if (timer.native)
            timer.native(timer.app, timer.callback, timer.id, timer.parameter);
        else
            Timer::runGuest(timer.app, timer.callback, timer.id, timer.parameter, !timer.unlocked);
        if (timed)
            pf::perf::timerFired(timer.callback, pf::perf::now() - start);
    }

private:
    // THREAD_PRIORITY_TIME_CRITICAL, the priority of the winmm timer thread.
    static const int TimeCriticalPriority = 15;

    pf::Mutex                   m_mutex;
    pf::CondVar                 m_cond;
    bool                        m_running;
    x86::reg32                  m_lastId;
    x86::reg32                  m_firingId;     // periodic timer whose callback runs
    bool                        m_firingKilled;
    bool                        m_firingUnlocked;   // an unlocked callback runs
    x86::reg32                  m_serviceId;
    pf::CondVar                 m_fired;
    std::vector<PendingTimer>   m_pending;
    pf::Thread                  m_thread;
};

static TimerService* s_service = nullptr;

void getSystemTime(SYSTEMTIME* systemTime)
{
#ifdef __vita__
    SceDateTime now;
    sceRtcGetCurrentClockLocalTime(&now);
    systemTime->wYear = WORD(now.year);
    systemTime->wMonth = WORD(now.month);
    systemTime->wDayOfWeek = WORD(sceRtcGetDayOfWeek(now.year, now.month, now.day));
    systemTime->wDay = WORD(now.day);
    systemTime->wHour = WORD(now.hour);
    systemTime->wMinute = WORD(now.minute);
    systemTime->wSecond = WORD(now.second);
    systemTime->wMilliseconds = WORD(now.microsecond / 1000);
#else
    timeval now;
    gettimeofday(&now, nullptr);
    tm local;
    localtime_r(&now.tv_sec, &local);
    systemTime->wYear = WORD(local.tm_year + 1900);
    systemTime->wMonth = WORD(local.tm_mon + 1);
    systemTime->wDayOfWeek = WORD(local.tm_wday);
    systemTime->wDay = WORD(local.tm_mday);
    systemTime->wHour = WORD(local.tm_hour);
    systemTime->wMinute = WORD(local.tm_min);
    systemTime->wSecond = WORD(local.tm_sec);
    systemTime->wMilliseconds = WORD(now.tv_usec / 1000);
#endif
}

x86::reg32 timeGetTickCount()
{
    return pf::ticksMs();
}

void Timer::init()
{
    if (!s_service)
        s_service = new TimerService();
}

void Timer::fini()
{
    delete s_service;
    s_service = nullptr;
}

x86::reg32 Timer::set(WinApplication* app, x86::reg32 delayMs, x86::reg32 callback, x86::reg32 parameter,
                      bool periodic)
{
    if (!s_service)
        return 0;
    const PendingTimer timer = { 0, pf::ticksUs() + uint64_t(delayMs) * 1000,
                                 periodic ? std::max<uint32_t>(delayMs, 1) * 1000 : 0, app, callback, parameter,
                                 findNative(callback),
                                 std::find(s_unlocked.begin(), s_unlocked.end(), callback) != s_unlocked.end() };
    return s_service->add(timer);
}

bool Timer::kill(x86::reg32 id)
{
    return s_service && s_service->remove(id);
}

void Timer::setNativeCallback(x86::reg32 callback, NativeCallback native)
{
    s_natives.push_back(NativeEntry{ callback, native });
}

void Timer::setUnlockedCallback(x86::reg32 callback)
{
    s_unlocked.push_back(callback);
}

void Timer::runGuest(WinApplication* app, x86::reg32 callback, x86::reg32 id, x86::reg32 parameter, bool locked)
{
    alignas(16) x86::CPU cpu{};
    app->runThread(cpu, callback, { id, 0, parameter, 0, 0 }, locked, true);
}

}
