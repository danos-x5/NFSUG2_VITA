// Native sub_5d69b0: scenery box-tree walk with an explicit stack (thiscall: section, view). Its stack stays in the guest frame.

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>
#include <scenery.h>

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
#include <arm_neon.h>
#endif

namespace ug2
{

void sub_5d69b0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5d6570(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

void sub_5bc300(win32::WinApplication* app, x86::CPU& cpu);

namespace
{

inline void put(x86::reg8* p, x86::reg32 value) { memcpy(p, &value, 4); }
inline x86::reg32 get(const x86::reg8* p)
{
    x86::reg32 value;
    memcpy(&value, p, 4);
    return value;
}

// Copies the node's two corners to frame+0x20 and frame+0x30.
void stageBox(x86::reg8* f, x86::reg32 node)
{
#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
    // node may be unaligned, frame is 16-aligned; frame+0x2c gets word 3, a slot the guest never uses.
    const x86::reg8* const n = rc::host(node);
    const uint8x16_t lo = vld1q_u8(n);
    const uint8x8_t hi = vld1_u8(n + 0x10);
    x86::reg8* out = f + 0x20;
    vst1q_u8(out, lo);
    out += 0x10;
    vst1q_lane_u32(reinterpret_cast<uint32_t*>(out), vreinterpretq_u32_u8(lo), 3);
    out += 0x4;
    vst1_u8(out, hi);
#else
    put(f + 0x20, rc::ld32(node));
    put(f + 0x24, rc::ld32(node + 0x4));
    put(f + 0x28, rc::ld32(node + 0x8));
    put(f + 0x30, rc::ld32(node + 0xc));
    put(f + 0x34, rc::ld32(node + 0x10));
    put(f + 0x38, rc::ld32(node + 0x14));
#endif
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("tree_cull_native"))
    {
        sub_5d69b0_guest(app, cpu);
        return;
    }
    const x86::reg32 section = cpu.ecx;
    const x86::reg32 entry = cpu.esp;
    rc::st32(entry - 0x4, cpu.ebp);
    cpu.ebp = entry - 0x4;
    // Guest frame base: overruns land where the guest's do.
    const x86::reg32 frame = ((entry - 0x4) & ~x86::reg32(15)) - 0x180;
    x86::reg8* const base = rc::host(frame);
    put(base + 0x80, rc::ld32(section + 0x28));
    base[0x40] = 1;
    put(base + 0x8, cpu.ebx);
    put(base + 0x4, cpu.esi);
    put(base, cpu.edi);
    // Callees keep ebx, esi, edi and return with esp at frame (ret 0xc), as the guest relies on.
    int depth = 1;
    for (;;)
    {
        x86::reg8* f = base;
        // Keeps the frame offsets in addressing modes instead of one hoisted pointer each.
        asm("" : "+r"(f));
        x86::reg8* const states = f + 0x40;
        x86::reg8* const nodes = f + 0x80;
        --depth;
        x86::reg8 state = states[depth];
        const x86::reg32 node = get(nodes + 4 * depth);
        if (state == 1)
        {
            stageBox(f, node);
            put(f - 0x4, 0);
            put(f - 0x8, frame + 0x30);
            put(f - 0xc, frame + 0x20);
            cpu.ecx = rc::ld32(rc::ld32(entry + 0x4) + 0x40);
            cpu.esp = frame - 0x10;
            sub_5bc300(app, cpu);
            state = x86::reg8(cpu.eax);
        }
        int16_t count;
        if (state != 0 && (count = int16_t(rc::ld16(node + 0x18))) > 0)
        {
            cpu.ebx = (cpu.ebx & 0xffffff00u) | state;
            cpu.esi = node;
            cpu.edi = frame + 0x80 + 4 * depth;
            int i = 0;
            for (;;)
            {
                const int16_t child = int16_t(rc::ld16(node + 0x1a + 2 * i));
                if (child >= 0)
                {
                    put(f - 0x4, state);
                    put(f - 0x8, rc::ld32(entry + 0x4));
                    put(f - 0xc, x86::reg32(int32_t(child)));
                    cpu.ecx = section;
                    cpu.esp = frame - 0x10;
                    sub_5d6570(app, cpu);
                }
                else
                {
                    put(nodes + 4 * depth, rc::ld32(section + 0x28) - x86::reg32(int32_t(child) * 36));
                    states[depth] = state;
                    ++depth;
                    cpu.edi = frame + 0x80 + 4 * depth;
                }
                ++i;
                count = int16_t(rc::ld16(node + 0x18));
                if (i >= count)
                    break;
                app->allowPreemption(cpu);
            }
        }
        if (depth == 0)
            break;
        app->allowPreemption(cpu);
    }
    cpu.ebx = get(base + 0x8);
    cpu.esi = get(base + 0x4);
    cpu.edi = get(base);
    cpu.ecx = frame + 0x80;
    cpu.edx = frame + 0x40;
    cpu.ebp = rc::ld32(entry - 0x4);
    cpu.esp = entry + 0x8;
}

// ecx, edx and eax not compared: the caller reloads them.
const shadow::Spec spec = {"sub_5d69b0", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi | shadow::Esp, 0, false, true};

void declare(x86::CPU&, shadow::Spans&) {}

}

void sub_5d69b0(win32::WinApplication* app, x86::CPU& cpu)
{
    scenery::timed(scenery::PhaseCull, [&] { shadow::call(spec, app, cpu, body, sub_5d69b0_guest, declare); });
}

}}
