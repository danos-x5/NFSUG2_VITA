#ifndef UG2_AUDIO_CULL_H_
#define UG2_AUDIO_CULL_H_

#include <lib/winapp.h>
#include <x86.h>

// Silent-voice culling in the EA SND mixer (audio_silence_db): voices below the threshold skip resampler kernels,
// filters and the mix; their sources keep running, so positions, loops and notifications stay in step.
namespace ug2 { namespace cull
{

// Reads audio_silence_db and installs the module hooks; before guest code runs.
void configure(win32::WinApplication& app);

// Around one voice render call 0x729574: marks the silent voices, restores their gains afterwards.
class Voices
{
public:
    explicit Voices(const x86::CPU& cpu);
    ~Voices();

private:
    Voices(const Voices&) = delete;
    Voices& operator=(const Voices&) = delete;
    bool m_on;
};

// True when the resampler kernel call now running belongs to a silent voice.
bool skipKernel(const x86::CPU& cpu);

// Advance the linear (0x733700) or cubic (0x733388) kernel's position and fraction as its n samples would, then
// return from it; the cubic one declines (false) for steps it does not model.
void advanceLinear(x86::CPU& cpu);
bool advanceCubic(x86::CPU& cpu);

// audio_cull_check: nothing is skipped; the kernels run and their results are compared with the advance.
bool checking();

class KernelCheck
{
public:
    KernelCheck(const x86::CPU& cpu, bool cubic);
    void finish() const;

private:
    x86::reg32 m_position, m_fraction, m_predictedPosition = 0, m_predictedFraction = 0;
    bool       m_valid = false;
};

// Compares the advances with the guest kernels on random data (audio_selftest).
void selfTest(win32::WinApplication& app);

// audio_cull_check: the mix kernel call about to run (voice in esi), adds what culling would drop to a shadow sum.
void checkMix(const x86::CPU& cpu);

}}

#endif
