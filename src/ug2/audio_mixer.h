#ifndef UG2_AUDIO_MIXER_H_
#define UG2_AUDIO_MIXER_H_

#include <lib/winapp.h>
#include <x86.h>
#include <platform/perf.h>

// EA SND render loop without the execution lock (audio_mixer.cpp).
namespace ug2 { namespace mixer
{

// Reads audio_mixer, logs the mode and runs audio_selftest; before guest code runs.
void configure(win32::WinApplication& app);

// True while the calling guest thread runs the render loop 0x725C8E without the execution lock.
bool unlocked(const x86::CPU& cpu);

// In that loop: true when a bank routine's call of library function target reaches only audited code; else false
// with the failed check (a string literal) and the guest value it failed on.
bool libraryAudited(x86::reg32 target, const x86::CPU& cpu, const char*& reason, x86::reg32& value);

// Mid-render: takes the execution lock if the audit fails now.
bool reaudit(win32::WinApplication* app, x86::CPU& cpu);

// True for the bank library functions that touch only their arguments and SND state.
bool libraryPure(x86::reg32 target);

// Holds the execution lock while it lives when the calling thread runs the render loop without it.
class Locked
{
public:
    Locked(win32::WinApplication* app, const x86::CPU& cpu, int cause = pf::perf::SndOther);
    ~Locked();

private:
    Locked(const Locked&) = delete;
    Locked& operator=(const Locked&) = delete;
    win32::WinApplication*  m_app;
    const x86::CPU&         m_cpu;
    bool                    m_taken;
    int                     m_cause;
};

}}

#endif
