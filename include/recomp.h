#ifndef RECOMP_H_
#define RECOMP_H_

// Support code for the generated game code (tools/recompiler): guest memory loads and stores.

#include <x86.h>
#include <lib/memmap.h>
#include <csetjmp>
#include <cstring>

namespace win32
{
class WinApplication;
}

namespace rc
{

// Host pointer of a guest address.
inline x86::reg8* host(x86::reg32 address)
{
    return win32::g_guestBase + address;
}

// rep movs as one memmove; false when element order would differ.
inline bool repMove(bool df, x86::reg32 dst, x86::reg32 src, x86::reg32 count, x86::reg32 size)
{
    const x86::reg64 bytes = x86::reg64(count) * size;
    if (df || x86::reg64(src) + bytes > 0x100000000ull || x86::reg64(dst) + bytes > 0x100000000ull)
        return false;
    if (dst > src && dst - src < bytes)
        return false;
    memmove(host(dst), host(src), size_t(bytes));
    return true;
}

// rep stos at once; false with df set or a wrap.
inline bool repStore(bool df, x86::reg32 dst, x86::reg32 count, x86::reg32 size, x86::reg32 value)
{
    const x86::reg64 bytes = x86::reg64(count) * size;
    if (df || x86::reg64(dst) + bytes > 0x100000000ull)
        return false;
    x86::reg8* p = host(dst);
    const x86::reg8 b = x86::reg8(value);
    const bool uniform = size == 1 || (size == 2 ? x86::reg16(value) == x86::reg16(b * 0x101u)
                                                 : value == b * 0x01010101u);
    if (uniform)
        memset(p, b, size_t(bytes));
    else if (size == 2)
        for (x86::reg32 i = 0; i < count; ++i)
            memcpy(p + 2 * i, &value, 2);
    else
        for (x86::reg32 i = 0; i < count; ++i)
            memcpy(p + 4 * i, &value, 4);
    return true;
}

// setjmp/longjmp: a setjmp call in generated code saves a host jump buffer, keyed by the guest jmp_buf, and the x87
// depth; longJump resumes there after the guest longjmp restored the registers into cpu (src/lib/jump.cpp).
jmp_buf& jumpBuffer(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 guestBuffer);

[[noreturn]] void longJump(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 guestBuffer);

// div/idiv r/m32: edx:eax by a 32-bit divisor.
struct Division
{
    x86::reg32 quotient;
    x86::reg32 remainder;
};

// Wide dividend with a 32-bit quotient (hi < d): a double estimate within 1 of the quotient, corrected by the
// remainder. vcvt (fcvtzu on AArch64) saturates where the estimate rounds up to 2^32.
inline Division divideWide(x86::reg32 hi, x86::reg32 lo, x86::reg32 d)
{
    const double estimate = (double(hi) * 0x1p32 + double(lo)) / double(d);
#if defined(__arm__)
    float bits;
    asm("vcvt.u32.f64 %0, %P1" : "=t"(bits) : "w"(estimate));
    x86::reg32 q;
    memcpy(&q, &bits, sizeof(q));
#elif defined(__aarch64__)
    x86::reg32 q;
    asm("fcvtzu %w0, %d1" : "=r"(q) : "w"(estimate));
#else
    x86::reg32 q = estimate >= 0x1p32 ? 0xffffffffu : x86::reg32(estimate);
#endif
    const x86::reg64 n = (x86::reg64(hi) << 32) | lo;
    x86::sreg64 r = x86::sreg64(n - x86::reg64(q) * d);
    if (r < 0)
    {
        --q;
        r += d;
    }
    else if (x86::reg64(r) >= d)
    {
        ++q;
        r -= d;
    }
    return {q, x86::reg32(r)};
}

// A zero divisor or a quotient that does not fit (#DE on x86): the 64-bit libgcc division, out of line so the
// compiler does not turn a known zero divisor into a trap.
[[gnu::noinline, gnu::cold]] inline Division divideUnsignedWide(x86::reg32 hi, x86::reg32 lo, x86::reg32 d)
{
    const x86::reg64 n = (x86::reg64(hi) << 32) | lo;
    return {x86::reg32(n / d), x86::reg32(n % d)};
}

[[gnu::noinline, gnu::cold]] inline Division divideSignedWide(x86::reg32 hi, x86::reg32 lo, x86::reg32 d)
{
    const x86::sreg64 n = x86::sreg64((x86::reg64(hi) << 32) | lo);
    return {x86::reg32(n / x86::sreg32(d)), x86::reg32(n % x86::sreg32(d))};
}

// Results as the 64-bit C++ division gives them.
inline Division divideUnsigned(x86::reg32 hi, x86::reg32 lo, x86::reg32 d)
{
    if (hi == 0 && d != 0)
        return {lo / d, lo % d};
    if (hi < d)
        return divideWide(hi, lo, d);
    return divideUnsignedWide(hi, lo, d);
}

// A sign-extended dividend (cdq) takes the 32-bit division, except for the divisors 0 and -1.
inline Division divideSigned(x86::reg32 hi, x86::reg32 lo, x86::reg32 d)
{
    if (hi == x86::reg32(x86::sreg32(lo) >> 31) && d + 1u > 1u)
        return {x86::reg32(x86::sreg32(lo) / x86::sreg32(d)), x86::reg32(x86::sreg32(lo) % x86::sreg32(d))};
    return divideSignedWide(hi, lo, d);
}

template< typename T >
inline T load(x86::reg32 address)
{
    T value;
    memcpy(&value, win32::g_guestBase + address, sizeof(T));
    return value;
}

template< typename T >
inline void store(x86::reg32 address, T value)
{
    memcpy(win32::g_guestBase + address, &value, sizeof(T));
}

inline x86::reg8 ld8(x86::reg32 address)    { return load<x86::reg8>(address); }
inline x86::reg16 ld16(x86::reg32 address)  { return load<x86::reg16>(address); }
inline x86::reg32 ld32(x86::reg32 address)  { return load<x86::reg32>(address); }
inline x86::reg64 ld64(x86::reg32 address)  { return load<x86::reg64>(address); }
inline float ldf32(x86::reg32 address)      { return load<float>(address); }
inline double ldf64(x86::reg32 address)     { return load<double>(address); }

inline void st8(x86::reg32 address, x86::reg8 value)    { store(address, value); }
inline void st16(x86::reg32 address, x86::reg16 value)  { store(address, value); }
inline void st32(x86::reg32 address, x86::reg32 value)  { store(address, value); }
inline void st64(x86::reg32 address, x86::reg64 value)  { store(address, value); }
inline void stf32(x86::reg32 address, float value)      { store(address, value); }
inline void stf64(x86::reg32 address, double value)     { store(address, value); }

}

#endif /* !RECOMP_H_ */
