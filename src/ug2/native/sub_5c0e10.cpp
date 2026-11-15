// Native sub_5c0e10: one particle into the batch (cdecl: particle record); the first of a batch binds matrices and
// texture and locks the buffer. The quad of sub_5bb430 is built inline.

#include <native/common.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5c0e10_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bdde0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bcc30(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bb320(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

#if defined(__ARM_FP) && !defined(NFSU_FPU_DOUBLE)

// Fixed operand order: of two NaNs the first wins, as in the translated body.
inline float add(float a, float b)
{
    float r;
#ifdef __aarch64__
    __asm__("fadd %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
#else
    __asm__("vadd.f32 %0, %1, %2" : "=t"(r) : "t"(a), "t"(b));
#endif
    return r;
}

inline float mul(float a, float b)
{
    float r;
#ifdef __aarch64__
    __asm__("fmul %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
#else
    __asm__("vmul.f32 %0, %1, %2" : "=t"(r) : "t"(a), "t"(b));
#endif
    return r;
}

inline float sub(float a, float b)
{
    float r;
#ifdef __aarch64__
    __asm__("fsub %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
#else
    __asm__("vsub.f32 %0, %1, %2" : "=t"(r) : "t"(a), "t"(b));
#endif
    return r;
}

#else

inline Real add(Real a, Real b) { return a + b; }
inline Real sub(Real a, Real b) { return a - b; }
inline Real mul(Real a, Real b) { return a * b; }

#endif

inline void vertex(x86::reg32 v, Real x, Real y, Real z, x86::reg32 colour, x86::reg32 s, x86::reg32 t)
{
    rc::stf32(v, float(x));
    rc::stf32(v + 4, float(y));
    rc::stf32(v + 8, float(z));
    rc::st32(v + 0xc, colour);
    rc::st32(v + 0x10, s);
    rc::st32(v + 0x14, t);
}

// Batch start: the guest's frame and registers, for its three callees.
[[gnu::noinline]] bool begin(win32::WinApplication* app, x86::CPU& cpu, x86::reg32 p)
{
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 ebx = cpu.ebx, ebp = cpu.ebp, esi = cpu.esi, edi = cpu.edi;
    rc::st32(entry - 4, ebp);
    const x86::reg32 frame = ((entry - 4) & 0xfffffff0u) - 0x64 - 12;
    cpu.ebp = entry - 4;
    cpu.esi = p;
    cpu.edi = rc::ld32(p + 8);
    cpu.ebx = rc::ld32(p + 4);
    rc::st8(0x872b98, 1);
    rc::st32(frame - 4, rc::ld32(rc::ld32(p) + 4));
    rc::st32(frame - 8, 0x872e90);
    cpu.esp = frame - 12;
    sub_5bdde0(app, cpu);
    if (cpu.terminate)
        return false;
    rc::st32(frame - 12, cpu.ebx);
    cpu.esp = frame - 16;
    sub_5bcc30(app, cpu);
    if (cpu.terminate)
        return false;
    rc::st32(frame + 0xc, cpu.edi);
    rc::st32(frame - 4, frame + 0xc);
    cpu.ecx = 0x86b7a4;
    cpu.esp = frame - 8;
    sub_5bb320(app, cpu);
    if (cpu.terminate)
        return false;
    rc::st32(0x87074c, 1);
    cpu.ebx = ebx;
    cpu.ebp = ebp;
    cpu.esi = esi;
    cpu.edi = edi;
    cpu.esp = entry;
    return true;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("particles_native", true); }))
    {
        sub_5c0e10_guest(app, cpu);
        return;
    }
    const x86::reg32 p = argument(cpu, 0);
    if (rc::ld32(0x87074c) == 0 && !begin(app, cpu, p))
        return;

    const Real c[3] = {ld(p + 0x10), ld(p + 0x14), ld(p + 0x18)};
    const Real width = ld(p + 0x30);
    const Real height = ld(p + 0x34);
    Real a[3], b[3];
    for (int i = 0; i < 3; ++i)
    {
        a[i] = mul(ld(0x873370 + 4 * i), width);
        b[i] = mul(ld(0x8763c0 + 4 * i), height);
    }
    const auto ftol2 = [](Real value) { return x86::reg32(x86::truncInteger64(double(value))); };
    x86::reg32 colour = ftol2(ld(p + 0x2c)) << 8;
    colour = (colour | ftol2(ld(p + 0x20))) << 8;
    colour = (colour | ftol2(ld(p + 0x24))) << 8;
    colour |= ftol2(ld(p + 0x28));
    const x86::reg32 s0 = rc::ld32(p + 0x48), t0 = rc::ld32(p + 0x4c);
    const x86::reg32 s1 = rc::ld32(p + 0x40), t1 = rc::ld32(p + 0x44);

    x86::reg32 ecx = 0x872b98;
    if (rc::ld8(0x872b98) != 0)
    {
        const x86::reg32 cursor = rc::ld32(0x86b7c8);
        ecx = cursor;
        if (cursor != 0 && cursor < rc::ld32(0x86b7cc))
        {
            if ((colour & 0xff000000u) <= 0x14000000u)
            {
                for (x86::reg32 v = cursor; v != cursor + 0x60; v += 0x18)
                {
                    rc::st32(v, 0);
                    rc::st32(v + 4, 0);
                    rc::st32(v + 8, 0);
                }
            }
            else
            {
                Real q[4][3];
                for (int i = 0; i < 3; ++i)
                {
                    q[0][i] = add(add(a[i], b[i]), c[i]);
                    q[1][i] = add(sub(c[i], a[i]), b[i]);
                    q[2][i] = sub(sub(c[i], a[i]), b[i]);
                    q[3][i] = sub(add(a[i], c[i]), b[i]);
                }
                vertex(cursor, q[0][0], q[0][1], q[0][2], colour, s0, t0);
                vertex(cursor + 0x18, q[1][0], q[1][1], q[1][2], colour, s1, t0);
                vertex(cursor + 0x30, q[2][0], q[2][1], q[2][2], colour, s1, t1);
                vertex(cursor + 0x48, q[3][0], q[3][1], q[3][2], colour, s0, t1);
            }
            ecx = cursor + 0x60;
            rc::st32(0x86b7c8, ecx);
        }
    }
    rc::st32(0x870750, rc::ld32(0x870750) + 1);
    cpu.eax = 1;
    cpu.ecx = ecx;
    cpu.esp += 4;
}

}

// No shadow compare: the batch start locks the vertex buffer: a second run would lock it again.
void sub_5c0e10(win32::WinApplication* app, x86::CPU& cpu)
{
    body(app, cpu);
}

}}
