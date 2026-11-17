// Native sub_5dd360: out = V1 + arg2 * V2, then out += arg1 * V3 (thiscall, ret 12), sub_43d0c0 order kept.

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5dd360_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// Sign-extended int8 field, shifted left, converted to float.
x86::Float shiftedInt8(x86::reg32 address, unsigned shift)
{
    return x86::Float(x86::sreg32(x86::reg32(x86::reg32(x86::sreg8(rc::ld8(address))) << shift)));
}

// Sign-extended int16 field, shifted left, converted to float.
x86::Float shiftedInt16(x86::reg32 address, unsigned shift)
{
    return x86::Float(x86::sreg32(x86::reg32(x86::reg32(x86::sreg16(rc::ld16(address))) << shift)));
}

// Sign-extended int16 field converted to float.
x86::Float int16Field(x86::reg32 address)
{
    return x86::Float(x86::sreg32(x86::reg32(x86::sreg16(rc::ld16(address)))));
}

// Hides a value so NaN-sensitive folds match the guest.
x86::Float opaque(x86::Float x)
{
    asm("" : "+w"(x));
    return x;
}

// t * dir + base in sub_43d0c0 operand order.
x86::Float mulAdd(x86::Float t, x86::Float dir, x86::Float base)
{
    x86::Float x = t;
    x *= dir;
    x += base;
    return x;
}

// Two sub_43d0c0 vector multiply-adds kept in registers.
void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("vec_blend_native"))
    {
        sub_5dd360_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 self = cpu.ecx;
    const x86::reg32 out = argument(cpu, 0);
    const x86::reg32 t1Bits = argument(cpu, 1);
    const x86::Float t1 = opaque(ld(entry + 8));
    const x86::Float t2 = opaque(ld(entry + 12));
    const x86::Float k1 = ld(0x88ea70u);
    const x86::Float k2 = ld(0x784278u);

    x86::Float a = int16Field(self + 0x18);
    a *= k1;
    x86::Float b = int16Field(self + 0x1a);
    b *= k1;
    x86::Float c = shiftedInt16(self + 0x12, 13);
    c *= k2;
    x86::Float d = shiftedInt8(self + 0x21, 9);
    d *= k2;
    x86::Float e = shiftedInt8(self + 0x20, 9);
    e *= k2;
    a = opaque(a);
    b = opaque(b);
    d = opaque(d);
    e = opaque(e);
    const x86::Float negA = opaque(-a);
    // Hidden last so the compiler keeps fadd(product, c), matching the guest's NaN pick.
    c = opaque(c);

    const x86::Float r0 = mulAdd(t1, a, mulAdd(t2, b, ld(self)));
    const x86::Float r1 = mulAdd(t1, b, mulAdd(t2, negA, ld(self + 0x4)));
    const x86::Float r2 = mulAdd(t1, d, mulAdd(t2, e, c));

    rc::stf32(out, float(r0));
    rc::stf32(out + 0x4u, float(r1));
    rc::stf32(out + 0x8u, float(r2));

    cpu.eax = out;
    cpu.ecx = out;
    cpu.edx = t1Bits;
    // The guest leaves cpu.esp at its last call's stack pointer, not at entry + 16.
    cpu.esp = (((entry - 0x4u) & ~x86::reg32(15)) - 0x3c) - 0x28;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 12, false});
}

const shadow::Spec spec = {"sub_5dd360", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi | shadow::Esp, 0, false, false};

}

void sub_5dd360(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5dd360_guest, declare);
}

}}
