// Native sub_5a8790: chase camera update (thiscall, no arguments): clamps and eases the follow parameter, then rebuilds the look matrices through the vector and matrix helpers.

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5a8790_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_441000(win32::WinApplication* app, x86::CPU& cpu);
void sub_59d250(win32::WinApplication* app, x86::CPU& cpu);
void sub_43c780(win32::WinApplication* app, x86::CPU& cpu);
void sub_43c350(win32::WinApplication* app, x86::CPU& cpu);
void sub_4013f0(win32::WinApplication* app, x86::CPU& cpu);
void sub_4012b0(win32::WinApplication* app, x86::CPU& cpu);
void sub_43d1e0(win32::WinApplication* app, x86::CPU& cpu);
void sub_591480(win32::WinApplication* app, x86::CPU& cpu);
void sub_5913d0(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("chase_cam_native", true); }))
    {
        sub_5a8790_guest(app, cpu);
        return;
    }
    const reg32 fc = cpu.fpu.count;
    const reg32 entryEsp = cpu.esp;
    reg32 esp = entryEsp - 4;
    reg32 ebp = esp;
    rc::st32(esp, cpu.ebp);
    const reg32 oebx = cpu.ebx, oesi = cpu.esi, oedi = cpu.edi;
    esp = (esp & 0xfffffff0u) - 0xe4;
    esp -= 4; rc::st32(esp, oebx);
    esp -= 4; rc::st32(esp, oesi);
    esp -= 4; rc::st32(esp, oedi);
    const reg32 esi = cpu.ecx;
    reg32 ebx = esi + 0x90;
    const reg32 edi = esi + 0x30;
    const auto push = [&](reg32 v) { esp -= 4; rc::st32(esp, v); };
    const auto prepare = [&] { cpu.esp = esp - 4; cpu.fpu.count = fc; };
    const auto result = [&] { return Real(cpu.fpu.regs[(fc + 1) & 7]); };
    // sub_43c990 inline: re-pushes (out, a, b) as (out, b, a) for sub_4013f0.
    const auto matmul = [&](reg32 out, reg32 a, reg32 b) {
        rc::st32(esp - 8, a);
        rc::st32(esp - 12, b);
        rc::st32(esp - 16, out);
        cpu.eax = a; cpu.ecx = b; cpu.edx = out;
        cpu.esp = esp - 20; cpu.fpu.count = fc;
        sub_4013f0(app, cpu);
    };

    // Camera position minus the target vector, offset by the camera's own offsets.
    push(ebx);
    reg32 cam = rc::ld32(esi + 0xa4);
    push(cam + 0x30);
    reg32 ecx = esp + 0x28;
    push(ecx);
    cpu.eax = cam + 0x30; cpu.ecx = ecx; cpu.ebp = ebp;
    prepare();
    sub_441000(app, cpu);
    if (cpu.terminate) return;
    Real fp1 = ld(esp + 0x2c);
    cam = rc::ld32(esi + 0xa4);
    fp1 += ld(cam + 0x20);
    const reg32 ctl = rc::ld32(esi + 0xa0);
    Real fp2 = ld(cam + 0x24);
    esp += 0xc;
    fp2 += ld(esp + 0x24);
    Real fp3 = ld(cam + 0x28);
    fp3 += ld(esp + 0x28);
    push(cam + 0x30);
    rc::stf32(esp + 0x3c, float(fp3));
    const reg32 z = rc::ld32(esp + 0x3c);
    push(esp + 0x24);
    rc::stf32(esp + 0x28, float(fp1));
    rc::st32(esp + 0x30, z);
    rc::stf32(esp + 0x2c, float(fp2));
    push(ctl + 0x114);
    push(ctl + 0x150);
    cpu.ecx = ctl; cpu.ebx = ebx; cpu.esi = esi;
    prepare();
    sub_59d250(app, cpu);
    if (cpu.terminate) return;
    ebx = cpu.ebx;
    esp = cpu.esp;
    ebp = cpu.ebp;
    fp1 = result();

    // Clamp the follow parameter between the two limits and track its rate.
    fp1 += ld(rc::ld32(rc::ld32(esi + 0xa0) + 0x14) + 0x10);
    const reg32 limits = rc::ld32(esi + 0x14);
    rc::st32(esp + 0x18, rc::ld32(esi + 0x1c));
    const Real lo = ld(limits + 0x14);
    if (fp1 > lo)
    {
        rc::st32(esi + 0x1c, rc::ld32(limits + 0x14));
        rc::st32(esi + 0x80, 1);
    }
    else
    {
        const Real hi = ld(limits + 0x18);
        if (!(fp1 < hi))
        {
            rc::stf32(esi + 0x1c, float(fp1));
            rc::st32(esi + 0x80, 1);
        }
        else
        {
            rc::st32(esi + 0x1c, rc::ld32(limits + 0x18));
            rc::st32(esi + 0x80, 0);
        }
    }
    if (ld(0x7fc800) > ld(0x7843ec))
        rc::stf32(esi + 0x20, float((ld(esi + 0x1c) - ld(esp + 0x18)) / ld(0x7fc800)));

    const Real vy = ld(ebx + 4);
    const reg32 vx = rc::ld32(ebx);
    const Real vz = ld(ebx + 8);
    rc::st32(edi, vx);
    rc::stf32(edi + 4, float(vy));
    ecx = esp + 0x50;
    push(ecx);
    rc::stf32(edi + 8, float(vz));
    rc::stf32(esi + 0x38, float(ld(esi + 0x1c) + ld(esi + 0x38)));
    cpu.ecx = ecx;
    cpu.esp = esp - 4;
    cpu.fpu.count = fc;
    sub_43c780(app, cpu);
    if (cpu.terminate) return;
    fp1 = ld(rc::ld32(esi + 0xa0) + 0x174);
    esp += 4;
    rc::stf32(esp + 0x1c, float(fp1));
    if (fp1 == ld(0x7d7d3c))
    {
        // Only the guest's last compare before the end reaches the caller.
        cpu.fpu.status.word = x86::reg16((cpu.fpu.status.word & ~0x4500) | 0x4000);
    }
    else
    {
        push(esp + 0xb0);
        prepare();
        sub_43c780(app, cpu);
        if (cpu.terminate) return;
        ecx = rc::ld32(esi + 0xa0);
        const reg32 angle = rc::ld32(ecx + 0x24);
        push(angle);
        rc::st32(esp + 0x20, angle);
        cpu.eax = angle; cpu.ecx = ecx; cpu.edx = angle; cpu.edi = edi;
        prepare();
        sub_43c350(app, cpu);
        if (cpu.terminate) return;
        fp1 = result();
        fp1 *= ld(0x7a1b38);
        esp += 4;
        fp1 *= ld(esp + 0x20);
        rc::stf32(esp + 0x20, float(fp1));
        fp1 += ld(0x784254);
        rc::stf32(esp, float(fp1));
        prepare();
        sub_43c350(app, cpu);
        if (cpu.terminate) return;
        fp1 = result();
        ecx = rc::ld32(esp + 0x20);
        rc::stf32(esp + 0x1c, float(fp1));
        push(ecx);
        cpu.ecx = ecx;
        prepare();
        sub_43c350(app, cpu);
        if (cpu.terminate) return;
        fp1 = result();
        const reg32 s = rc::ld32(esp + 0x20);
        rc::stf32(esp + 0xc8, float(-fp1));
        rc::st32(esp + 0xb8, s);
        rc::stf32(esp + 0xbc, float(fp1));
        const reg32 b0 = esp + 0xb8;
        push(b0);
        rc::st32(esp + 0xd0, s);
        const reg32 a0 = esp + 0x5c;
        push(a0);
        const reg32 o0 = esp + 0x60;
        push(o0);
        matmul(o0, a0, b0);
        if (cpu.terminate) return;
        esp += 0x14;
    }

    // Build the view matrix from the target position and orientation.
    const Real ty = ld(edi + 4);
    const reg32 tx = rc::ld32(edi);
    rc::stf32(esp + 0x84, float(ty));
    const Real tz = ld(edi + 8);
    push(esp + 0x50);
    rc::stf32(esp + 0x8c, float(tz));
    push(esi + 0x40);
    rc::st32(esp + 0x88, tx);
    rc::st32(esp + 0x94, 0x3f800000u);
    prepare();
    sub_4012b0(app, cpu);
    if (cpu.terminate) return;
    const reg32 extra = rc::ld32(esi + 0xa8);
    esp += 8;
    if (extra != 0)
    {
        const reg32 b1 = (rc::ld32(esi + 0xac) << 6) + rc::ld32(extra + 8);
        push(b1);
        const reg32 a1 = esp + 0x54;
        push(a1);
        const reg32 o1 = esp + 0x58;
        push(o1);
        matmul(o1, a1, b1);
        if (cpu.terminate) return;
        esp += 0xc;
    }
    push(esp + 0x50);
    ecx = rc::ld32(esi + 0xa0);
    cpu.ecx = ecx;
    prepare();
    sub_591480(app, cpu);
    if (cpu.terminate) return;
    esp += 4;
    reg32 edx = rc::ld32(esi + 0xa4);
    push(edi);
    push(edx + 0x30);
    const reg32 outVec = esp + 0xa8;
    push(outVec);
    cpu.eax = outVec;
    prepare();
    sub_441000(app, cpu);
    if (cpu.terminate) return;
    edx = rc::ld32(esi + 0xa4);
    push(esp + 0xac);
    push(edx + 0xc0);
    push(esp + 0x54);
    prepare();
    sub_43d1e0(app, cpu);
    if (cpu.terminate) return;
    fp1 = ld(esp + 0x58);
    cam = rc::ld32(esi + 0xa4);
    fp1 += ld(cam + 0xa0);
    fp2 = ld(cam + 0xa4);
    fp2 += ld(esp + 0x5c);
    fp3 = ld(cam + 0xa8);
    const reg32 out = rc::ld32(esi + 0xa0) + 0xf0;
    fp3 += ld(esp + 0x60);
    const reg32 c2 = esp + 0x68;
    push(c2);
    rc::stf32(esp + 0x54, float(fp3));
    rc::stf32(esp + 0xac, float(fp1));
    const reg32 xo = rc::ld32(esp + 0xac);
    const Real third = ld(esp + 0x54);
    rc::st32(out, xo);
    rc::stf32(out + 4, float(fp2));
    const reg32 srcVec = esp + 0x6c;
    rc::stf32(out + 8, float(third));
    const reg32 b2 = rc::ld32(esi + 0xa4) + 0xd0;
    push(b2);
    push(srcVec);
    matmul(srcVec, b2, c2);
    if (cpu.terminate) return;
    ecx = rc::ld32(esi + 0xa0);
    esp += 0x24;
    push(esp + 0x50);
    cpu.ecx = ecx;
    prepare();
    sub_5913d0(app, cpu);
    if (cpu.terminate) return;
    esp += 4;

    // Final value of the lateral offset from the clamped parameter.
    reg32 eax = cpu.eax;
    bool zeroOut = rc::ld32(esi + 0x80) == 0;
    if (!zeroOut)
    {
        const Real zero = ld(0x7d7d3c);
        const Real t = ld(esi + 0x1c);
        fp1 = t > zero ? t : zero;
        ecx = rc::ld32(esi + 0x14);
        fp1 *= ld(ecx);
        fp1 += ld(0x784250);
        fp1 *= ld(ecx + 4);
        fp2 = ld(esi + 0x20);
        const reg32 slot = fp2 > zero ? 0 : 1;
        fp2 = ld(ecx + slot * 4 + 8);
        const reg32 prior = rc::ld32(esi + 0x18);
        fp2 *= ld(esi + 0x20);
        fp1 = fp2 + fp1 * ld(esi + 0x1c);
        fp1 += ld(esi + 0x88);
        rc::stf32(esi + 0x24, float(fp1));
        fp2 = ld(esi + 0x1c);
        fp2 -= ld(prior + 0x1c);
        fp2 *= ld(ecx + 0x10);
        fp1 += fp2;
        rc::stf32(esi + 0x24, float(fp1));
        cpu.fpu.compare(fp1, zero);
        eax = (prior & 0xffff0000u) | x86::reg16(cpu.fpu.status.word);
        zeroOut = fp1 < zero;
    }
    if (zeroOut)
        rc::st32(esi + 0x24, 0);

    const reg32 pedi = rc::ld32(esp);
    const reg32 pesi = rc::ld32(esp + 4);
    const reg32 pebx = rc::ld32(esp + 8);
    cpu.eax = eax;
    cpu.ecx = ecx;
    cpu.ebx = pebx;
    cpu.esi = pesi;
    cpu.edi = pedi;
    cpu.ebp = rc::ld32(ebp);
    cpu.esp = ebp + 8;
    cpu.fpu.count = fc;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const reg32 self = cpu.ecx;
    const reg32 ctl = rc::ld32(self + 0xa0);
    spans.push_back({self + 0x1c, 0x68, false});
    spans.push_back({ctl + 0x70, 0x8c, false});
    spans.push_back({ctl + 0x168, 4, false});
    spans.push_back({cpu.esp - 0x600, 0x600, false});
}

const shadow::Spec spec = {"sub_5a8790", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};

}

void sub_5a8790(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5a8790_guest, declare);
}

}}
