#ifndef UG2_AUDIO_STATS_H_
#define UG2_AUDIO_STATS_H_

#include <lib/winapp.h>
#include <x86.h>

// Development counters of the EA SND mixer thread (dev_mixer_stats=on, needs timing_log): voices, module runs, kernel
// work and time by scope with and without the SND mutex, logged once a second.
namespace ug2 { namespace mixstats
{

enum Scope
{
    Loop, Tick, Callbacks, Notifications, Voices, Aux, Bank, Mix, Convert, Resample, Eaxa, Modules,
};

extern bool g_on;

inline bool on() { return g_on; }

// Reads dev_mixer_stats and wraps the module renders; before guest code runs.
void configure(win32::WinApplication& app);

void enter(const x86::CPU& cpu, int scope);
void leave(const x86::CPU& cpu);
// Render loop entry: the calling thread becomes the counted one.
void loopEnter(const x86::CPU& cpu, x86::reg32 frames);
void loopLeave(const x86::CPU& cpu);
void voicesEnter(const x86::CPU& cpu, x86::reg32 frames);
void voicesLeave(const x86::CPU& cpu);
void mixCall(const x86::CPU& cpu, x86::reg32 n, float gain, x86::reg32 src);
void work(const x86::CPU& cpu, int scope, uint32_t elements, uint32_t extra = 0);

// Times a scope on the counted thread.
class Scoped
{
public:
    Scoped(const x86::CPU& cpu, int scope) : m_cpu(on() ? &cpu : nullptr) { if (m_cpu) enter(cpu, scope); }
    ~Scoped() { if (m_cpu) leave(*m_cpu); }

private:
    const x86::CPU* m_cpu;
};

}}

#endif
