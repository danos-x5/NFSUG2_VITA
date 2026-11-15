#ifndef PLATFORM_SAMPLER_H_
#define PLATFORM_SAMPLER_H_

// Sampling profiler (sampler=on): the calling thread's pc about every sampler_interval_us, read with the VM thread
// functions CapUnlocker opens, written to a file at stop (samples.py on the development branch reads it).

#include <cstdint>

namespace pf { namespace sampler
{

// Samples the calling thread from now on, when sampler=on.
void start(const char* path);
// sampler=on with sampler_thread=mixer: the audio mixer calls retarget with its thread.
bool sampleMixer();
// sampler_thread=io: the first I/O thread calls retarget with its thread.
bool sampleIo();
void retarget(int thread);
// Any thread: a pair (0xffffffff, value) goes before the next sample when value changes.
void mark(uint32_t value);
// Ends sampling and writes the file.
void stop();

// Vita: finds the pc, lr and sp context words once.
bool calibrate();
// Vita: a VM-suspended thread's registers; negative on failure.
int registers(int thread, uint32_t* pc, uint32_t* lr, uint32_t* sp);

}}

#endif
