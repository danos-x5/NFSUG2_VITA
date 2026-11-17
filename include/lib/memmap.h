#ifndef LIB_MEMMAP_H
#define LIB_MEMMAP_H

#include <x86.h>
#include <vector>

namespace pf { namespace perf { enum Alloc : int; } }

namespace win32
{

class Mutex;

// Host address of guest address 0: one memblock allocated by a static initialiser in memmap.cpp.
// Const, so compiled code need not reload it after guest stores. No static initialiser may touch
// guest memory.
extern x86::reg8* const g_guestBase;

struct Section
{
    const x86::reg8*    data;
    x86::reg32          baseAddress;
    x86::reg32          size;
};

class MemMap
{
public:
    // Low: guest blocks, small ones lowest fit, LargeSize and above highest fit below the runtime area.
    // High: buffers of the runtime itself, highest fit, the runtime area first.
    enum Placement
    {
        Low,
        High
    };
    static const x86::reg32 LargeSize = 4 * 1024 * 1024;
    static const x86::reg32 DefaultRuntimeSize = 20 * 1024 * 1024;

    // Called without the allocator lock when no range fits; returns true when it freed guest memory, and the
    // allocation is tried again.
    typedef bool (*Reclaim)();

    // Called when guest memory is exhausted, before the fatal error, to log what holds it.
    typedef void (*Report)();

    // The kind names the caller in the timing_log block counts and the exhaustion report. Exhaustion is fatal.
    MemMap(x86::reg32 size, Placement placement = Low, pf::perf::Alloc kind = pf::perf::Alloc(0));
    ~MemMap();
    // Null instead of the fatal error when no range fits after the reclaim.
    static MemMap* tryCreate(x86::reg32 size, Placement placement, pf::perf::Alloc kind);

    x86::reg32 getBlockStart() const  { return s_addressOffset + m_block * 4096; }
    x86::reg32 getBlockSize() const   { return m_blockCount * 4096; }

    // split off: every Low block is placed lowest fit.
    static void init(x86::reg32 baseAddress, const std::vector<Section>& sections, bool split = true,
                     x86::reg32 runtimeSize = DefaultRuntimeSize);
    static void fini();
    static MemMap* findBlock(x86::reg32 memIndex);
    // True when [address, address + size) lies inside the guest address space.
    static bool isGuestRange(x86::reg32 address, x86::reg32 size);
    static void setReclaim(Reclaim reclaim);
    // Up to 4 reports.
    static void addReport(Report report);
    // Logs the request, free ranges, kinds, a usage map and the added reports.
    static void report(x86::reg32 requested, pf::perf::Alloc kind, Placement placement);

    struct Usage
    {
        x86::reg32 guestUsed, guestFree, guestLargest, guestRanges;         // KB, KB, KB, count
        x86::reg32 runtimeUsed, runtimeFree, runtimeLargest, runtimeRanges;
        x86::reg32 blocks;
    };
    // The two areas of the split placement (all guest when split is off); takes the allocator lock briefly.
    static Usage usage();

    static void fillDebugGraph(x86::reg16* graph);
    static void fillDebugGraph(x86::reg32* graph);
private:
    struct Unplaced {};
    MemMap(x86::reg32 size, pf::perf::Alloc kind, Unplaced);
    // Takes a free range, after the reclaim if needed; false when none fits.
    bool place(x86::reg32 size, Placement placement);

    // First block of a free range of count blocks, or -1; s_lock held.
    static x86::reg32 findLow(x86::reg32 count);
    static x86::reg32 findHigh(x86::reg32 count, x86::reg32 limit);

    x86::reg32 m_block;
    x86::reg32 m_blockCount;
    int        m_kind;

private:
    static x86::reg8*           s_blocks;
    static x86::reg32           s_blockCount;
    static x86::reg32           s_blockSize;
    static x86::reg32           s_sectionSize;
    static x86::reg32           s_addressOffset;
    static x86::reg32           s_lowFree;      // every block below is used
    static x86::reg32           s_highFree;     // every block from here on is used
    static x86::reg32           s_runtimeStart; // first block of the runtime area
    static bool                 s_split;
    static Reclaim              s_reclaim;
    static Mutex*               s_lock;
    static x86::reg32           s_liveCount;
};

}

#endif // MEMMAP_H
