#ifndef UG2_AUDIO_DEFER_H_
#define UG2_AUDIO_DEFER_H_

#include <lib/winapp.h>
#include <x86.h>

// The SND tick hook (and with audio_defer_notify=on, stream notifications) delivered by a callback thread, so the mixer does not wait for the execution lock.
namespace ug2 { namespace sndq
{

// Reads audio_defer (needs audio_mixer=unlocked) and logs it; before guest code runs.
void configure(win32::WinApplication& app);
bool enabled();
// audio_defer_hook: the tick hook goes to the callback thread.
bool hookDeferred();
// True when nothing queued is waiting or being delivered (inline delivery keeps the order).
bool idle();
// idle(), or with audio_inline_with_hooks=on only tick hooks queued or running (the SND mutex orders them).
bool inlineAllowed();
// audio_defer_log: a notification batch the mixer delivered under the lock; busy when inlineAllowed() was false.
void countLocked(bool busy);
bool logging();
// Mixer thread, once per render call with the execution lock held: starts the thread, flushes a previous mixer's leftovers.
void attach(win32::WinApplication* app, x86::CPU& cpu);
// Mixer thread: moves EA's queued notifications to the ring; false (nothing moved) when it lacks room.
bool queueNotifications();
// Mixer thread: asks the callback thread to run the SND tick hook.
void requestHook();
// Calling guest thread with the execution lock: delivers everything queued, in order, before it returns.
void fence(win32::WinApplication* app, x86::CPU& cpu);
void shutdown();

}}

#endif
