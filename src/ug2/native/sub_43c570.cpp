// Native sub_43c570: 16-bit angle of (x, y) from the arctangent byte table 0x7EACB0 (cdecl; result in ax). Registers,
// status word and argument slot end as in the guest.

#include <native/check.h>
#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>

namespace ug2
{

void sub_43c570_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// _ftol2 outside the int32 range, as native::sub_75bed4.
__attribute__((noinline, cold)) x86::reg64 wideTrunc(float value)
{
    asm volatile("" ::: "memory");
    return x86::reg64(x86::truncInteger64(double(value)));
}

inline x86::reg64 ftol(Real p)
{
    if (__builtin_expect((bits(float(p)) & 0x7fffffffu) < 0x4f000000u, 1))
        return x86::reg64(int64_t(x86::sreg32(p)));
    return wideTrunc(float(p));
}

constexpr x86::reg32 Table = 0x7eacb0;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("phys_vector_native"))
    {
        sub_43c570_guest(app, cpu);
        return;
    }
    const x86::reg32 E = cpu.esp;
    const Real zero = ld(0x7d7d3c);
    x86::reg32 esi = 0;
    Real x = ld(E + 4);
    cpu.fpu.compare(x, zero);
    if (x < zero)
    {
        x = -x;
        rc::stf32(E + 4, float(x));
        x = ld(E + 4);
        esi = 1;
    }
    Real y = ld(E + 8);
    if (y < zero)
    {
        esi ^= 3;
        y = -y;
    }
    x86::reg32 eax = cpu.eax, ecx, edx = cpu.edx;
    cpu.fpu.compare(x, y);
    if (x > y)
    {
        Real r = y;
        r /= x;
        r *= ld(0x786db8);
        const x86::reg64 t = ftol(r);
        eax = x86::reg32(t);
        edx = x86::reg32(t >> 32);
        const x86::reg32 at = Table - eax;
        ecx = ((at & 0xffff0000u) | rc::ld8(at)) << 5;
    }
    else if (cpu.fpu.compare(y, x), y > x)
    {
        Real r = x;
        r /= y;
        r *= ld(0x786db8);
        const x86::reg32 idx = x86::reg32(ftol(r));
        edx = Table - idx;
        eax = ((idx & 0xffff0000u) | rc::ld8(edx)) << 5;
        ecx = 0x4000u - eax;
    }
    else
        ecx = y == zero ? 0 : 0x2000u;
    if (esi == 0)
        eax = (eax & 0xffff0000u) | (ecx & 0xffffu);
    else if (esi == 3)
        eax = 0u - ecx;
    else if (esi == 1)
        eax = 0x8000u - ecx;
    else
        eax = ecx + 0x8000u;
    cpu.eax = eax;
    cpu.ecx = ecx;
    cpu.edx = edx;
    cpu.esp = E + 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{cpu.esp + 4, 4, false});
}

const x86::reg8 compared = shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp |
                           shadow::Esi | shadow::Edi;
const shadow::Spec spec = {"sub_43c570", compared, 0, false, false};
check::Stats s_check = {"sub_43c570"};

}

void sub_43c570(win32::WinApplication* app, x86::CPU& cpu)
{
    if (check::enabled()) [[unlikely]]
        check::compare(s_check, compared, app, cpu, body, sub_43c570_guest, declare);
    else
        shadow::call(spec, app, cpu, body, sub_43c570_guest, declare);
}

}}
