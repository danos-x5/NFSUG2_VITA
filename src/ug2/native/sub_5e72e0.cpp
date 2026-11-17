// Native sub_5e72e0: ground height, material and normal query against the surface cache (thiscall, ret 16, result in st0).

#include <algorithm>

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_5e72e0_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_5e68d0(win32::WinApplication* app, x86::CPU& cpu);
void sub_5e0c90(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

constexpr int maxListed = 512;

// _ftol2 outside the int32 range, as native::sub_75bed4.
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

void inc8(x86::reg32 address, int delta)
{
    rc::st8(address, uint8_t(rc::ld8(address) + delta));
}

// One edge term of sub_5d3330.
Real edge(x86::reg32 patch, x86::reg32 a, x86::reg32 b, Real px, Real py)
{
    Real t1 = ld(patch + b + 4);
    t1 -= ld(patch + a + 4);
    Real t2 = px;
    t2 -= ld(patch + a);
    t1 *= t2;
    Real t3 = py;
    t3 -= ld(patch + a + 4);
    Real t4 = ld(patch + b);
    t4 -= ld(patch + a);
    t3 *= t4;
    t1 -= t3;
    return t1;
}

// sub_5d3330 inlined: writes only its argument slot, edx = pos.
Real pointTest(x86::reg32 patch, Real px, Real py)
{
    const Real k2 = ld(0x7842a0);
    const Real z = ld(0x7d7d3c);
    Real r = k2;
    const Real e1 = edge(patch, 0x30, 0x40, px, py);
    if (!(k2 > e1))
    {
        r = e1;
        if (e1 > z)
            return e1;
    }
    const Real e2 = edge(patch, 0x40, 0x50, px, py);
    if (!(r > e2))
        r = e2;
    if (r > z)
        return r;
    const Real e3 = edge(patch, 0x60, 0x30, px, py);
    if (!(r > e3))
        r = e3;
    if (r > z)
        return r;
    if (rc::ld8(patch + 9) != 4)
        return r;
    const Real e4 = edge(patch, 0x50, 0x60, px, py);
    if (!(r > e4))
        r = e4;
    return r;
}

Real height(x86::reg32 patch, Real px, Real py)
{
    Real t1 = ld(patch + 0x70);
    t1 *= px;
    Real t2 = ld(patch + 0x74);
    t2 *= py;
    t1 += t2;
    t1 -= ld(patch + 0x7c);
    Real q = ld(0x7842a0);
    q /= ld(patch + 0x78);
    t1 *= q;
    return t1;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("ground_query_native"))
    {
        ug2::sub_5e72e0_guest(app, cpu);
        return;
    }
    const x86::reg32 entry = cpu.esp;
    const x86::reg32 fpuCount = cpu.fpu.count;
    const x86::reg32 self = cpu.ecx;
    const x86::reg32 pos = rc::ld32(entry + 4);
    const x86::reg32 pmat = rc::ld32(entry + 8);
    const x86::reg32 pnorm = rc::ld32(entry + 12);
    const x86::reg32 pfound = rc::ld32(entry + 16);
    const x86::reg32 savedEbx = cpu.ebx, savedEsi = cpu.esi, savedEdi = cpu.edi, savedEbp = cpu.ebp;

    const auto leave = [&](Real result, x86::reg32 ecx, x86::reg32 edx) {
        cpu.eax = pnorm;
        cpu.ecx = ecx;
        cpu.edx = edx;
        cpu.ebx = savedEbx;
        cpu.esi = savedEsi;
        cpu.edi = savedEdi;
        cpu.ebp = savedEbp;
        cpu.esp = entry + 20;
        cpu.fpu.count = fpuCount + 1;
        cpu.fpu.regs[(fpuCount + 1) & 7] = result;
    };
    const auto writeNone = [&] {
        if (pfound)
            rc::st8(pfound, 0);
        if (pmat)
            rc::st32(pmat, 1);
        if (pnorm)
        {
            rc::st32(pnorm, 0);
            rc::st32(pnorm + 4, 0);
            rc::st32(pnorm + 8, 0xbf800000);
        }
    };

    cpu.edi = self;
    cpu.ebp = pos;
    rc::st32(entry - 0x248, pos);
    rc::st32(entry - 0x244, 0x41100000);
    cpu.esp = entry - 0x24c;
    ug2::sub_5e68d0(app, cpu);

    const x86::reg32 world = rc::ld32(self + 8);
    if (world == 0)
    {
        writeNone();
        return leave(Real(rc::ldf32(pos + 8)), cpu.ecx, cpu.edx);
    }

    float limit = float(Real(rc::ldf32(pos + 8)) + ld(0x784250));

    const int32_t nA = int32_t(rc::ld32(self + 0x2bc));
    for (int32_t i = 0; i < nA; i++)
    {
        const x86::reg32 patch = rc::ld32(self + 0x2c0 + 4 * x86::reg32(i));
        const Real px = ld(pos);
        const Real py = ld(pos + 4);
        if (!(pointTest(patch, px, py) < ld(0x7d7d3c)))
            continue;
        const Real h = height(patch, px, py);
        if (rc::ld8(patch + 0xa) & 1)
        {
            if (!(h <= Real(limit)))
                continue;
            Real d = Real(limit);
            d -= h;
            if (!(d < ld(0x7844f8)))
                continue;
        }
        x86::reg32 edx = pos;
        if (i > 0)
            edx = rc::ld32(self + 0x2c0);
        for (int32_t k = i; k > 0; k--)
            rc::st32(self + 0x2c0 + 4 * x86::reg32(k), rc::ld32(self + 0x2c0 + 4 * x86::reg32(k - 1)));
        rc::st32(self + 0x2c0, patch);
        x86::reg32 ecx = patch;
        if (pfound)
            rc::st8(pfound, 1);
        if (pmat)
        {
            edx = x86::reg32(int32_t(int8_t(rc::ld8(patch + 0xb))));
            rc::st32(pmat, edx);
        }
        if (pnorm)
        {
            ecx = rc::ld32(patch + 0x70);
            rc::st32(pnorm, ecx);
            rc::stf32(pnorm + 4, float(ld(patch + 0x74)));
            rc::stf32(pnorm + 8, float(ld(patch + 0x78)));
        }
        return leave(h, ecx, edx);
    }

    for (int32_t k = 0; k < int32_t(rc::ld32(self + 0x2f0)); k++)
        rc::st8(rc::ld32(self + 0x2f4 + 4 * x86::reg32(k)) + 8, 1);

    const Real px = ld(pos);
    const Real py = ld(pos + 4);
    Real fx = px;
    fx -= ld(world + 0x38);
    fx *= ld(0x7a5180);
    const x86::reg32 ix = ftol(fx);
    Real fy = py;
    fy -= ld(world + 0x3c);
    fy *= ld(0x7a5180);
    const x86::reg32 iy = ftol(fy);
    Real cx = Real(x86::sreg32(ix));
    cx *= ld(0x7875b4);
    cx += ld(world + 0x38);
    Real cy = Real(x86::sreg32(iy));
    cy *= ld(0x7875b4);
    cy += ld(world + 0x3c);
    const float cyStored = float(cy);
    Real gx = px;
    gx -= cx;
    gx *= ld(0x788164);
    const x86::reg32 qx = ftol(gx);
    Real gy = py;
    gy -= Real(cyStored);
    gy *= ld(0x788164);
    const x86::reg32 qy = ftol(gy);
    const x86::reg32 mask = (x86::reg32(1) << (qx & 31)) << ((qy << 2) & 31);

    const int32_t width = int16_t(rc::ld16(world + 0x34));
    const int32_t height2 = int16_t(rc::ld16(world + 0x36));
    const int32_t ci = std::min(width - 1, std::max(int32_t(ix), 0));
    const int32_t cj = std::min(height2 - 1, std::max(int32_t(iy), 0));
    const x86::reg32 cell = rc::ld32(world + 0x20) + 4 * x86::reg32(width * cj + ci);

    x86::reg32 best = 0;
    float bestV = 0;
    {
        const x86::reg32 init = 0xc7c34f80u;
        memcpy(&bestV, &init, 4);
    }
    x86::reg32 listing[maxListed];
    int32_t used = 0;

    for (int pass = 0; pass < 2; pass++)
    {
        if (pass == 1)
        {
            const x86::reg32 big = 0x7f7fffffu;
            memcpy(&limit, &big, 4);
        }
        Real sx = ld(pos);
        sx *= ld(0x784238);
        const int32_t X = int32_t(ftol(sx));
        Real sy = ld(pos + 4);
        sy *= ld(0x784238);
        const int32_t Y = int32_t(ftol(sy));
        const int32_t start = int32_t(rc::ld16(cell));
        const int32_t end = start + int32_t(rc::ld16(cell + 2));
        // Reloaded after every guest store below.
        x86::reg32 ents = rc::ld32(world + 0x18), pats = rc::ld32(world + 8);
        for (int32_t e = start; e < end; e++)
        {
            const x86::reg32 ent = ents + 4 * x86::reg32(e);
            if (!(mask & rc::ld16(ent + 2)))
                continue;
            const x86::reg32 pat = pats + (x86::reg32(rc::ld16(ent)) << 5);
            if (rc::ld8(pat + 3) & 0x0a)
                break;
            const x86::reg32 b1 = rc::ld8(pat + 1);
            const auto edgeAt = [&](x86::reg32 off) { return int32_t(x86::reg32(int32_t(int16_t(rc::ld16(pat + off)))) << 13); };
            if (X < edgeAt(8 + 2 * (b1 & 3)))
                continue;
            if (Y < edgeAt(0x10 + 2 * ((b1 >> 2) & 3)))
                continue;
            if (X > edgeAt(8 + 2 * ((b1 >> 4) & 3)))
                continue;
            if (Y > edgeAt(0x10 + 2 * (b1 >> 6)))
                continue;
            const x86::reg32 t = rc::ld8(pat);
            if (t != 0 && int8_t(rc::ld8(rc::ld32(0x88ff94) + (t << 5) + 8)) == 0)
                continue;
            x86::reg32 patch;
            const int16_t index = int16_t(rc::ld16(pat + 6));
            if (index >= 0)
            {
                patch = rc::ld32(0x883d90) + (x86::reg32(index) << 7);
                inc8(patch + 8, 1);
            }
            else
            {
                const x86::reg32 esp = entry - 0x254;
                rc::st32(esp + 4, pat);
                cpu.ecx = world;
                cpu.esp = esp;
                cpu.fpu.count = fpuCount;
                ug2::sub_5e0c90(app, cpu);
                patch = cpu.eax;
            }
            ents = rc::ld32(world + 0x18);
            pats = rc::ld32(world + 8);
            if (used < maxListed)
                listing[used] = patch;
            used++;
            const Real qpx = ld(pos);
            const Real qpy = ld(pos + 4);
            const Real v = pointTest(patch, qpx, qpy);
            const Real z = ld(0x7d7d3c);
            if (!(v <= z))
                continue;
            const float h = float(height(patch, qpx, qpy));
            const x86::reg32 flags = rc::ld8(patch + 0xa);
            if (!(flags & 1))
            {
                if (v < z)
                {
                    best = patch;
                    bestV = h;
                    break;
                }
            }
            if (!(Real(h) > Real(bestV)))
                continue;
            if (!(Real(h) <= Real(limit)))
                continue;
            bestV = h;
            best = patch;
            if (flags & 0x40)
                continue;
            Real d = Real(limit);
            d -= Real(h);
            if (d < ld(0x7844f8))
                break;
        }
        if (best != 0)
            break;
    }

    const x86::reg32 listBCount = rc::ld32(self + 0x2f0);
    x86::reg32 ecx = listBCount;
    if (int32_t(listBCount) > 0)
    {
        for (int32_t k = 0; k < int32_t(rc::ld32(self + 0x2f0)); k++)
            rc::st8(rc::ld32(self + 0x2f4 + 4 * x86::reg32(k)) + 8, 0);
        ecx = self + 0x2f4 + 4 * rc::ld32(self + 0x2f0);
    }
    const int32_t kept = std::min(used, maxListed);
    for (int32_t k = 0; k < kept; k++)
    {
        inc8(listing[k] + 8, -1);
        ecx = listing[k];
    }
    x86::reg32 edx = self;
    if (best == 0)
    {
        writeNone();
        return leave(Real(rc::ldf32(pos + 8)), ecx, edx);
    }
    if (!(rc::ld8(best + 0xa) & 0x40))
    {
        inc8(best + 8, 1);
        int32_t n = int32_t(rc::ld32(self + 0x2bc)) + 1;
        rc::st32(self + 0x2bc, x86::reg32(n));
        if (n > 4)
        {
            rc::st32(self + 0x2bc, 4);
            inc8(rc::ld32(self + 0x2cc) + 8, -1);
        }
        int32_t m = int32_t(rc::ld32(self + 0x2bc)) - 1;
        if (m > 0)
        {
            for (; m > 0; m--)
                rc::st32(self + 0x2c0 + 4 * x86::reg32(m), rc::ld32(self + 0x2c0 + 4 * x86::reg32(m - 1)));
            ecx = 0;
        }
        else
            ecx = x86::reg32(m);
        rc::st32(self + 0x2c0, best);
    }
    if (pfound)
        rc::st8(pfound, 1);
    if (pmat)
    {
        ecx = x86::reg32(int32_t(int8_t(rc::ld8(best + 0xb))));
        rc::st32(pmat, ecx);
    }
    if (pnorm)
    {
        edx = rc::ld32(best + 0x70);
        rc::st32(pnorm, edx);
        rc::stf32(pnorm + 4, float(ld(best + 0x74)));
        rc::stf32(pnorm + 8, float(ld(best + 0x78)));
    }
    leave(Real(bestV), ecx, edx);
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const x86::reg32 self = cpu.ecx;
    if (const x86::reg32 pmat = argument(cpu, 1))
        spans.push_back({pmat, 4, false});
    if (const x86::reg32 pnorm = argument(cpu, 2))
        spans.push_back({pnorm, 12, false});
    if (const x86::reg32 pfound = argument(cpu, 3))
        spans.push_back({pfound, 1, false});
    spans.push_back({self + 0x2bc, 4, false});
    spans.push_back({self + 0x2c0, 16, false});
    const int32_t count = int32_t(rc::ld32(self + 0x2f0));
    for (int32_t k = 0; k < count && k < 64; k++)
        spans.push_back({rc::ld32(self + 0x2f4 + 4 * x86::reg32(k)) + 8, 1, false});
}

const shadow::Spec spec = {"sub_5e72e0", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, true, false};

}

void sub_5e72e0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5e72e0_guest, declare);
}

}}
