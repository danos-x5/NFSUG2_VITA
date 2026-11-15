// Native sub_5c5930: draw-list walker (cull, material pick, one 5ceb30 submit per item). Mirrors guest registers and stack words; children run through entry points.
#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{
void sub_5c5930_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_4905a0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5bc300(win32::WinApplication* app, x86::CPU& cpu);
void sub_5ceb30(win32::WinApplication* app, x86::CPU& cpu);
void sub_488e40(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{
namespace
{
using x86::reg32;

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("ug2_draw_walk_native", true); }))
    {
        sub_5c5930_guest(app, cpu);
        return;
    }
    const reg32 E = cpu.esp;
    const reg32 B = E - 0x30;
    const reg32 oebx = cpu.ebx, oebp = cpu.ebp, oesi = cpu.esi, oedi = cpu.edi;
    const reg32 T = cpu.ecx;
    const reg32 P = rc::ld32(E + 4);
    const reg32 Q = rc::ld32(P + 0xc);
    if (Q == 0)
    {
        cpu.eax = 0;
        cpu.ebx = oebx; cpu.ebp = oebp; cpu.esi = oesi; cpu.edi = oedi;
        cpu.esp = E + 0x18;
        return;
    }
    const reg32 matrix = rc::ld32(E + 8);
    const reg32 W = rc::ld32(E + 0xc);
    const reg32 flags = rc::ld32(E + 0x10);
    const reg32 pass = rc::ld32(E + 0x14);
    reg32 edx = cpu.edx;
    const reg32 R = rc::ld32(Q);
    const reg32 K = rc::ld32(R + 0x3c);
    const reg32 kindv = rc::ld32(K + 4);
    reg32 status;
    if ((flags & 0x200) != 0 && rc::ld32(0x870cc4) != 0)
    {
        status = rc::ld32(0x870cd4) != 0 ? 0x7ff090u : 0x7ff0b4u;
        rc::st32(R + 0x38, status);
    }
    else if (rc::ld32(0x870cd4) == 0)
    {
        status = 0x7ff0d8;
        rc::st32(R + 0x38, status);
    }
    else
        status = rc::ld32(R + 0x38);
    bool flag18 = false;
    if (rc::ld8(0x8707cd) != 0)
    {
        edx = rc::ld32(0x870848);
        flag18 = rc::ld32(edx) == 0;
    }
    reg32 V = K;
    reg32 eax = status;
    if (status != 0)
    {
        eax = rc::ld32(status + rc::ld32(K + 4) * 4);
        if (eax != 0xffffffffu)
        {
            edx = rc::ld32(eax * 4 + 0x86eb28);
            V = edx;
        }
        rc::st32(R + 0x38, 0);
    }
    if (rc::ld32(P + 0x10) != 0)
    {
        cpu.eax = eax; cpu.ecx = P; cpu.edx = edx; cpu.ebx = 0; cpu.esp = B - 4;
        cpu.ebp = P; cpu.esi = flags; cpu.edi = R;
        sub_4905a0(app, cpu);
        if (cpu.terminate) return;
    }
    // sub_60fdf0 inlined: it only sets eax; cpu.esp is rewritten before every later call.
    if (rc::ld32(P + 8) == 0xbe43edbbu)
    {
        const bool four = rc::ld32(0x8a1c8c) == 4;
        cpu.eax = four;
        if (four && rc::ld32(0x870d14) != 0)
            V = rc::ld32(0x86eb78);
    }
    reg32 esp = B;
    reg32 esi = flags;
    if (rc::ld32(V + 0x10) != 0)
    {
        if (rc::ld32(rc::ld32(0x870848)) != 0 && rc::ld32(V + 4) != 5)
            V = rc::ld32(0x86eb44);
        if (int32_t(rc::ld32(R + 0x10)) > 0)
        {
            reg32 ebx = W;
            const bool cull = (flags & 4) == 0;
            const bool kind23 = kindv == 2 || kindv == 3;
            for (reg32 i = 0, off = 0;; ++i, off += 0x3c)
            {
                esi = rc::ld32(R + 0x14) + off;
                if (cull && (rc::ld8(esi + 0x38) & 1) == 0)
                {
                    rc::st32(B - 4, matrix);
                    rc::st32(B - 8, esi + 0x10);
                    rc::st32(B - 0xc, esi);
                    cpu.eax = matrix; cpu.ecx = T; cpu.ebx = ebx; cpu.esp = B - 0x10;
                    cpu.ebp = P; cpu.esi = esi; cpu.edi = R;
                    sub_5bc300(app, cpu);
                    if (cpu.terminate) return;
                    esi = cpu.esi;
                    if (cpu.eax == 0)
                        goto next;
                }
                {
                    const reg32 pm = rc::ld32(rc::ld32(Q + 0x2c) + rc::ld8(esi + 0x1c) * 8 + 4);
                    const reg32 mid = rc::ld32(pm + 0x24);
                    if (mid == rc::ld32(0x870a18))
                        goto next;
                    const reg32 idx = rc::ld8(esi + 0x20);
                    reg32 e;
                    if (idx != 0xff)
                        e = rc::ld32(rc::ld32(Q + 0x3c) + idx * 8 + 4);
                    else
                    {
                        // sub_48ac50 inlined; the 5ceb30 setup below rewrites cpu.esp.
                        e = rc::ld32(0x7f399c);
                        cpu.eax = e;
                    }
                    const uint8_t dl = rc::ld8(pm + 0x56);
                    reg32 ecx = V;
                    if (dl == 2 && rc::ld32(V + 4) <= 1)
                        ecx = rc::ld32(0x86eb44);
                    reg32 edi = flag18;
                    if (flag18)
                    {
                        const reg32 k = rc::ld32(ecx + 4);
                        if (dl != 2)
                        {
                            edi = kindv;
                            if (kind23)
                            {
                                if (k <= 3)
                                    ecx = rc::ld32(0x86eb4c + k * 4);
                                goto chosen;
                            }
                            if (dl == 0)
                                goto tail;
                        }
                        if (k == 0)
                        {
                            ecx = rc::ld32(0x86eb5c);
                            goto chosen;
                        }
                        if (k == 1 || k == 7)
                        {
                            ecx = rc::ld32(0x86eb60);
                            goto chosen;
                        }
                    tail:
                        if (k == 2)
                            ecx = rc::ld32(0x86eb54);
                        else if (k == 3)
                            ecx = rc::ld32(0x86eb58);
                    }
                chosen:
                    if (ebx == 0 && rc::ld32(ecx + 4) == 5)
                        ebx = rc::ld32(T + 0x54);
                    if (mid == 0x6d57415u || mid == 0xbd3d01a8u)
                        ecx = rc::ld32(0x86eb80);
                    rc::st32(B - 4, pass);
                    rc::st32(B - 8, e);
                    rc::st32(B - 0xc, ebx);
                    rc::st32(B - 0x10, matrix);
                    rc::st32(B - 0x14, pm);
                    rc::st32(B - 0x18, ecx);
                    rc::st32(B - 0x1c, rc::ld32(esi + 0x38));
                    rc::st32(B - 0x20, Q);
                    rc::st32(B - 0x24, esi);
                    cpu.edx = Q; cpu.ebx = ebx; cpu.esp = B - 0x28;
                    cpu.ebp = pm; cpu.esi = esi; cpu.edi = edi;
                    sub_5ceb30(app, cpu);
                    if (cpu.terminate) return;
                    ebx = cpu.ebx;
                    esp = cpu.esp + 0x24;
                    esi = cpu.esi;
                }
            next:
                if (!(int32_t(i + 1) < int32_t(rc::ld32(R + 0x10))))
                    break;
            }
        }
    }
    cpu.ecx = P; cpu.esp = B - 4; cpu.esi = esi; cpu.edi = R;
    sub_488e40(app, cpu);
    if (cpu.terminate) return;
    cpu.ebx = oebx; cpu.ebp = oebp; cpu.esi = oesi; cpu.edi = oedi;
    cpu.esp = esp + 0x30 + 0x18;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const reg32 q = rc::ld32(rc::ld32(cpu.esp + 4) + 0xc);
    if (q != 0)
        spans.push_back({rc::ld32(q) + 0x38, 4, false});
}
const shadow::Spec spec = {"sub_5c5930", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};
}

void sub_5c5930(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5c5930_guest, declare);
}
}}
