// Native sub_5bb430: appends one camera-facing quad (four 24-byte vertices) to the locked particle buffer
// (thiscall: quad record, ret 4). The record is read once: it must not lie in the buffer.

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5bb430_guest(win32::WinApplication* app, x86::CPU& cpu);

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

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("particles_native"))
    {
        sub_5bb430_guest(app, cpu);
        return;
    }
    const x86::reg32 slot = cpu.esp;
    cpu.esp += 8;
    if (rc::ld8(cpu.ecx) == 0)
        return;
    const x86::reg32 cursor = rc::ld32(0x86b7c8);
    cpu.ecx = cursor;
    if (cursor == 0 || cursor >= rc::ld32(0x86b7cc))
        return;
    const x86::reg32 q = rc::ld32(slot + 4);
    if ((rc::ld32(q + 0x10) & 0xff000000u) <= 0x14000000u)
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
        const x86::reg32 colour = rc::ld32(q + 0x10);
        const x86::reg32 s0 = rc::ld32(q + 0x14), t0 = rc::ld32(q + 0x18);
        const x86::reg32 s1 = rc::ld32(q + 0x1c), t1 = rc::ld32(q + 0x20);
        Real c[3], a[3], b[3];
        for (int i = 0; i < 3; ++i)
        {
            c[i] = ld(q + 4 * i);
            a[i] = ld(q + 0x30 + 4 * i);
            b[i] = ld(q + 0x40 + 4 * i);
        }
        Real p[4][3];
        for (int i = 0; i < 3; ++i)
        {
            p[0][i] = add(add(a[i], b[i]), c[i]);
            p[1][i] = add(sub(c[i], a[i]), b[i]);
            p[2][i] = sub(sub(c[i], a[i]), b[i]);
            p[3][i] = sub(add(a[i], c[i]), b[i]);
        }
        vertex(cursor, p[0][0], p[0][1], p[0][2], colour, s0, t0);
        vertex(cursor + 0x18, p[1][0], p[1][1], p[1][2], colour, s1, t0);
        vertex(cursor + 0x30, p[2][0], p[2][1], p[2][2], colour, s1, t1);
        vertex(cursor + 0x48, p[3][0], p[3][1], p[3][2], colour, s0, t1);
    }
    cpu.ecx = cursor + 0x60;
    rc::st32(0x86b7c8, cursor + 0x60);
}

void declare(x86::CPU&, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{0x86b7c8, 4, false});
    const x86::reg32 cursor = rc::ld32(0x86b7c8);
    if (cursor != 0 && cursor < rc::ld32(0x86b7cc))
        spans.push_back(shadow::Span{cursor, 0x60, false});
}

// The translated body writes back ecx only.
const shadow::Spec spec = {"sub_5bb430", shadow::Ecx, 0, false, false};

}

void sub_5bb430(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bb430_guest, declare);
}

}}
