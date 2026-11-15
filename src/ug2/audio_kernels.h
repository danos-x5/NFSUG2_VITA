#ifndef UG2_AUDIO_KERNELS_H_
#define UG2_AUDIO_KERNELS_H_

// Fast EA SND mixer kernels (audio_neon_kernels) on guest memory at m: NEON on ARMv7, the same float maths elsewhere.
// Convert and output stay bit-identical to x87.

#include <cstdint>
#include <cstring>

#if defined(__ARM_NEON) && (defined(__arm__) || defined(UG2_AUDIO_TEST_NEON))
#include <arm_neon.h>
#define UG2_AUDIO_NEON 1
#endif

namespace ug2
{

namespace kernels
{

inline float loadFloat(const uint8_t* p)
{
    float value;
    memcpy(&value, p, sizeof(value));
    return value;
}

inline void storeFloat(uint8_t* p, float value)
{
    memcpy(p, &value, sizeof(value));
}

inline float loadAligned(const uint8_t* p)
{
    float value;
    memcpy(&value, __builtin_assume_aligned(p, 4), sizeof(value));
    return value;
}

inline void storeAligned(uint8_t* p, float value)
{
    memcpy(__builtin_assume_aligned(p, 4), &value, sizeof(value));
}

// dst += src * gain in the guest's descending blocks.
inline void mix(uint8_t* m, uint32_t n, float gain, uint32_t src, uint32_t dst)
{
    while (n & 3)
    {
        float value = loadFloat(m + (src + n * 4 - 4)) * gain;
        value += loadFloat(m + (dst + n * 4 - 4));
        storeFloat(m + (dst + n * 4 - 4), value);
        if (n == 1)
            return;
        --n;
    }
    for (;;)
    {
        const uint32_t s = src + n * 4 - 16;
        const uint32_t d = dst + n * 4 - 16;
#ifdef UG2_AUDIO_NEON
        float* const to = reinterpret_cast<float*>(m + d);
        const float32x4_t product = vmulq_n_f32(vld1q_f32(reinterpret_cast<const float*>(m + s)), gain);
        vst1q_f32(to, vaddq_f32(product, vld1q_f32(to)));
#else
        float v[4];
        for (int i = 0; i < 4; ++i)
            v[i] = loadFloat(m + s + 4 * i) * gain;
        for (int i = 0; i < 4; ++i)
            v[i] += loadFloat(m + d + 4 * i);
        for (int i = 0; i < 4; ++i)
            storeFloat(m + d + 4 * i, v[i]);
#endif
        const uint32_t previous = n;
        n -= 4;
        if (!(int32_t(previous) > 4))
            return;
    }
}

// 16-bit to float, descending; exact, as on x87.
inline void convert(uint8_t* m, uint32_t n, uint32_t src, uint32_t dst)
{
    while (n & 7)
    {
        int16_t value;
        memcpy(&value, m + (src + n * 2 - 2), sizeof(value));
        storeFloat(m + (dst + n * 4 - 4), float(value));
        if (n == 1)
            return;
        --n;
    }
    for (;;)
    {
        const uint32_t s = src + n * 2 - 16;
        const uint32_t d = dst + n * 4 - 32;
#ifdef UG2_AUDIO_NEON
        const int16x8_t value = vld1q_s16(reinterpret_cast<const int16_t*>(m + s));
        const float32x4_t low = vcvtq_f32_s32(vmovl_s16(vget_low_s16(value)));
        const float32x4_t high = vcvtq_f32_s32(vmovl_s16(vget_high_s16(value)));
        vst1q_f32(reinterpret_cast<float*>(m + d), low);
        vst1q_f32(reinterpret_cast<float*>(m + d + 16), high);
#else
        int16_t value[8];
        memcpy(value, m + s, sizeof(value));
        for (int i = 0; i < 8; ++i)
            storeFloat(m + d + 4 * i, float(value[i]));
#endif
        const uint32_t previous = n;
        n -= 8;
        if (!(int32_t(previous) > 8))
            return;
    }
}

inline bool overlaps(uint64_t a, uint64_t aEnd, uint64_t b, uint64_t bEnd)
{
    return a < bEnd && b < aEnd;
}

// Disjoint guest ranges that do not wrap.
inline bool apart(uint32_t a, uint32_t aLength, uint32_t b, uint32_t bLength)
{
    return a - b >= bLength && b - a >= aLength;
}

// Linear resampler, 32.32 positions; false when guest order matters.
inline bool resample(uint8_t* m, uint32_t n, uint32_t src, uint32_t dst, uint32_t& position, uint32_t& fraction,
                     uint32_t step, uint32_t stepFraction, float scale, uint32_t scratch)
{
    const uint32_t count = int32_t(n) >= 2 ? n : (n & 1);
    if (!count)
        return true;
    if (step >= 0x1000 || count >= 0x100000)
        return false;
    const uint64_t increment = (uint64_t(step) << 32) | stepFraction;
    uint64_t accumulator = (uint64_t(position) << 32) | fraction;
    uint64_t last;
    if (__builtin_add_overflow(accumulator, increment * (count - 1), &last))
        return false;
    const uint64_t readBegin = src + uint64_t(position) * 4;
    const uint64_t readEnd = src + (last >> 32) * 4 + 8;
    const uint64_t writeEnd = dst + uint64_t(count) * 4;
    if (readEnd > 0x100000000ull || writeEnd > 0x100000000ull || overlaps(readBegin, readEnd, dst, writeEnd) ||
        overlaps(readBegin, readEnd, scratch, scratch + 8) || overlaps(dst, writeEnd, scratch, scratch + 8))
        return false;
    const uint8_t* const in = m + src;
    uint8_t* out = m + dst;
    uint32_t i = 0;
#ifdef UG2_AUDIO_NEON
    const uint32_t lanes[4] = { fraction, fraction + stepFraction, fraction + 2 * stepFraction,
                                fraction + 3 * stepFraction };
    uint32x4_t fractions = vld1q_u32(lanes);
    const uint32x4_t fractionStep = vdupq_n_u32(4 * stepFraction);
    for (; i + 4 <= count; i += 4)
    {
        const float* const p0 = reinterpret_cast<const float*>(in + (accumulator >> 32) * 4);
        accumulator += increment;
        const float* const p1 = reinterpret_cast<const float*>(in + (accumulator >> 32) * 4);
        accumulator += increment;
        const float* const p2 = reinterpret_cast<const float*>(in + (accumulator >> 32) * 4);
        accumulator += increment;
        const float* const p3 = reinterpret_cast<const float*>(in + (accumulator >> 32) * 4);
        accumulator += increment;
        const float32x4x2_t pairs = vuzpq_f32(vcombine_f32(vld1_f32(p0), vld1_f32(p1)),
                                              vcombine_f32(vld1_f32(p2), vld1_f32(p3)));
        const float32x4_t weight = vmulq_n_f32(vcvtq_f32_u32(fractions), scale);
        const float32x4_t value = vaddq_f32(vmulq_f32(vsubq_f32(pairs.val[1], pairs.val[0]), weight), pairs.val[0]);
        vst1q_f32(reinterpret_cast<float*>(out), value);
        out += 16;
        fractions = vaddq_u32(fractions, fractionStep);
    }
#endif
    for (; i < count; ++i)
    {
        const uint8_t* const p = in + (accumulator >> 32) * 4;
        const float s0 = loadFloat(p);
        const float weight = float(uint32_t(accumulator)) * scale;
        float value = (loadFloat(p + 4) - s0) * weight;
        value += s0;
        storeFloat(out, value);
        out += 4;
        accumulator += increment;
    }
    position = uint32_t(accumulator >> 32);
    fraction = uint32_t(accumulator);
    return true;
}

// One EA-XA frame in float; out and row 4-aligned.
inline void eaxaFrame(const uint8_t* __restrict in, uint8_t* __restrict out, const uint8_t* __restrict row, float a,
                      float b, float& y1, float& y2)
{
    float older = y2;
    float previous = y1;
    for (int i = 0; i < 14; ++i)
    {
        const uint32_t byte = in[1 + i];
        float first = older * a;
        first += loadAligned(row + (byte >> 4) * 4);
        first += previous * b;
        float second = previous * a;
        second += loadAligned(row + (byte & 0xf) * 4);
        second += first * b;
        storeAligned(out + 8 * i, first);
        storeAligned(out + 8 * i + 4, second);
        older = first;
        previous = second;
    }
    y1 = previous;
    y2 = older;
}

// Float to 16-bit, saturation counts; exact. False on overlapping buffers.
inline bool output(uint8_t* m, uint32_t& samples, uint32_t end, uint32_t& out, float bias, uint32_t& high,
                   uint32_t& low)
{
    const uint32_t count = samples < end ? uint32_t((uint64_t(end - samples) + 3) / 4) : 0;
    const uint64_t outEnd = uint64_t(out) + 2ull * count;
    if (outEnd > 0x100000000ull || overlaps(samples, uint64_t(samples) + 4ull * count, out, outEnd))
        return false;
    uint8_t* in = m + samples;
    uint8_t* to = m + out;
    uint32_t i = 0;
#ifdef UG2_AUDIO_NEON
    const float32x4_t offset = vdupq_n_f32(bias);
    const uint32x4_t mask = vdupq_n_u32(0xfffff);
    const uint32x4_t lowest = vdupq_n_u32(0x7fff);
    const uint32x4_t highest = vdupq_n_u32(0xf8000);
    const uint32x4_t middle = vdupq_n_u32(0x80000);
    const uint32x4_t negative = vdupq_n_u32(0x8000);
    uint32x4_t highs = vdupq_n_u32(0);
    uint32x4_t lows = vdupq_n_u32(0);
    for (; i + 4 <= count; i += 4)
    {
        const float32x4_t value = vaddq_f32(vld1q_f32(reinterpret_cast<const float*>(in)), offset);
        vst1q_f32(reinterpret_cast<float*>(in), value);
        const uint32x4_t bits = vandq_u32(vreinterpretq_u32_f32(value), mask);
        const uint32x4_t saturated = vandq_u32(vcgtq_u32(bits, lowest), vcltq_u32(bits, highest));
        const uint32x4_t positive = vcltq_u32(bits, middle);
        highs = vsubq_u32(highs, vandq_u32(saturated, positive));
        lows = vsubq_u32(lows, vbicq_u32(saturated, positive));
        const uint32x4_t clamped = vbslq_u32(saturated, vbslq_u32(positive, lowest, negative), bits);
        vst1_u16(reinterpret_cast<uint16_t*>(to), vmovn_u32(clamped));
        in += 16;
        to += 8;
    }
    const uint32x2_t h = vpadd_u32(vget_low_u32(highs), vget_high_u32(highs));
    const uint32x2_t l = vpadd_u32(vget_low_u32(lows), vget_high_u32(lows));
    high += vget_lane_u32(h, 0) + vget_lane_u32(h, 1);
    low += vget_lane_u32(l, 0) + vget_lane_u32(l, 1);
#endif
    for (; i < count; ++i)
    {
        const float value = loadFloat(in) + bias;
        storeFloat(in, value);
        uint32_t bits;
        memcpy(&bits, &value, sizeof(bits));
        bits &= 0xfffff;
        if (bits > 0x7fff && bits < 0xf8000)
        {
            const bool positive = bits < 0x80000;
            high += positive;
            low += !positive;
            bits = positive ? 0x7fff : 0xffff8000;
        }
        const uint16_t half = uint16_t(bits);
        memcpy(to, &half, sizeof(half));
        in += 4;
        to += 2;
    }
    samples += 4 * count;
    out += 2 * count;
    return true;
}

}

}

#endif /* !UG2_AUDIO_KERNELS_H_ */
