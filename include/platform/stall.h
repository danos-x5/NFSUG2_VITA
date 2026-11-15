#ifndef PLATFORM_STALL_H_
#define PLATFORM_STALL_H_

// Stall reporter (stall_report, stall_seconds): when presents stop, logs each thread's kernel state and registers
// and the GXM progress below. symbolize.py (development branch) maps the addresses.

#include <atomic>
#include <cstdint>

namespace pf { namespace stall
{

enum
{
    MaxSlots = 4,
    MaxLayers = 2,
    MaxRecorders = 4
};

// GXM progress: relaxed stores by the thread owning each field, read only by the report.
struct Gxm
{
    std::atomic<uint32_t>       begun;              // last frame begun (FrameRenderer::beginFrame)
    std::atomic<uint32_t>       submitted;
    std::atomic<uint32_t>       completed;          // last frame the frame recorder saw finished
    std::atomic<uint32_t>       drawing;            // render thread: frame in drawFrame, its scene and scene count
    std::atomic<uint32_t>       scene;
    std::atomic<uint32_t>       scenes;
    std::atomic<uint32_t>       draws;              // draws of the frame in drawFrame
    std::atomic<uint32_t>       presents;           // render thread: presents queued to the display
    std::atomic<uint32_t>       flips;              // display callbacks run
    std::atomic<uint32_t>       flipped;            // present index of the last display callback
    std::atomic<int>            displayThread;
    std::atomic<uint32_t>       jobs;               // command list jobs of the last recordLists, and finished
    std::atomic<uint32_t>       jobsDone;
    std::atomic<uint32_t>       jobGeneration;
    std::atomic<uint32_t>       uploadsQueued;      // last upload ticket
    std::atomic<uint32_t>       uploadsDone;
    std::atomic<const char*>    render;             // stage markers: string literals
    std::atomic<const char*>    frames;             // the thread recording frames (beginFrame, uploads)
    std::atomic<const char*>    recorder[MaxRecorders];     // command list recorders, [0] the render thread
    std::atomic<uint32_t>       recorderJob[MaxRecorders];
    std::atomic<const volatile unsigned int*> slotDone[MaxSlots];   // notification and the value it waits for
    std::atomic<uint32_t>       slotValue[MaxSlots];
    std::atomic<const volatile unsigned int*> layerDone[MaxLayers];
    std::atomic<uint32_t>       layerValue[MaxLayers];
};

extern Gxm g_gxm;

inline void mark(std::atomic<const char*>& stage, const char* name)
{
    stage.store(name, std::memory_order_relaxed);
}

inline void set(std::atomic<uint32_t>& field, uint32_t value)
{
    field.store(value, std::memory_order_relaxed);
}

// Increments a field with a single writer.
inline void bump(std::atomic<uint32_t>& field)
{
    field.store(field.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

inline void recorder(uint32_t index, const char* name)
{
    if (index < MaxRecorders)
        mark(g_gxm.recorder[index], name);
}

inline void recorderJob(uint32_t index, uint32_t job)
{
    if (index < MaxRecorders)
        set(g_gxm.recorderJob[index], job);
}

// Main thread, after cores::init: starts the reporter when stall_report=on.
void start();
void stop();

// pf::Thread registry: the threads a report covers.
void addThread(int id, const char* name);
void removeThread(int id);

}}

#endif
