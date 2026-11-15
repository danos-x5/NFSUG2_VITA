// Native sub_5cf420: draw-queue flush over the 16 x 23 list heads. Guest frame kept, registers live in the CPU; children run through entry points, CommitChanges and EndPass through the bridge.

#include <native/common.h>
#include <platform/cached.h>
#include <platform/config.h>
#include <scenery.h>
#include <winapi/d3d9_native.h>

#include <initializer_list>

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
#include <arm_neon.h>
#endif

namespace ug2
{

void sub_5cf420_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bcc30(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bdde0(win32::WinApplication* app, x86::CPU& cpu);
void sub_48b1e0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bdaa0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bd140(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bd4a0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bdcd0(win32::WinApplication* app, x86::CPU& cpu);
void sub_60fe00(win32::WinApplication* app, x86::CPU& cpu);
void sub_4901d0(win32::WinApplication* app, x86::CPU& cpu);
void sub_60fde0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43cdf0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43f920(win32::WinApplication* app, x86::CPU& cpu);
void sub_5cea20(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

namespace br = win32::d3d9::bridge;
using x86::reg32;
using x86::sreg32;
typedef void (*Child)(win32::WinApplication*, x86::CPU&);

const reg32 CurrentGroup = 0x870910;
const reg32 DevicePointer = 0x870974;
const reg32 CurrentView = 0x870848;
const reg32 LastStream = 0x87092c;
const reg32 LastIndices = 0x870930;
const reg32 EntryPool = 0x87091c;
const reg32 BucketPool = 0x870918;
const reg32 StripPool = 0x870954;
const reg32 Ring = 0x8a07a8;

// The guest's callee-saved registers, kept in the CPU, and its frame base (esp after the prologue).
struct State
{
    reg32& ebx;
    reg32& ebp;
    reg32& esi;
    reg32& edi;
    reg32 frame;
};

// flush_direct=off: CommitChanges and EndPass through the vtable.
bool s_direct = true;

inline void increment(reg32 address) { rc::st32(address, rc::ld32(address) + 1); }

inline reg32 third(reg32 value) { return reg32((uint64_t(0xaaaaaaabu) * value) >> 32) >> 1; }

inline void toCpu(x86::CPU& cpu, const State&, reg32 esp)
{
    cpu.esp = esp;
}

inline void fromCpu(const x86::CPU&, State&) {}

// Leaf child: the guest assumes its esp.
inline bool leaf(win32::WinApplication* app, x86::CPU& cpu, const State& s, Child child, reg32 esp)
{
    toCpu(cpu, s, esp);
    child(app, cpu);
    return !cpu.terminate;
}

// Child after which the guest reloads esp.
inline bool call(win32::WinApplication* app, x86::CPU& cpu, State& s, Child child, reg32 esp, reg32 cleanup)
{
    toCpu(cpu, s, esp);
    child(app, cpu);
    if (cpu.terminate)
        return false;
    fromCpu(cpu, s);
    s.frame = cpu.esp + cleanup;
    return true;
}

// stdcall COM method through the guest vtable.
bool com(win32::WinApplication* app, x86::CPU& cpu, State& s, reg32 object, reg32 slot, std::initializer_list<reg32> args)
{
    reg32 at = s.frame;
    for (const reg32* a = args.end(); a != args.begin();)
        rc::st32(at -= 4, *--a);
    rc::st32(at -= 4, object);
    toCpu(cpu, s, at - 4);
    br::callGuest(app, cpu, rc::ld32(rc::ld32(object) + 4 * slot));
    if (cpu.terminate)
        return false;
    fromCpu(cpu, s);
    s.frame = cpu.esp;
    return true;
}

// Effect CommitChanges (vtable slot 67).
inline bool commitChanges(win32::WinApplication* app, x86::CPU& cpu, State& s, reg32 effect)
{
    if (__builtin_expect(s_direct, 1))
    {
        app->allowPreemption(cpu);
        if (__builtin_expect(br::effectCommitChanges(app, cpu, effect), 1))
            return true;
    }
    return com(app, cpu, s, effect, 67, {});
}

// Effect EndPass (vtable slot 68).
inline bool endPass(win32::WinApplication* app, x86::CPU& cpu, State& s, reg32 effect)
{
    if (__builtin_expect(s_direct, 1))
    {
        app->allowPreemption(cpu);
        if (__builtin_expect(br::effectEndPass(app, cpu, effect), 1))
            return true;
    }
    return com(app, cpu, s, effect, 68, {});
}

// thiscall group function (vtable +4 begin, +8 end).
bool group(win32::WinApplication* app, x86::CPU& cpu, State& s, reg32 g, reg32 offset)
{
    toCpu(cpu, s, s.frame - 4);
    cpu.ecx = g;
    br::callGuest(app, cpu, rc::ld32(rc::ld32(g) + offset));
    if (cpu.terminate)
        return false;
    fromCpu(cpu, s);
    s.frame = cpu.esp;
    return true;
}

// rep movs as the translated code runs it.
void repMove(x86::CPU& cpu, reg32& si, reg32& di, reg32 count, reg32 size)
{
    if (rc::repMove(cpu.flags.df, cpu.ees + di, si, count, size))
    {
        si += count * size;
        di += count * size;
        return;
    }
    const reg32 step = cpu.flags.df ? 0u - size : size;
    for (reg32 n = count; n != 0; --n, si += step, di += step)
        if (size == 4)
            rc::st32(cpu.ees + di, rc::ld32(si));
        else
            rc::st8(cpu.ees + di, rc::ld8(si));
}

// Lazy vertex buffer: create, lock, copy the vertices, unlock.
bool createVertexBuffer(win32::WinApplication* app, x86::CPU& cpu, State& s)
{
    const reg32 mesh = s.ebx;
    const reg32 size = rc::ld32(mesh + 0x30);
    rc::st32(mesh + 0x40, 1);
    if (!com(app, cpu, s, rc::ld32(DevicePointer), 26, {size, 8, 0, 1, s.esi, 0}))
        return false;
    s.esi = rc::ld32(s.esi);
    if (!com(app, cpu, s, s.esi, 11, {0, 0, s.frame + 0x28, 0}))
        return false;
    const reg32 bytes = rc::ld32(s.ebx + 0x30);
    s.esi = rc::ld32(s.ebx + 0x28);
    s.edi = rc::ld32(s.frame + 0x28);
    repMove(cpu, s.esi, s.edi, bytes >> 2, 4);
    repMove(cpu, s.esi, s.edi, bytes & 3, 1);
    if (!com(app, cpu, s, rc::ld32(s.ebx + 0x2c), 12, {}))
        return false;
    s.edi = rc::ld32(s.frame + 0x14);
    return true;
}

// Type 0x14 group: ring texture, fade float and its alpha byte.
bool fadeParameters(win32::WinApplication* app, x86::CPU& cpu, State& s)
{
    const reg32 E = s.frame;
    if (!leaf(app, cpu, s, sub_60fe00, E - 4))
        return false;
    reg32 i = cpu.eax;
    i = rc::ld32(Ring + 4 * i) == rc::ld32(rc::ld32(E + 0x1c) + 0x24) ? i - 2 : i - 1;
    i &= 7;
    rc::st32(E - 4, 0);
    rc::st32(E - 8, 0);
    rc::st32(E - 0xc, rc::ld32(Ring + 4 * i));
    if (!leaf(app, cpu, s, sub_4901d0, E - 0x10))
        return false;
    const reg32 info = cpu.eax;
    reg32 current = rc::ld32(CurrentGroup);
    const reg32 effect = rc::ld32(current + 0x20);
    s.esi = rc::ld32(effect);
    br::effectSetTexture(app, cpu, effect, rc::ld32(rc::ld32(current + 0x1c) + 0x894), rc::ld32(rc::ld32(info) + 0x18));

    const reg32 depth = cpu.fpu.count;
    if (!leaf(app, cpu, s, sub_60fde0, E - 4))
        return false;
    rc::stf32(E + 0x1c, float(cpu.fpu.regs[(depth + 1) & 7]));
    cpu.fpu.count = depth;
    current = rc::ld32(CurrentGroup);
    s.esi = rc::ld32(E + 0x1c);
    br::effectSetFloat(app, cpu, rc::ld32(current + 0x20), rc::ld32(rc::ld32(current + 0x1c) + 0x844), rc::ldf32(E + 0x1c));

    const Real product = Real(rc::ldf32(E + 0x1c)) * Real(rc::ldf32(0x784544));
    s.esi = rc::ld32(rc::ld32(CurrentGroup) + 0x20);
    s.edi = rc::ld32(s.esi);
    cpu.fpu.regs[(depth + 1) & 7] = product;
    const reg32 alpha = reg32(x86::truncInteger64(product)) << 24;
    br::effectSetInt(app, cpu, s.esi, rc::ld32(rc::ld32(rc::ld32(CurrentGroup) + 0x1c) + 0x86c), sreg32(alpha));
    s.edi = rc::ld32(E + 0x14);
    return true;
}

// Indexed entry (edi strip, ebp entry): skinning, buffers, a DrawIndexedPrimitive per pass.
bool drawIndexed(win32::WinApplication* app, x86::CPU& cpu, State& s)
{
    rc::st32(s.frame + 0x14, s.edi);
    const reg32 skin = rc::ld32(s.ebp + 0x28);
    if (skin != 0 && (rc::ld8(s.ebp + 0x14) & 1))
    {
        rc::st32(s.frame - 4, s.edi);
        rc::st32(s.frame - 8, skin);
        if (!call(app, cpu, s, sub_5bdcd0, s.frame - 0xc, 8))
            return false;
    }
    s.ebx = rc::ld32(rc::ld32(s.ebp + 0x10));
    s.esi = s.ebx + 0x2c;
    if (rc::ld32(s.ebx + 0x2c) == 0 && !createVertexBuffer(app, cpu, s))
        return false;
    const reg32 mesh = s.ebx;
    if (rc::ld32(mesh + 0x2c) != rc::ld32(LastStream))
    {
        const reg32 buffer = rc::ld32(mesh + 0x2c);
        s.esi = rc::ld32(rc::ld32(CurrentGroup) + 8);
        br::setStreamSource(app, cpu, rc::ld32(DevicePointer), 0, buffer, 0, s.esi);
        rc::st32(LastStream, rc::ld32(mesh + 0x2c));
        increment(0x872ee8);
    }
    if (rc::ld32(mesh + 0x20) != rc::ld32(LastIndices))
    {
        br::setIndices(app, cpu, rc::ld32(DevicePointer), rc::ld32(mesh + 0x20));
        rc::st32(LastIndices, rc::ld32(mesh + 0x20));
        increment(0x872eec);
    }
    if (rc::ld32(s.edi + 0xc) == 0 || rc::ld32(mesh + 0x34) >= 0xffffu)
        return true;
    rc::st32(s.frame + 0x18, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x14));
    const reg32 current = rc::ld32(CurrentGroup);
    const reg32 type = rc::ld32(current + 4);
    if (type == 0x14)
    {
        if (!fadeParameters(app, cpu, s))
            return false;
    }
    else if ((rc::ld8(0x8707cd) == 0 || rc::ld32(rc::ld32(CurrentView)) != 0) && rc::ld8(current + 0x18) != 0 &&
             (type == 0 || type == 1 || type == 5 || type == 4))
        rc::st32(s.frame + 0x18, 1);
    if (!commitChanges(app, cpu, s, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x20)))
        return false;
    s.esi = 0;
    if (sreg32(rc::ld32(s.frame + 0x18)) > 0)
    {
        do
        {
            br::effectPass(app, cpu, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x20), s.esi);
            const reg32 device = rc::ld32(DevicePointer);
            rc::st32(s.frame + 0x1c, rc::ld32(device));
            const reg32 count = third(rc::ld32(s.edi + 0xc));
            s.edi = rc::ld32(s.frame + 0x14);
            const reg32 start = reg32(sreg32(rc::ld32(s.edi + 0x34) - rc::ld32(s.ebx + 0x18)) >> 1);
            br::drawIndexedPrimitive(app, cpu, device, 4, 0, 0, rc::ld32(s.ebx + 0x34), start, count);
            if (!endPass(app, cpu, s, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x20)))
                return false;
            ++s.esi;
        } while (sreg32(s.esi) < sreg32(rc::ld32(s.frame + 0x18)));
    }
    rc::st32(s.ebx + 0x40, 1);
    increment(0x872ee0);
    rc::st32(0x872ef0, rc::ld32(0x872ef0) + third(rc::ld32(s.ebx + 0x34)));
    return true;
}

// Immediate entry (edi strip): a DrawPrimitiveUP strip per pass, then the strip back to its pool.
bool drawImmediate(win32::WinApplication* app, x86::CPU& cpu, State& s)
{
    br::setRenderState(app, cpu, rc::ld32(DevicePointer), 14, 0);
    rc::st32(LastStream, 0);
    const reg32 entryGroup = rc::ld32(s.ebp + 0x18);
    s.ebx = rc::ld32(entryGroup + 0x14);
    if (!commitChanges(app, cpu, s, rc::ld32(entryGroup + 0x20)))
        return false;
    s.esi = 0;
    if (sreg32(s.ebx) > 0)
    {
        do
        {
            br::effectPass(app, cpu, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x20), s.esi);
            const reg32 vertices = s.edi + 4;
            br::drawPrimitiveUP(app, cpu, rc::ld32(DevicePointer), 5, rc::ld32(s.edi) - 2,
                                vertices ? rc::host(vertices) : nullptr, 0x18);
            if (!endPass(app, cpu, s, rc::ld32(rc::ld32(s.ebp + 0x18) + 0x20)))
                return false;
            ++s.esi;
        } while (sreg32(s.esi) < sreg32(s.ebx));
    }
    br::setRenderState(app, cpu, rc::ld32(DevicePointer), 14, 1);
    increment(0x872ee4);
    rc::st32(s.frame - 4, s.edi);
    cpu.ecx = rc::ld32(StripPool);
    return leaf(app, cpu, s, sub_43cdf0, s.frame - 8);
}

// One entry: unlinked, its setup children, its draw, back to the entry pool.
bool drawEntry(win32::WinApplication* app, x86::CPU& cpu, State& s, reg32 list)
{
    s.ebp = rc::ld32(list);
    const reg32 next = rc::ld32(s.ebp), previous = rc::ld32(s.ebp + 4);
    rc::st32(previous, next);
    rc::st32(next + 4, previous);
    rc::st32(s.frame + 0x1c, rc::ld32(s.esi + 0x10));
    rc::st32(s.frame - 4, rc::ld32(rc::ld32(CurrentView) + 4));
    rc::st32(s.frame - 8, rc::ld32(s.ebp + 0x24));
    if (!call(app, cpu, s, sub_5bdde0, s.frame - 0xc, 8))
        return false;
    if (rc::ld32(rc::ld32(rc::ld32(s.ebp + 0x18) + 0x1c) + 0x59c) != 0)
    {
        rc::st32(s.frame - 4, 9);
        if (!leaf(app, cpu, s, sub_48b1e0, s.frame - 8))
            return false;
        rc::st32(s.frame - 8, cpu.eax);
        rc::st32(s.frame - 0xc, rc::ld32(s.ebp + 0x24));
        if (!call(app, cpu, s, sub_5bdaa0, s.frame - 0x10, 0xc))
            return false;
    }
    if (const reg32 light = rc::ld32(s.ebp + 0x1c))
    {
        rc::st32(s.frame - 4, rc::ld32(s.ebp + 0x24));
        rc::st32(s.frame - 8, light);
        if (!call(app, cpu, s, sub_5bd140, s.frame - 0xc, 8))
            return false;
    }
    if (const reg32 material = rc::ld32(s.ebp + 0x20))
    {
        rc::st32(s.frame - 4, rc::ld32(s.ebp + 0x14));
        rc::st32(s.frame - 8, material);
        if (!call(app, cpu, s, sub_5bd4a0, s.frame - 0xc, 8))
            return false;
    }
    const reg32 immediate = rc::ld32(s.ebp + 0xc);
    s.edi = rc::ld32(s.ebp + 8);
    if (!(immediate != 0 ? drawImmediate(app, cpu, s) : drawIndexed(app, cpu, s)))
        return false;
    rc::st32(s.frame - 4, s.ebp);
    rc::st32(s.frame - 8, rc::ld32(EntryPool));
    return leaf(app, cpu, s, sub_43f920, s.frame - 0xc);
}

// A non-empty list (ebx head, ebp group slot): group begin, buckets and entries, group end.
bool drawList(win32::WinApplication* app, x86::CPU& cpu, State& s)
{
    s.edi = rc::ld32(s.ebp);
    rc::st32(CurrentGroup, s.edi);
    rc::st32(s.frame + 0x2c, s.edi);
    if (!group(app, cpu, s, s.edi, 4))
        return false;
    br::setVertexDeclaration(app, cpu, rc::ld32(DevicePointer), rc::ld32(s.edi + 0x24));
    br::effectBegin(app, cpu, rc::ld32(s.edi + 0x20), reinterpret_cast<win32::UINT*>(rc::host(s.frame + 0x30)), 0);
    s.esi = rc::ld32(s.ebx);
    increment(0x872f04);
    rc::st32(s.frame + 0x10, s.esi);
    if (s.esi != s.ebx)
    {
        for (;;)
        {
            const reg32 bucket = s.esi;
            const reg32 next = rc::ld32(bucket), previous = rc::ld32(bucket + 4);
            rc::st32(previous, next);
            rc::st32(next + 4, previous);
            increment(0x872ed8);
            rc::st32(s.frame - 4, rc::ld32(bucket + 0x10));
            if (!call(app, cpu, s, sub_5bcc30, s.frame - 8, 4))
                return false;
            reg32 list = s.esi + 0x18;
            if (rc::ld32(list) != list)
            {
                do
                {
                    if (!drawEntry(app, cpu, s, list))
                        return false;
                    s.esi = rc::ld32(s.frame + 0x10);
                    list = s.esi + 0x18;
                } while (rc::ld32(list) != list);
                s.edi = rc::ld32(s.frame + 0x2c);
                s.ebx = rc::ld32(s.frame + 0x20);
            }
            rc::st32(rc::ld32(rc::ld32(s.esi + 0x10)) + 0x1c, 0);
            cpu.ecx = s.esi;
            if (!leaf(app, cpu, s, sub_5cea20, s.frame - 4))
                return false;
            rc::st32(s.frame - 4, s.esi);
            rc::st32(s.frame - 8, rc::ld32(BucketPool));
            if (!leaf(app, cpu, s, sub_43f920, s.frame - 0xc))
                return false;
            const reg32 first = rc::ld32(s.ebx);
            rc::st32(s.frame + 0x10, first);
            if (first == s.ebx)
                break;
            s.esi = rc::ld32(s.frame + 0x10);
        }
        s.ebp = rc::ld32(s.frame + 0x24);
    }
    if (!group(app, cpu, s, s.edi, 8))
        return false;
    s.edi = rc::ld32(s.edi + 0x20);
    br::effectEnd(app, cpu, s.edi);
    rc::st32(CurrentGroup, 0);
    return true;
}

const reg32 Heads = 0x870d30;
const reg32 Slots = 0x86eb28;
const reg32 Row = 23;
const reg32 HeadCount = 16 * Row;

inline bool listEmpty(reg32 k) { return rc::ld32(Heads + 8 * k) == Heads + 8 * k; }

// First non-empty list head at or after k, HeadCount when none.
inline reg32 nextList(reg32 k)
{
#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
    while (k < HeadCount)
    {
        const reg32 start = k < HeadCount - 16 ? k : HeadCount - 16;
        const reg32 base = Heads + 8 * start;
        const uint32_t* p = reinterpret_cast<const uint32_t*>(rc::host(base));
        const uint32x4_t lanes = {0, 8, 16, 24};
        const uint32x4_t a = vaddq_u32(vdupq_n_u32(base), lanes);
        const uint32x4_t d0 = veorq_u32(vld2q_u32(p).val[0], a);
        const uint32x4_t d1 = veorq_u32(vld2q_u32(p + 8).val[0], vaddq_u32(a, vdupq_n_u32(32)));
        const uint32x4_t d2 = veorq_u32(vld2q_u32(p + 16).val[0], vaddq_u32(a, vdupq_n_u32(64)));
        const uint32x4_t d3 = veorq_u32(vld2q_u32(p + 24).val[0], vaddq_u32(a, vdupq_n_u32(96)));
        const uint32x4_t d = vorrq_u32(vorrq_u32(d0, d1), vorrq_u32(d2, d3));
        if (vget_lane_u64(vreinterpret_u64_u32(vorr_u32(vget_low_u32(d), vget_high_u32(d))), 0) != 0)
            for (; k < start + 16; ++k)
                if (!listEmpty(k))
                    return k;
        k = start + 16;
    }
    return HeadCount;
#else
    for (; k < HeadCount; ++k)
        if (!listEmpty(k))
            return k;
    return HeadCount;
#endif
}

// The guest's 16 x 23 loop step after s.ebx, s.ebp; false at its end.
inline bool guestStep(State& s)
{
    s.ebp += 4;
    s.ebx += 8;
    rc::st32(s.frame + 0x20, s.ebx);
    rc::st32(s.frame + 0x24, s.ebp);
    if (sreg32(s.ebp) < sreg32(0x86eb84))
        return true;
    if (sreg32(s.ebx) >= sreg32(0x8718b0))
        return false;
    s.ebp = Slots;
    rc::st32(s.frame + 0x24, s.ebp);
    rc::st32(s.frame + 0x20, s.ebx);
    return true;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("flush_native", true); }))
    {
        sub_5cf420_guest(app, cpu);
        return;
    }
    s_direct = pf::cachedStatic([] { return pf::Config::getBool("flush_direct", true); });
    State s = {cpu.ebx, cpu.ebp, cpu.esi, cpu.edi, cpu.esp - 0x34};
    rc::st32(s.frame + 0xc, s.ebx);
    rc::st32(s.frame + 8, s.ebp);
    rc::st32(s.frame + 4, s.esi);
    rc::st32(s.frame, s.edi);
    increment(0x872edc);
    reg32 k = 0;
    bool guest = false;
    for (;;)
    {
        if (!guest)
        {
            k = nextList(k);
            if (k == HeadCount)
                break;
            s.ebx = Heads + 8 * k;
            s.ebp = Slots + 4 * (k % Row);
            rc::st32(s.frame + 0x20, s.ebx);
            rc::st32(s.frame + 0x24, s.ebp);
        }
        else if (!guestStep(s))
            break;
        else if (rc::ld32(s.ebx) == s.ebx)
            continue;
        const reg32 ebx = s.ebx, ebp = s.ebp;
        if (!drawList(app, cpu, s))
            return;
        if (!guest)
        {
            guest = s.ebx != ebx || s.ebp != ebp;
            ++k;
        }
    }
    cpu.edi = rc::ld32(s.frame);
    cpu.esi = rc::ld32(s.frame + 4);
    cpu.ebp = rc::ld32(s.frame + 8);
    rc::st32(0x87094c, 0);
    cpu.ebx = rc::ld32(s.frame + 0xc);
    cpu.esp = s.frame + 0x38;
}

}

// Not routed through shadow::call: the flush unlinks and frees, so a second run cannot be compared.
void sub_5cf420(win32::WinApplication* app, x86::CPU& cpu)
{
    scenery::timed(scenery::PhaseFlush, [&] { body(app, cpu); });
}

}}
