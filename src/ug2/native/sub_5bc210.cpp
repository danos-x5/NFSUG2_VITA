// Native replacement of sub_5bc210: transforms the box [pmin, pmax] by a matrix (cdecl: M, pmin, pmax).

#include <native/box_frustum.h>
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5bc210_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)

// Unaligned-safe byte accesses; the asm stops narrowing to vldr.
inline float32x4_t load4(const uint8_t* p)
{
    uint8x16_t v = vld1q_u8(p);
    __asm__("" : "+w"(v));
    return vreinterpretq_f32_u8(v);
}

inline float32x2_t load2(const uint8_t* p)
{
    uint8x8_t v = vld1_u8(p);
    __asm__("" : "+w"(v));
    return vreinterpret_f32_u8(v);
}

// Hides lanes from SLP, which reorders operands (NaN payloads).
inline float scalar(float v)
{
    __asm__("" : "+w"(v));
    return v;
}

inline void store2(uint8_t* p, float32x2_t v)
{
    uint8x8_t u = vreinterpret_u8_f32(v);
    __asm__("" : "+w"(u));
    vst1_u8(p, u);
}

#endif

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("box_cull_native"))
    {
        sub_5bc210_guest(app, cpu);
        return;
    }
    const x86::reg32 esp = cpu.esp;
    const x86::reg32 matrix = rc::ld32(esp + 4);
    const x86::reg32 pmin = rc::ld32(esp + 8);
    const x86::reg32 pmax = rc::ld32(esp + 12);
#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
    uint8_t* const base = win32::g_guestBase;
    const float32x4_t r0 = load4(base + matrix), r1 = load4(base + (matrix + 0x10)), r2 = load4(base + (matrix + 0x20));
    const float32x2_t r3 = load2(base + (matrix + 0x30)), r3z = load2(base + (matrix + 0x34));
    const float32x2_t a = load2(base + pmin), az = load2(base + (pmin + 4));
    const float32x2_t b = load2(base + pmax), bz = load2(base + (pmax + 4));
    const float m[16] = {scalar(vgetq_lane_f32(r0, 0)), scalar(vgetq_lane_f32(r0, 1)), scalar(vgetq_lane_f32(r0, 2)), 0.0f,
                         scalar(vgetq_lane_f32(r1, 0)), scalar(vgetq_lane_f32(r1, 1)), scalar(vgetq_lane_f32(r1, 2)), 0.0f,
                         scalar(vgetq_lane_f32(r2, 0)), scalar(vgetq_lane_f32(r2, 1)), scalar(vgetq_lane_f32(r2, 2)), 0.0f,
                         scalar(vget_lane_f32(r3, 0)), scalar(vget_lane_f32(r3, 1)), scalar(vget_lane_f32(r3z, 1)), 0.0f};
    float lo[3] = {scalar(vget_lane_f32(a, 0)), scalar(vget_lane_f32(a, 1)), scalar(vget_lane_f32(az, 1))};
    float hi[3] = {scalar(vget_lane_f32(b, 0)), scalar(vget_lane_f32(b, 1)), scalar(vget_lane_f32(bz, 1))};
    boxcull::transform(m, lo, hi);
    store2(base + pmin, vset_lane_f32(lo[1], vdup_n_f32(lo[0]), 1));
    rc::stf32(pmin + 8, lo[2]);
    store2(base + pmax, vset_lane_f32(hi[1], vdup_n_f32(hi[0]), 1));
    rc::stf32(pmax + 8, hi[2]);
#else
    float m[16] = {};
    for (int k = 0; k < 3; ++k)
        for (int j = 0; j < 3; ++j)
            m[4 * k + j] = rc::ldf32(matrix + 16 * k + 4 * j);
    for (int j = 0; j < 3; ++j)
        m[12 + j] = rc::ldf32(matrix + 0x30 + 4 * j);
    float lo[3], hi[3];
    for (int j = 0; j < 3; ++j)
    {
        lo[j] = rc::ldf32(pmin + 4 * j);
        hi[j] = rc::ldf32(pmax + 4 * j);
    }
    boxcull::transform(m, lo, hi);
    for (int j = 0; j < 3; ++j)
    {
        rc::stf32(pmin + 4 * j, lo[j]);
        rc::stf32(pmax + 4 * j, hi[j]);
    }
#endif
    cpu.esp = esp + 4;
}

void declare(x86::CPU& entry, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{rc::ld32(entry.esp + 8), 12, false});
    spans.push_back(shadow::Span{rc::ld32(entry.esp + 12), 12, false});
}

// Preserved registers only: eax, ecx, edx are dead after the only call site (sub_5bc300 reloads them).
const shadow::Spec spec = {"sub_5bc210", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5bc210(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bc210_guest, declare);
}

}}
