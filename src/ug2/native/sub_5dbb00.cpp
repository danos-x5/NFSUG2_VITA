// Native sub_5dbb00: sums the lights of the global light list near a screen point into three channels and a weight, writes colour ints (cdecl).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5dbb00_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5b98a0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc200(win32::WinApplication* app, x86::CPU& cpu);
void sub_502fd0(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const x86::reg32 ListHead = 0x88f1f8;

__attribute__((noinline, cold)) x86::reg64 wideTrunc(float value)
{
    asm volatile("" ::: "memory");
    return x86::reg64(x86::truncInteger64(double(value)));
}

x86::reg32 ftol(Real p)
{
    if (__builtin_expect((bits(float(p)) & 0x7fffffffu) < 0x4f000000u, 1))
        return x86::reg32(x86::sreg32(p));
    return x86::reg32(wideTrunc(float(p)));
}

x86::reg32 callPlain(win32::WinApplication* app, x86::CPU& cpu, void (*fn)(win32::WinApplication*, x86::CPU&), x86::reg32 slot)
{
    cpu.esp = slot;
    fn(app, cpu);
    return cpu.eax;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("light_gather_native"))
    {
        sub_5dbb00_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 bp = entry - 4;
    const x86::reg32 frame = (bp & ~15u) - 0x30;
    const x86::reg32 object = argument(cpu, 0);
    const x86::sreg32 px = x86::sreg32(argument(cpu, 1));
    const x86::sreg32 py = x86::sreg32(argument(cpu, 2));
    const x86::reg32 outR = argument(cpu, 3), outG = argument(cpu, 4), outB = argument(cpu, 5), outA = argument(cpu, 6);
    const x86::reg32 savedBx = cpu.ebx, savedSi = cpu.esi, savedDi = cpu.edi, savedBp = cpu.ebp;
    const x86::reg32 zeroAt = 0x7d7d3c;

    rc::st32(frame + 0x24, 0);
    if (callPlain(app, cpu, sub_5b98a0, frame - 4) == 3 && rc::ld32(object + 4) == 2)
    {
        const x86::reg32 value = callPlain(app, cpu, sub_5bc200, frame - 4);
        rc::st32(frame + 0x28, -value);
        rc::stf32(frame + 0x24, float(Real(x86::sreg32(-value))));
    }
    x86::reg32 node = rc::ld32(ListHead);
    rc::st32(frame + 0x1c, 0);
    rc::st32(frame + 0x18, 0);
    rc::st32(frame + 0x20, 0);
    rc::st32(frame + 0x28, 0);
    cpu.ecx = ListHead;
    cpu.esi = node;
    cpu.ebp = bp;
    const x86::reg32 end = callPlain(app, cpu, sub_502fd0, frame - 4);
    rc::st32(frame + 0x2c, end);
    cpu.ebx = savedBx;
    cpu.esi = savedSi;
    cpu.edi = savedDi;
    cpu.ebp = savedBp;
    if (cpu.terminate)
        return;

    Real sumY = ld(frame + 0x20);
    Real weight = ld(frame + 0x28);
    while (node != end)
    {
        if (ld(node + 0x54) > ld(zeroAt))
        {
            x86::reg32 dx = px - ftol(ld(node + 0x44));
            if (x86::sreg32(dx) < 0)
                dx = -dx;
            const x86::reg32 ny = ftol(ld(node + 0x48));
            x86::reg32 dy = ftol(ld(frame + 0x24)) - ny + py;
            if (x86::sreg32(dy) < 0)
                dy = -dy;
            const x86::sreg32 big = x86::sreg32(dx) > x86::sreg32(dy) ? x86::sreg32(dx) : x86::sreg32(dy);
            const x86::sreg32 small = x86::sreg32(dx) > x86::sreg32(dy) ? x86::sreg32(dy) : x86::sreg32(dx);
            rc::st32(frame + 0x20, x86::reg32((small >> 2) + big));

            Real falloff = Real(x86::sreg32(rc::ld32(frame + 0x20))) * ld(0x800e50);
            falloff = ld(0x784540) - falloff;
            falloff *= ld(0x787af0);
            if (!(falloff < ld(zeroAt)))
            {
                const Real remaining = ld(0x784250) - weight;
                rc::stf32(frame + 0x20, float(remaining));
                falloff *= ld(node + 0x54);
                if (!(ld(frame + 0x20) < ld(zeroAt)))
                {
                    Real r = ld(node + 0xc) * ld(0x784260);
                    r *= falloff;
                    r *= ld(frame + 0x20);
                    r += ld(frame + 0x1c);
                    rc::stf32(frame + 0x1c, float(r));
                    Real g = ld(node + 0x10) * ld(0x784260);
                    g *= falloff;
                    g *= ld(frame + 0x20);
                    g += ld(frame + 0x18);
                    rc::stf32(frame + 0x18, float(g));
                    Real b = ld(node + 0x14) * ld(0x784260);
                    b *= falloff;
                    b *= ld(frame + 0x20);
                    sumY += b;
                }
                weight += falloff;
            }
        }
        node = rc::ld32(node);
    }

    const x86::reg32 first = rc::ld32(ListHead);
    Real scale = ld(0x800e4c) * ld(first + 0x60);
    scale *= ld(0x800e4c);
    rc::stf32(frame + 0x20, float(scale));
    if (weight > ld(zeroAt))
    {
        rc::stf32(frame + 0x1c, float(ld(frame + 0x20) * ld(frame + 0x1c)));
        rc::stf32(frame + 0x18, float(ld(frame + 0x20) * ld(frame + 0x18)));
        sumY *= ld(frame + 0x20);
        if (ld(frame + 0x1c) > ld(0x784544))
            rc::st32(frame + 0x1c, 0x437f0000);
        if (ld(frame + 0x18) > ld(0x784544))
            rc::st32(frame + 0x18, 0x437f0000);
        if (sumY > ld(0x784544))
            sumY = ld(0x784544);
    }

    rc::st32(outR, ftol(ld(frame + 0x1c)));
    rc::st32(outG, ftol(ld(frame + 0x18)));
    rc::st32(outB, ftol(sumY));
    Real total = sumY + ld(frame + 0x18);
    total += ld(frame + 0x1c);
    total *= ld(0x7a5274);
    if (total > ld(0x784250))
        total = ld(0x784250);
    Real current = Real(x86::sreg32(rc::ld32(outA)));
    if (x86::sreg32(rc::ld32(outA)) < 0)
        current += ld(0x78454c);
    current *= total;
    rc::st32(outA, ftol(current));
    cpu.esp = entry + 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 3), 4, false});
    spans.push_back(shadow::Span{argument(cpu, 4), 4, false});
    spans.push_back(shadow::Span{argument(cpu, 5), 4, false});
    spans.push_back(shadow::Span{argument(cpu, 6), 4, false});
}

const shadow::Spec spec = {"sub_5dbb00", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_5dbb00(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5dbb00_guest, declare);
}

}}
