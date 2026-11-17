// Native replacement of sub_5bc300: box A, B against the six frustum planes (thiscall, optional matrix M).

#include <native/box_frustum.h>
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
#define UG2_BOX_NEON 1
#endif

namespace ug2
{

void sub_5bc300_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

enum Mode
{
    Guest,
    Scalar,
    Neon,
};

const x86::reg32 Half = 0x784260;
const x86::reg32 Zero = 0x7d7d3c;
// Outcome code: plane | result << 3; plane is the rejecting plane, or 6 when none rejects.
const int NoPlane = 6;

#ifdef UG2_BOX_NEON
// Out of line, off the NEON hot path.
[[gnu::noinline]]
#endif
int testScalar(x86::reg32 boxA, x86::reg32 boxB, x86::reg32 matrix, x86::reg32 planes)
{
    float a[3], b[3];
    for (int i = 0; i < 3; ++i)
    {
        a[i] = rc::ldf32(boxA + 4 * i);
        b[i] = rc::ldf32(boxB + 4 * i);
    }
    if (matrix != 0)
    {
        float m[16] = {};
        for (int k = 0; k < 3; ++k)
            for (int j = 0; j < 3; ++j)
                m[4 * k + j] = rc::ldf32(matrix + 16 * k + 4 * j);
        for (int j = 0; j < 3; ++j)
            m[12 + j] = rc::ldf32(matrix + 0x30 + 4 * j);
        boxcull::transform(m, a, b);
    }
    int rejected = -1;
    const int result = boxcull::test(a, b, ld(Half), ld(Zero), [&](int k) {
        const x86::reg32 p = planes + 16 * k;
        return boxcull::Plane{{rc::ldf32(p), rc::ldf32(p + 4), rc::ldf32(p + 8)}, rc::ldf32(p + 12)};
    }, rejected);
    return result == 0 ? rejected : NoPlane | result << 3;
}

#ifdef UG2_BOX_NEON

static_assert(sizeof(Real) == sizeof(float), "the NEON box test needs the float baseline");

const float* hostFloats(x86::reg32 address)
{
    return reinterpret_cast<const float*>(rc::host(address));
}

// Byte loads: no alignment assumed for guest pointers.
float32x4_t load4(x86::reg32 address)
{
    return vreinterpretq_f32_u8(vld1q_u8(rc::host(address)));
}

// x, y, z, z.
float32x4_t load3(x86::reg32 address)
{
    return vcombine_f32(vreinterpret_f32_u8(vld1_u8(rc::host(address))), vdup_n_f32(rc::ldf32(address + 8)));
}

// boxcull::transform and test in NEON, same order; lane 3 unused.
int testNeon(x86::reg32 boxA, x86::reg32 boxB, x86::reg32 matrix, x86::reg32 planes)
{
    float32x4_t a = load3(boxA), b = load3(boxB);
    if (matrix != 0)
    {
        float32x4_t lo = load4(matrix + 0x30), hi = lo;
        const auto step = [&](float32x4_t x, float32x4_t y) {
            const uint32x4_t less = vcltq_f32(x, y);
            lo = vaddq_f32(lo, vbslq_f32(less, x, y));
            hi = vaddq_f32(hi, vbslq_f32(less, y, x));
        };
        const float32x4_t m0 = load4(matrix), m1 = load4(matrix + 0x10), m2 = load4(matrix + 0x20);
        step(vmulq_lane_f32(m0, vget_low_f32(a), 0), vmulq_lane_f32(m0, vget_low_f32(b), 0));
        step(vmulq_lane_f32(m1, vget_low_f32(a), 1), vmulq_lane_f32(m1, vget_low_f32(b), 1));
        step(vmulq_lane_f32(m2, vget_high_f32(a), 0), vmulq_lane_f32(m2, vget_high_f32(b), 0));
        a = lo;
        b = hi;
    }
    const float32x4_t c = vmulq_lane_f32(vaddq_f32(b, a), vld1_dup_f32(hostFloats(Half)), 0);
    const float32x4_t e = vsubq_f32(b, c);
    const float32x2_t c01 = vget_low_f32(c), c2 = vget_high_f32(c);
    const float32x2_t e01 = vget_low_f32(e), e2 = vget_high_f32(e);
    const float* p = hostFloats(planes);
    const float32x4x4_t f = vld4q_f32(p);
    const float32x2x4_t g = vld4_f32(p + 16);
    const float32x4_t limit = vld1q_dup_f32(hostFloats(Zero));
    const float32x4_t rf = vaddq_f32(vaddq_f32(vmulq_lane_f32(vabsq_f32(f.val[2]), e2, 0),
                                               vmulq_lane_f32(vabsq_f32(f.val[0]), e01, 0)),
                                     vmulq_lane_f32(vabsq_f32(f.val[1]), e01, 1));
    const float32x4_t sf = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_lane_f32(f.val[2], c2, 0), vmulq_lane_f32(f.val[0], c01, 0)),
                                               vmulq_lane_f32(f.val[1], c01, 1)),
                                     f.val[3]);
    const float32x2_t rg = vadd_f32(vadd_f32(vmul_lane_f32(vabs_f32(g.val[2]), e2, 0),
                                             vmul_lane_f32(vabs_f32(g.val[0]), e01, 0)),
                                    vmul_lane_f32(vabs_f32(g.val[1]), e01, 1));
    const float32x2_t sg = vadd_f32(vadd_f32(vadd_f32(vmul_lane_f32(g.val[2], c2, 0), vmul_lane_f32(g.val[0], c01, 0)),
                                             vmul_lane_f32(g.val[1], c01, 1)),
                                    g.val[3]);
    const uint32x4_t bitsF = {1u, 2u, 4u, 8u};
    const uint32x2_t bitsG = {16u, 32u};
    const uint32x4_t outF = vandq_u32(vcltq_f32(vaddq_f32(sf, rf), limit), bitsF);
    const uint32x4_t inF = vandq_u32(vcltq_f32(vsubq_f32(sf, rf), limit), bitsF);
    const uint32x2_t outG = vand_u32(vclt_f32(vadd_f32(sg, rg), vget_low_f32(limit)), bitsG);
    const uint32x2_t inG = vand_u32(vclt_f32(vsub_f32(sg, rg), vget_low_f32(limit)), bitsG);
    const uint32x2_t out = vorr_u32(vorr_u32(vget_low_u32(outF), vget_high_u32(outF)), outG);
    const uint32x2_t in = vorr_u32(vorr_u32(vget_low_u32(inF), vget_high_u32(inF)), inG);
    const uint32x2_t flags = vpadd_u32(out, in);
    const uint32_t reject = vget_lane_u32(flags, 0);
    const int plane = __builtin_ctz(reject | 1u << NoPlane);
    return reject ? plane : vget_lane_u32(flags, 1) ? plane | 1 << 3 : plane | 2 << 3;
}

#endif

int mode()
{
    if (!NFSU_NATIVE_SETTING("box_cull_native"))
        return Guest;
#ifdef UG2_BOX_NEON
    if (NFSU_NATIVE_SETTING("box_neon"))
        return Neon;
#endif
    return Scalar;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    const int use = pf::cachedStatic([] { return mode(); });
    if (use == Guest)
    {
        sub_5bc300_guest(app, cpu);
        return;
    }
    const x86::reg32 esp = cpu.esp;
    const x86::reg32 boxA = rc::ld32(esp + 4);
    const x86::reg32 boxB = rc::ld32(esp + 8);
    const x86::reg32 matrix = rc::ld32(esp + 12);
    const x86::reg32 self = rc::ld32(cpu.ecx);
    const x86::reg32 planes = self + 0xc0;
#ifdef UG2_BOX_NEON
    const int code = use == Neon ? testNeon(boxA, boxB, matrix, planes) : testScalar(boxA, boxB, matrix, planes);
#else
    const int code = testScalar(boxA, boxB, matrix, planes);
#endif
    const int plane = code & 7;
    cpu.ecx = self + 0xc4 + 16 * plane;
    cpu.edx = x86::reg32(plane + 1);
    cpu.eax = x86::reg32(code >> 3);
    cpu.esp = esp + 16;
}

void declare(x86::CPU&, shadow::Spans&) {}

// eax is read after every call site; ecx and edx are dead there but are set exactly as the guest sets them.
const shadow::Spec spec = {"sub_5bc300", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5bc300(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bc300_guest, declare);
}

}}
