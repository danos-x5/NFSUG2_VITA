// Native sub_43c2b0: odd polynomial sine-like of a u16 angle, pushed on the x87 stack (cdecl: angle word).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_43c2b0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("sine_poly_native", true); }))
    {
        sub_43c2b0_guest(app, cpu);
        return;
    }
    const x86::reg32 angle = rc::ld16(cpu.esp + 4);
    rc::st32(cpu.esp + 4, angle);
    Real x = Real(x86::sreg32(angle)) * ld(0x784240);
    Real b;
    const Real t0 = ld(0x786da0);
    if (x >= t0)
    {
        cpu.fpu.compare(x, t0);
        b = ld(0x784250);
        x = x - ld(0x786d9c);
    }
    else
    {
        // The second compare rewrites every status bit the first one set.
        const Real t1 = ld(0x784254);
        cpu.fpu.compare(x, t1);
        if (x >= t1)
        {
            x = x - ld(0x786d98);
            b = ld(0x7842a0);
        }
        else
        {
            b = ld(0x784250);
        }
    }

    const Real p2 = x * x;
    const Real p3 = p2 * x;
    const Real p5 = p3 * p2;
    const Real p7 = p5 * p2;
    const Real p9 = p7 * p2;
    const Real t3 = p3 * ld(0x786d94);
    const Real t5 = p5 * ld(0x786d90);
    const Real t7 = p7 * ld(0x786d8c);
    const Real t9 = p9 * ld(0x786d88);
    Real a = x - t3;
    a = a + t5;
    a = a - t7;
    a = a + t9;
    const Real result = a * b;

    cpu.eax = cpu.fpu.status.word;
    cpu.fpu.regs[(cpu.fpu.count + 1) & 7] = result;
    cpu.fpu.count += 1;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{cpu.esp + 4, 4, false});
}

const shadow::Spec spec = {"sub_43c2b0", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi | shadow::Esp, 0, true, false};

}

void sub_43c2b0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_43c2b0_guest, declare);
}

}}
