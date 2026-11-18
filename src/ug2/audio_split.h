#ifndef UG2_AUDIO_SPLIT_H_
#define UG2_AUDIO_SPLIT_H_

#include <lib/winapp.h>
#include <x86.h>

// EA SND voice render outside the SND mutex (audio_mixer_split) and the dev watch of other threads' holds.
namespace ug2 { namespace split
{

// Reads the settings; before guest code runs.
void configure(win32::WinApplication& app);

bool on();

// Other threads, at the start of an SND function that touches render state: waits until the render is done.
void gate(win32::WinApplication* app, const x86::CPU& cpu);

// Gain changes of a voice (by index) so far.
uint32_t gainWrites(x86::reg32 voice);

// Mixer, around one voice render: gives the SND mutex up while it lives, takes it back at the end or at a voice end.
class Window
{
public:
    Window(win32::WinApplication* app, x86::CPU& cpu);
    ~Window();

private:
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    x86::CPU&   m_cpu;
    bool        m_open;
};

// Mixer, around the render loop 0x725C8E (dev_snd_watch): learns the fields the mixer writes.
void loopBegin(const x86::CPU& cpu);
void loopEnd(const x86::CPU& cpu);

}}

#endif
