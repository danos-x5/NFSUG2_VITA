#ifndef UG2_NATIVE_BOX_FRUSTUM_H_
#define UG2_NATIVE_BOX_FRUSTUM_H_

// Box transform (sub_5bc210) and box-against-planes test (sub_5bc300) in the guest's x87 order (x86::Float), bit-exact.

#include <cmath>
#include <fpu.h>

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)
#include <arm_neon.h>
#endif

namespace ug2 { namespace native { namespace boxcull {

using Real = x86::Float;

struct Plane
{
    float n[3];
    float d;
};

// Transforms box [pmin, pmax] in place by M.
inline void transform(const float* m, float* pmin, float* pmax)
{
    Real lo[3], hi[3];
    for (int j = 0; j < 3; ++j)
        lo[j] = hi[j] = m[12 + j];
    for (int k = 0; k < 3; ++k)
        for (int j = 0; j < 3; ++j)
        {
            const float term = m[4 * k + j];
            const Real x = Real(pmin[k]) * term;
            const float y = float(Real(pmax[k]) * term);
            if (x < Real(y))
            {
                lo[j] += x;
                hi[j] += Real(y);
            }
            else
            {
                lo[j] += Real(y);
                hi[j] += x;
            }
        }
    for (int j = 0; j < 3; ++j)
    {
        pmin[j] = float(lo[j]);
        pmax[j] = float(hi[j]);
    }
}

// Result 0, 1 or 2; rejected receives the rejecting plane.
template <class Load>
inline int test(const float* a, const float* b, Real half, Real zero, Load load, int& rejected)
{
    float c[3], e[3];
    for (int i = 0; i < 3; ++i)
        c[i] = float((Real(b[i]) + Real(a[i])) * half);
    for (int i = 0; i < 3; ++i)
        e[i] = float(Real(b[i]) - Real(c[i]));
    bool straddles = false;
    for (int k = 0; k < 6; ++k)
    {
        const Plane p = load(k);
        const Real r = (Real(std::fabs(p.n[2])) * e[2] + Real(std::fabs(p.n[0])) * e[0])
                       + Real(std::fabs(p.n[1])) * e[1];
        const Real s = ((Real(c[2]) * p.n[2] + Real(c[0]) * p.n[0]) + Real(c[1]) * p.n[1]) + Real(p.d);
        if (s + r < zero)
        {
            rejected = k;
            return 0;
        }
        if (s - r < zero)
            straddles = true;
    }
    return straddles ? 1 : 2;
}

#if defined(__ARM_NEON) && !defined(NFSU_FPU_DOUBLE)

static_assert(sizeof(Real) == sizeof(float), "the NEON box test needs the float baseline");

// Same result as test(): six planes (n.xyz, d) at once, one add and multiply per step, no fused multiply-add.
inline int testNeon(const float* a, const float* b, float half, float zero, const float* planes, int& rejected)
{
    float c[3], e[3];
    for (int i = 0; i < 3; ++i)
        c[i] = (b[i] + a[i]) * half;
    for (int i = 0; i < 3; ++i)
        e[i] = b[i] - c[i];
    const float32x4_t c0 = vdupq_n_f32(c[0]), c1 = vdupq_n_f32(c[1]), c2 = vdupq_n_f32(c[2]);
    const float32x4_t e0 = vdupq_n_f32(e[0]), e1 = vdupq_n_f32(e[1]), e2 = vdupq_n_f32(e[2]);
    const float32x4_t limit = vdupq_n_f32(zero);
    const float32x4x4_t first = vld4q_f32(planes);
    const float32x2x4_t rest = vld4_f32(planes + 16);
    const uint32x4_t firstBits = {1u, 2u, 4u, 8u}, restBits = {16u, 32u, 0u, 0u};
    uint32_t reject = 0, straddle = 0;
    for (int g = 0; g < 2; ++g)
    {
        float32x4_t n0, n1, n2, d;
        if (g == 0)
        {
            n0 = first.val[0]; n1 = first.val[1]; n2 = first.val[2]; d = first.val[3];
        }
        else
        {
            n0 = vcombine_f32(rest.val[0], rest.val[0]); n1 = vcombine_f32(rest.val[1], rest.val[1]);
            n2 = vcombine_f32(rest.val[2], rest.val[2]); d = vcombine_f32(rest.val[3], rest.val[3]);
        }
        const float32x4_t r = vaddq_f32(vaddq_f32(vmulq_f32(vabsq_f32(n2), e2), vmulq_f32(vabsq_f32(n0), e0)),
                                        vmulq_f32(vabsq_f32(n1), e1));
        const float32x4_t s = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(c2, n2), vmulq_f32(c0, n0)), vmulq_f32(c1, n1)), d);
        const uint32x4_t bitsOf = g == 0 ? firstBits : restBits;
        const uint32x4_t out = vandq_u32(vcltq_f32(vaddq_f32(s, r), limit), bitsOf);
        const uint32x4_t in = vandq_u32(vcltq_f32(vsubq_f32(s, r), limit), bitsOf);
        const uint32x2_t o = vadd_u32(vget_low_u32(out), vget_high_u32(out));
        const uint32x2_t i = vadd_u32(vget_low_u32(in), vget_high_u32(in));
        reject |= vget_lane_u32(vpadd_u32(o, o), 0);
        straddle |= vget_lane_u32(vpadd_u32(i, i), 0);
    }
    if (reject)
    {
        rejected = __builtin_ctz(reject);
        return 0;
    }
    return straddle ? 1 : 2;
}

#endif

}}}

#endif /* !UG2_NATIVE_BOX_FRUSTUM_H_ */
